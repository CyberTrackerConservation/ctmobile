#include "Wizard.h"
#include "App.h"
#include "Form.h"

namespace
{
    Form* form(const QObject* obj)
    {
        return Form::parentForm(obj);
    }

    FieldManager* fieldManager(const QObject* obj)
    {
        return Form::parentForm(obj)->fieldManager();
    }
}

Wizard::Wizard(QObject* parent): QObject(parent)
{
    m_sighting = form(this)->sighting();
    m_canNext = m_canSave = m_canSkip = false;
}

void Wizard::reset()
{
    m_sighting->set_wizardPageStack(QVariantList());
    m_sighting->set_wizardRecordUid("");
    m_sighting->set_wizardFieldUids(QStringList());
    form(this)->saveState();
}

QString Wizard::pageUrl() const
{
    return "qrc:/imports/CyberTracker.1/FormWizardPage.qml";
}

QString Wizard::indexPageUrl() const
{
    return "qrc:/imports/CyberTracker.1/FormWizardIndexPage.qml";
}

void Wizard::first(const QString& recordUid, int transition)
{
    next(recordUid, "", transition);
}

void Wizard::back(int transition)
{
    auto updateButtonsOnLeave = qScopeGuard([&] { updateButtons(); });

    auto wizardPageStack = m_sighting->wizardPageStack();
    qFatalIf(wizardPageStack.isEmpty(), "Bad wizard state during back");

    // Remove the top-most page.
    wizardPageStack.removeLast();
    m_sighting->set_wizardPageStack(wizardPageStack);

    // If the wizard is empty, pop the main wizard page off.
    if (wizardPageStack.isEmpty())
    {
        m_sighting->set_wizardRecordUid("");
        m_sighting->set_wizardFieldUids(QStringList());
        form(this)->popPage();

        // Save the sighting.
        form(this)->saveSighting();
        return;
    }

    // Update the existing page.
    form(this)->replaceLastPage(pageUrl(), wizardPageStack.constLast().toMap(), transition);
}

bool Wizard::advance(const QString& recordUid, const QString& fieldUid, QVariantList* wizardPageStackOut, QString* errorOut, bool* lastPageOut)
{
    // Validate the current field value.
    if (errorOut)
    {
        *errorOut = error(recordUid, fieldUid);
        if (!errorOut->isEmpty())
        {
            return false;
        }
    }

    // Special handling for "field-list" appearance.
    auto recordField = m_sighting->getRecord(recordUid)->recordField();
    if (recordField->wizardFieldList() && fieldUid.isEmpty())
    {
        // Advance to the next field in the parent.
        if (recordField->group())
        {
            return advance(m_sighting->getRecord(recordUid)->parentRecordUid(), recordField->uid(), wizardPageStackOut, errorOut, lastPageOut);
        }

        // Page is a record editor for a multi-record field. Reverse to the record list page.
        reverse(wizardPageStackOut, [&](const QString& rUid, const QString& /*fUid*/) -> bool
        {
            return rUid != recordUid;
        });

        return true;
    }

    if (recordField->wizardFieldList() && !fieldUid.isEmpty())
    {
        reverse(wizardPageStackOut, [&](const QString& rUid, const QString& fUid) -> bool
        {
            return rUid == recordUid && fUid.isEmpty();
        });

        return true;
    }

    // Get the next visible field in the current record.
    auto nextFieldUid = QString();
    {
        auto fieldUids = QStringList();

        // The root record may have a list of fields.
        if (recordUid == m_sighting->wizardRecordUid())
        {
            fieldUids = m_sighting->wizardFieldUids();
        }

        // If no list, just use the record.
        if (fieldUids.isEmpty())
        {
            recordField->enumFields([&](BaseField* field)
            {
                fieldUids.append(field->uid());
            });
        }

        // Find the next visible field.
        for (auto i = fieldUids.indexOf(fieldUid) + 1; i < fieldUids.count(); i++)
        {
            auto fieldUid = fieldUids[i];
            if (m_sighting->getRecord(recordUid)->getFieldVisible(fieldUid))
            {
                nextFieldUid = fieldUid;
                break;
            }
        }
    }

    // Found a next field.
    if (!nextFieldUid.isEmpty())
    {
        auto nextField = fieldManager(this)->getField(nextFieldUid);

        // Flat field.
        if (!nextField->isRecordField())
        {
            wizardPageStackOut->append(QVariantMap {{ "recordUid", recordUid }, { "fieldUid", nextFieldUid }});
            return true;
        }

        // Record field.
        auto nextRecordField = RecordField::fromField(nextField);
        if (!nextRecordField->group())
        {
            wizardPageStackOut->append(QVariantMap {{ "recordUid", recordUid }, { "fieldUid", nextFieldUid }});
            return true;
        }

        // Group record.
        auto nextRecordUid = m_sighting->getRecord(recordUid)->getFieldValue(nextRecordField->uid()).toStringList().first();

        // Group record with field-list.
        if (!nextRecordField->wizardFieldList())
        {
            return advance(nextRecordUid, "", wizardPageStackOut, errorOut, lastPageOut);
        }

        wizardPageStackOut->append(QVariantMap {{ "recordUid", nextRecordUid }, { "fieldUid", "" }});

        return true;
    }

    // No next field: figure out where to go.
    if (nextFieldUid.isEmpty())
    {
        // If at the root record, then we have reached the last relevant field.
        if (recordUid == m_sighting->wizardRecordUid())
        {
            *lastPageOut = true;
            return false;
        }

        // For group record fields, we need to fall through to the next available field.
        if (m_sighting->getRecord(recordUid)->recordField()->group())
        {
            auto nextRecordUid = m_sighting->getRecord(recordUid)->parentRecordUid();
            auto nextFieldUid = recordField->uid();

            return advance(nextRecordUid, nextFieldUid, wizardPageStackOut, errorOut, lastPageOut);
        }

        // Fell off the end, so pop back to parent record.
        reverse(wizardPageStackOut, [&](const QString& rUid, const QString& /*fUid*/) -> bool
        {
            return rUid != recordUid;
        });

        return true;
    }

    return true;
}

void Wizard::reverse(QVariantList* wizardPageStackOut, const std::function<bool(const QString& recordUid, const QString& fieldUid)>& done)
{
    while (wizardPageStackOut->count() > 0)
    {
        auto lastPageParams = wizardPageStackOut->last().toMap();

        if (done(lastPageParams.value("recordUid").toString(), lastPageParams.value("fieldUid").toString()))
        {
            break;
        }

        wizardPageStackOut->removeLast();
    }
}

void Wizard::edit(const QString &recordUid, const QString& fieldUid, int transition)
{
    // Drill down if not in record view.
    auto recordField = m_sighting->getRecord(recordUid)->recordField();
    if (fieldUid.isEmpty() && !recordField->wizardFieldList())
    {
        next(recordUid, "", transition);
        return;
    }

    // Record view.
    auto updateButtonsOnLeave = qScopeGuard([this] { updateButtons(); });

    auto wizardPageStack = m_sighting->wizardPageStack();
    wizardPageStack.append(QVariantMap {{ "recordUid", recordUid }, { "fieldUid", fieldUid }});
    m_sighting->set_wizardPageStack(wizardPageStack);

    form(this)->replaceLastPage(pageUrl(), wizardPageStack.constLast().toMap(), transition);
}

void Wizard::next(const QString& recordUid, const QString& fieldUid, int transition)
{
    auto updateButtonsOnLeave = qScopeGuard([this] { updateButtons(); });

    auto error = QString();
    auto lastPage = false;
    auto wizardPageStack = m_sighting->wizardPageStack();
    auto wizardPageStackEmpty = wizardPageStack.isEmpty();

    if (!advance(recordUid, fieldUid, &wizardPageStack, &error, &lastPage))
    {
        if (!error.isEmpty())
        {
            emit form(this)->highlightInvalid();
            emit App::instance()->showError(error);
        }

        return;
    }

    m_sighting->set_wizardPageStack(wizardPageStack);
    auto nextPageParams = wizardPageStack.constLast().toMap();
    if (wizardPageStackEmpty)
    {
        form(this)->pushPage(pageUrl(), nextPageParams, transition);
    }
    else
    {
        form(this)->replaceLastPage(pageUrl(), nextPageParams, transition);
    }
}

void Wizard::home(int transition)
{
    qFatalIf(m_sighting->wizardPageStack().isEmpty(), "Bad wizard page stack on Home");

    form(this)->popPage(transition);
    form(this)->saveSighting();
}

void Wizard::skip(const QString& targetFieldUid, int transition)
{
    auto updateButtonsOnLeave = qScopeGuard([this] { updateButtons(); });

    auto wizardPageStack = m_sighting->wizardPageStack();
    auto pageParams = wizardPageStack.constLast().toMap();
    auto error = QString();
    auto lastPage = false;

    bool pageChange = false;

    while (advance(pageParams["recordUid"].toString(), pageParams["fieldUid"].toString(), &wizardPageStack, &error, &lastPage))
    {
        pageChange = true;
        pageParams = wizardPageStack.constLast().toMap();
        if (pageParams["fieldUid"] == targetFieldUid)
        {
            // Target field found, so fix the stack to jump there directly.
            wizardPageStack = m_sighting->wizardPageStack();
            wizardPageStack.append(pageParams);
            m_sighting->set_wizardPageStack(wizardPageStack);
            form(this)->replaceLastPage(pageUrl(), pageParams, transition);
            return;
        }
    }

    if (pageChange) // Page changed, so fix the stack to jump directly as far as we got.
    {
        wizardPageStack = m_sighting->wizardPageStack();
        wizardPageStack.append(pageParams);
        m_sighting->set_wizardPageStack(wizardPageStack);
        form(this)->replaceLastPage(pageUrl(), pageParams, transition);
    }
    else if (!error.isEmpty()) // Page did not change, so this is a standard validation error.
    {
        emit form(this)->highlightInvalid();
        emit App::instance()->showError(error);
    }
}

QVariantList Wizard::findSaveTargets()
{
    auto wizardPageStack = m_sighting->wizardPageStack();
    auto result = QVariantList();

    for (auto it = wizardPageStack.constBegin(); it != wizardPageStack.constEnd(); it++)
    {
        auto pageParams = (*it).toMap();
        auto field = fieldManager(this)->getField(pageParams["fieldUid"].toString());
        if (field->parameters().contains("saveTarget"))
        {
            auto targetElementUid = "saveTarget/" + field->parameters().value("saveTarget").toString();
            auto targetFieldUid = field->uid();
            result.append(QVariantMap {{ "elementUid", targetElementUid}, { "fieldUid", targetFieldUid }});
        }
    }

    return result;
}

bool Wizard::save(const QString& targetFieldUid, int transition)
{
    auto updateButtonsOnLeave = qScopeGuard([this] { updateButtons(); });

    // Ensure the record is valid, filtered to the subset of fields we care about.
    auto record = m_sighting->getRecord(m_sighting->wizardRecordUid());
    auto result = record->isValid(m_sighting->wizardFieldUids());
    if (!result)
    {
        emit form(this)->highlightInvalid();
        emit App::instance()->showError(tr("Form incomplete"));
        return false;
    }

    // Mark it as completed.
    form(this)->markSightingCompleted();

    // Commit the sighting.
    form(this)->saveSighting();

    // Reverse the wizard until we find the target field.
    if (!targetFieldUid.isEmpty())
    {
        auto sighting = form(this)->createSightingPtr(m_sighting->rootRecordUid());

        auto wizardPageStack = sighting->wizardPageStack();
        while (wizardPageStack.count() > 0)
        {
            auto params = wizardPageStack.constLast().toMap();

            // Reset field values.
            sighting->getRecord(params["recordUid"].toString())->resetFieldValue(params["fieldUid"].toString());

            // Remove page if not the target.
            if (params["fieldUid"] != targetFieldUid || !params["listElementUid"].toString().isEmpty())
            {
                wizardPageStack.removeLast();
                continue;
            }

            // Found: setup the new sighting.
            sighting->set_wizardPageStack(wizardPageStack);
            sighting->regenerateUids();
            m_sighting->load(sighting->save());
            m_sighting->recalculate();

            emit form(this)->sightingModified(m_sighting->rootRecordUid());

            form(this)->replaceLastPage(pageUrl(), m_sighting->wizardPageStack().constLast().toMap(), transition);

            return true;
        }
    }

    // No match: reset the wizard.
    m_sighting->set_wizardRecordUid("");
    m_sighting->set_wizardFieldUids(QStringList());
    m_sighting->set_wizardPageStack(QVariantList());
    form(this)->popPage();

    return true;
}

void Wizard::gotoField(const QString& targetFieldUid, int transition)
{
    auto updateButtonsOnLeave = qScopeGuard([this] { updateButtons(); });

    auto wizardPageStack = QVariantList();
    auto recordUid = m_sighting->wizardRecordUid();
    auto fieldUid = QString();
    auto lastPage = false;

    while (advance(recordUid, fieldUid, &wizardPageStack, nullptr, &lastPage))
    {
        auto pageParams = wizardPageStack.constLast().toMap();
        recordUid = pageParams["recordUid"].toString();
        fieldUid = pageParams["fieldUid"].toString();

        if (fieldUid == targetFieldUid || m_sighting->getRecord(recordUid)->recordFieldUid() == targetFieldUid)
        {
            // Remove the index pages.
            while (form(this)->getPageStack().constLast().toMap().value("pageUrl").toString() != pageUrl())
            {
                form(this)->popPage(0);
            }

            // Target field found, so fix the stack to jump there directly.
            m_sighting->set_wizardPageStack(wizardPageStack);

            // Replace the last wizard page.
            form(this)->replaceLastPage(pageUrl(), wizardPageStack.constLast().toMap(), transition);
            return;
        }
    }
}

void Wizard::index(const QString& recordUid, const QString& fieldUid, int transition)
{
    form(this)->pushPage(
        indexPageUrl(),
        QVariantMap {
            { "recordUid", m_sighting->wizardRecordUid() },
            { "fieldUid", fieldUid.isEmpty() ? m_sighting->getRecord(recordUid)->recordFieldUid() : fieldUid },
            { "filterFieldUids", m_sighting->wizardFieldUids() }
        }, transition);
}

QString Wizard::error(const QString& recordUid, const QString& fieldUid) const
{
    auto constraintMessage = m_sighting->getRecord(recordUid)->getFieldConstraintMessage(fieldUid);

    if (!fieldUid.isEmpty() && !m_sighting->getRecord(recordUid)->testFieldFlag(fieldUid, FieldFlag::Valid))
    {
        return constraintMessage;
    }

    if (fieldUid.isEmpty() && !m_sighting->getRecord(recordUid)->isValid() && m_sighting->getRecord(recordUid)->recordField()->wizardFieldList())
    {
        return constraintMessage;
    }

    return QString();
}

void Wizard::updateButtons()
{
    auto wizardPageStack = m_sighting->wizardPageStack();
    if (wizardPageStack.isEmpty())
    {
        return;
    }

    bool lastPage = false;
    int advanceCount = 0;
    auto pageParams = wizardPageStack.constLast().toMap();
    while (advance(pageParams["recordUid"].toString(), pageParams["fieldUid"].toString(), &wizardPageStack, nullptr, &lastPage))
    {
        advanceCount++;
        pageParams = wizardPageStack.constLast().toMap();
        if (advanceCount > 1)
        {
            break;
        }
    }

    auto canNext = advanceCount > 0;
    auto canSave = lastPage;
    auto canSkip = (advanceCount > 1) || (advanceCount == 1 && canSave);

    update_canNext(canNext);
    update_canSave(canSave);
    update_canSkip(canSkip);
    update_lastPage(lastPage && advanceCount == 0);
}

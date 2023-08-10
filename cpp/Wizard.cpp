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

void Wizard::init(const QString& recordUid)
{
    m_sighting->set_wizardRecordUid(recordUid);
    m_sighting->set_wizardPageStack(QVariantList());
    first(recordUid, TRANSITION_NO_REBUILD);
}

void Wizard::reset()
{
    m_sighting->set_wizardPageStack(QVariantList());
    m_sighting->set_wizardRecordUid("");
    m_sighting->set_wizardRules(QVariantMap());
    form(this)->saveState();
}

QString Wizard::pageUrl()
{
    return "qrc:/imports/CyberTracker.1/WizardPage.qml";
}

QString Wizard::indexPageUrl()
{
    return "qrc:/imports/CyberTracker.1/WizardIndexPage.qml";
}

QString Wizard::optionsPageUrl()
{
    return "qrc:/imports/CyberTracker.1/WizardOptionsPage.qml";
}

QString Wizard::recordUid() const
{
    return m_sighting->wizardRecordUid();
}

QVariantMap Wizard::rules() const
{
    return m_sighting->wizardRules();
}

QStringList Wizard::fieldUids() const
{
    return rules().value("fieldUids").toStringList();
}

bool Wizard::valid() const
{
    return m_sighting->getRecord(recordUid())->isValid(fieldUids());
}

bool Wizard::autoSave() const
{
    return !immersive() && form(this)->project()->wizardAutosave();
}

bool Wizard::immersive() const
{
    return form(this)->project()->immersive();
}

int Wizard::popCount() const
{
    return rules().value("popCount", -1).toInt();
}

QString Wizard::header() const
{
    return rules().value("header").toString();
}

QString Wizard::banner() const
{
    return rules().value("banner").toString();
}

QVariantMap Wizard::lastPageParams() const
{
    return m_sighting->wizardPageStack().isEmpty() ? QVariantMap() : m_sighting->wizardPageStack().constLast().toMap();
}

QString Wizard::lastPageRecordUid() const
{
    return lastPageParams().value("recordUid").toString();
}

QString Wizard::lastPageFieldUid() const
{
    return lastPageParams().value("fieldUid").toString();
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
        form(this)->popPage();

        // Save the sighting if autoSave.
        if (autoSave())
        {
            form(this)->saveSighting();
        }

        return;
    }

    // Update the existing page.
    emit rebuildPage(transition);
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
            fieldUids = Wizard::fieldUids();
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

        // Skip snapLocationField.
        if (qobject_cast<LocationField*>(nextField))
        {
            if (m_sighting->getSnapLocationFieldUid() == nextFieldUid)
            {
                return advance(recordUid, nextFieldUid, wizardPageStackOut, errorOut, lastPageOut);
            }
        }

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

        // Group record without field-list.
        if (!nextRecordField->wizardFieldList())
        {
            // Ignore error, because the whole record may not be valid yet.
            return advance(nextRecordUid, "", wizardPageStackOut, nullptr/*errorOut*/, lastPageOut);
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
    auto updateButtonsOnLeave = qScopeGuard([this] { updateButtons(); });
    auto wizardPageStack = m_sighting->wizardPageStack();

    auto recordField = m_sighting->getRecord(recordUid)->recordField();
    if (fieldUid.isEmpty() && !recordField->wizardFieldList())
    {
        auto lastPage = false;
        if (!advance(recordUid, "", &wizardPageStack, nullptr, &lastPage))
        {
            return;
        }
    }
    else
    {
        wizardPageStack.append(QVariantMap {{ "recordUid", recordUid }, { "fieldUid", fieldUid }});
    }

    m_sighting->set_wizardPageStack(wizardPageStack);

    emit rebuildPage(transition);
}

void Wizard::next(const QString& recordUid, const QString& fieldUid, int transition)
{
    auto updateButtonsOnLeave = qScopeGuard([this] { updateButtons(); });

    auto error = QString();
    auto lastPage = false;
    auto wizardPageStack = m_sighting->wizardPageStack();

    if (!advance(recordUid, fieldUid, &wizardPageStack, &error, &lastPage))
    {
        if (!error.isEmpty())
        {
            emit App::instance()->showError(error);

            // Do this after the showError, as it may trump with a better error message.
            emit form(this)->highlightInvalid();
        }

        return;
    }

    m_sighting->set_wizardPageStack(wizardPageStack);

    emit rebuildPage(transition);
}

void Wizard::home(int /*transition*/)
{
    qFatalIf(m_sighting->wizardPageStack().isEmpty(), "Bad wizard page stack on Home");

    auto form = ::form(this);

    // Immersive.
    if (immersive())
    {
        emit App::instance()->popPageStack();
        return;
    }

    // Default.
    form->popPages(popCount());

    if (autoSave())
    {
        form->saveSighting();
    }

    reset();
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

            emit rebuildPage(transition);
            return;
        }
    }

    if (pageChange) // Page changed, so fix the stack to jump directly as far as we got.
    {
        wizardPageStack = m_sighting->wizardPageStack();
        wizardPageStack.append(pageParams);
        m_sighting->set_wizardPageStack(wizardPageStack);

        emit rebuildPage(transition);
    }
    else if (!error.isEmpty()) // Page did not change, so this is a standard validation error.
    {
        emit form(this)->highlightInvalid();
        emit App::instance()->showError(error);
    }
}

bool Wizard::save(const QString& targetFieldUid, int transition)
{
    auto updateButtonsOnLeave = qScopeGuard([this] { updateButtons(); });

    auto form = ::form(this);

    // Ensure the sighting is valid.
    if (!valid())
    {
        emit form->highlightInvalid();
        emit App::instance()->showError(tr("Form incomplete"));
        return false;
    }

    // Complete editing by popping off the top-most wizard form.
    if (form->editing())
    {
        qFatalIf(!immersive(), "Editing requires immersive");
        reset();
        form->saveSighting();
        form->popPagesToParent();
        return true;
    }

    auto sightingData = m_sighting->save();

    // Save and mark completed.
    form->saveSighting();
    form->markSightingCompleted();

    //
    // Prepare the next sighting.
    //

    // Reverse the stack until we find the target.
    auto sighting = form->createSightingPtr(sightingData);
    auto wizardPageStack = sighting->wizardPageStack();
    auto foundTargetFieldUid = false;

    auto pageRecordUid = QString();
    auto pageFieldUid = QString();

    for (;;)
    {
        auto pageParams = wizardPageStack.constLast().toMap();
        pageRecordUid = pageParams["recordUid"].toString();
        pageFieldUid = pageParams["fieldUid"].toString();

        // An empty fieldUid means the page represents the full record, i.e. a field-list. Convert to record + field.
        if (pageFieldUid.isEmpty())
        {
            auto record = sighting->getRecord(pageRecordUid);
            pageRecordUid = record->parentRecordUid();
            pageFieldUid = record->recordFieldUid();
        }

        // Check for the first page.
        if (wizardPageStack.count() == 1)
        {
            break;
        }

        // Look for target or sub-list element.
        if (Utils::compareLastPathComponents(pageFieldUid, targetFieldUid) && pageParams["listElementUid"].toString().isEmpty())
        {
            foundTargetFieldUid = true;
            break;
        }

        // Remove page.
        wizardPageStack.removeLast();
    }

    // Reset all the fields after the target.
    if (!pageFieldUid.isEmpty())
    {
        auto fieldManager = ::fieldManager(this);
        auto rootField = fieldManager->rootField();
        auto field = fieldManager->getField(pageFieldUid);
        while (field->parentField() != rootField)
        {
            field = field->parentField();
        }

        auto fieldIndex = rootField->fieldIndex(field);
        for (auto i = fieldIndex; i < rootField->fieldCount(); i++)
        {
            sighting->resetRootFieldValue(rootField->field(i)->uid());
        }
    }

    // Reset the snap location field if provided.
    auto snapLocationFieldUid = sighting->getSnapLocationFieldUid();
    if (!snapLocationFieldUid.isEmpty())
    {
        sighting->resetFieldValue(sighting->rootRecordUid(), snapLocationFieldUid);
    }

    // Reset the track file field if provided.
    auto trackFileFieldUid = sighting->getTrackFileFieldUid();
    if (!trackFileFieldUid.isEmpty())
    {
        sighting->resetFieldValue(sighting->rootRecordUid(), trackFileFieldUid);
    }

    // Found target, so stay in wizard.
    if (foundTargetFieldUid || immersive())
    {
        sighting->set_wizardPageStack(wizardPageStack);
        sighting->regenerateUids();
        m_sighting->load(sighting->save());
        m_sighting->recalculate();

        emit form->sightingModified(m_sighting->rootRecordUid());
        emit rebuildPage(transition);
        return true;
    }

    // No target, non-immersive.
    form->popPages(popCount());
    emit form->sightingModified(m_sighting->rootRecordUid());

    return true;
}

void Wizard::gotoField(const QString& targetFieldUid, int transition)
{
    auto updateButtonsOnLeave = qScopeGuard([this] { updateButtons(); });

    auto wizardPageStack = QVariantList();
    auto recordUid = m_sighting->wizardRecordUid();
    auto fieldUid = QString();
    auto lastPage = false;
    auto form = ::form(this);

    while (advance(recordUid, fieldUid, &wizardPageStack, nullptr, &lastPage))
    {
        auto pageParams = wizardPageStack.constLast().toMap();
        recordUid = pageParams["recordUid"].toString();
        fieldUid = pageParams["fieldUid"].toString();

        // Skip if the target is not reached.
        if (fieldUid != targetFieldUid && m_sighting->getRecord(recordUid)->recordFieldUid() != targetFieldUid)
        {
            continue;
        }

        // Count the index and option pages.
        auto pageCount = 0;
        auto pageStack = form->getPageStack();

        for (auto i = pageStack.length() - 1; i >= 0; i--)
        {
            auto lastPageUrl = pageStack[i].toMap().value("pageUrl").toString();
            if (lastPageUrl == pageUrl() || lastPageUrl == indexPageUrl() || lastPageUrl == optionsPageUrl())
            {
                pageCount++;
                continue;
            }

            break;
        }

        // Pop excess pages.
        if (pageCount > 1)
        {
            form->popPage(pageCount - 1);
        }

        // Go to the target wizard page.
        m_sighting->set_wizardPageStack(wizardPageStack);
        form->replaceLastPage(pageUrl(), QVariantMap(), transition);

        return;
    }
}

void Wizard::index(const QString& recordUid, const QString& fieldUid, bool highlightInvalid, int transition)
{
    form(this)->pushPage(
        indexPageUrl(),
        QVariantMap {
            { "recordUid", m_sighting->wizardRecordUid() },
            { "fieldUid", fieldUid.isEmpty() ? m_sighting->getRecord(recordUid)->recordFieldUid() : fieldUid },
            { "filterFieldUids", fieldUids() },
            { "highlightInvalid", highlightInvalid }
        }, transition);
}

void Wizard::options(const QString& recordUid, const QString& fieldUid, int transition)
{
    form(this)->pushPage(
        optionsPageUrl(),
        QVariantMap {
            { "recordUid", m_sighting->wizardRecordUid() },
            { "fieldUid", fieldUid.isEmpty() ? m_sighting->getRecord(recordUid)->recordFieldUid() : fieldUid },
            { "filterFieldUids", fieldUids() }
        }, transition);
}

QString Wizard::error(const QString& recordUid, const QString& fieldUid) const
{
    auto finalRecordUid = recordUid;
    auto finalFieldUid = fieldUid;
    auto recordValid = true;
    auto fieldValid = true;

    // Empty field means use the recordUid only.
    if (fieldUid.isEmpty())
    {
        // Skip root record.
        if (recordUid == m_sighting->wizardRecordUid())
        {
            return QString();
        }

        // Pick up the group.
        auto record = m_sighting->getRecord(recordUid);
        finalRecordUid = record->parentRecordUid();
        finalFieldUid = record->recordFieldUid();
        recordValid = record->isValid();
    }
    else  // Validate child records.
    {
        auto field = fieldManager(this)->getField(fieldUid);
        if (field->isRecordField())
        {
            auto records = m_sighting->getFieldValue(recordUid, fieldUid, QStringList()).toStringList();
            for (auto recordIt = records.constBegin(); recordIt != records.constEnd(); recordIt++)
            {
                recordValid = m_sighting->getRecord(*recordIt)->isValid();
                if (!recordValid)
                {
                    break;
                }
            }
        }
    }

    auto record = m_sighting->getRecord(finalRecordUid);
    fieldValid = record->testFieldFlag(finalFieldUid, FieldFlag::Valid);

    // No error for valid fields.
    if (recordValid && fieldValid)
    {
        return QString();
    }

    // If constraint is not met -> show constraint message.
    if (!recordValid || !record->testFieldFlag(finalFieldUid, FieldFlag::Constraint))
    {
        return record->getFieldConstraintMessage(finalFieldUid);
    }

    // If required -> show the required message.
    if (!recordValid || record->testFieldFlag(finalFieldUid, FieldFlag::Required))
    {
        return record->getFieldRequiredMessage(finalFieldUid);
    }

    // This is reached when the field validation (e.g. NumberField) says the value is invalid, but no constraint has been set.
    return record->getFieldConstraintMessage(finalFieldUid);
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

        if (wizardPageStack.empty())
        {
            // Scenarios where the definition was changed.
            break;
        }

        pageParams = wizardPageStack.constLast().toMap();
        if (advanceCount > 1)
        {
            break;
        }
    }

    auto canNext = advanceCount > 0;
    auto canSave = (advanceCount == 0) && lastPage;
    auto canSkip = (advanceCount > 1) || (advanceCount == 1 && canSave);

    update_canBack(m_sighting->wizardPageStack().count() > 1);
    update_canNext(canNext);
    update_canSave(canSave);
    update_canSkip(canSkip);
    update_lastPage(lastPage && advanceCount == 0);
}

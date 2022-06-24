#include "FieldBinding.h"

FieldBinding::FieldBinding(QObject* parent): QObject(parent)
{
    m_complete = false;
}

FieldBinding::FieldBinding(QObject* parent, const QString& recordUid, const QString& fieldUid): QObject(parent)
{
    m_complete = false;
    m_recordUid = recordUid;
    m_fieldUid = fieldUid;
}

FieldBinding::~FieldBinding()
{
    reset();
}

void FieldBinding::reset()
{
    m_form = nullptr;
    m_recordUid = "";
    m_fieldUid = "";
}

void FieldBinding::classBegin()
{
}

void FieldBinding::componentComplete()
{
    connect(this, &FieldBinding::recordUidChanged, this, &FieldBinding::ready);
    connect(this, &FieldBinding::fieldUidChanged, this, &FieldBinding::ready);

    m_form = Form::parentForm(this);
    connect(m_form, &Form::fieldValueChanged, this, &FieldBinding::onFieldValueChanged);
    connect(m_form, &Form::recordChildValueChanged, this, &FieldBinding::onRecordChildValueChanged);

    update_complete(true);

    if (m_usedBeforeComplete)
    {
        emit changed();
    }
}

bool FieldBinding::ready()
{
    if (m_fieldUid.isEmpty())
    {
        m_usedBeforeComplete = m_usedBeforeComplete || !m_complete;
        return false;
    }

    if (!m_form)
    {
        m_form = Form::parentForm(this);
    }

    if (!m_form || !m_form->connected())
    {
        m_usedBeforeComplete = m_usedBeforeComplete || !m_complete;
        return false;
    }

    if (m_recordUid.isEmpty())
    {
        m_recordUid = m_form->rootRecordUid();
    }

    if (!m_form->hasRecord(m_recordUid))
    {
        reset();
        m_usedBeforeComplete = m_usedBeforeComplete || !m_complete;
        return false;
    }

    return true;
}

BaseField* FieldBinding::field()
{
    if (!ready())
    {
        return nullptr;
    }

    return m_form->getField(m_fieldUid);
}

QVariant FieldBinding::fieldAsVariant()
{
    if (!ready())
    {
        return QVariant();
    }

    return QVariant::fromValue<BaseField*>(field());
}

QString FieldBinding::fieldType()
{
    if (!ready())
    {
        return QString();
    }

    return m_form->getFieldType(m_fieldUid);
}

bool FieldBinding::fieldIsList()
{
    if (!ready())
    {
        return false;
    }

    return !field()->listElementUid().isEmpty();
}

bool FieldBinding::visible()
{
    if (!ready())
    {
        return false;
    }

    return m_form->getFieldVisible(m_recordUid, m_fieldUid);

}

QVariant FieldBinding::value()
{
    return getValue(QVariant());
}

bool FieldBinding::isEmpty()
{
    if (!ready())
    {
        return true;
    }

    auto v = value();

    return v.toMap().isEmpty() && 
           v.toList().isEmpty() &&
           v.toString().isEmpty() && 
           field()->toDisplayValue(m_form->elementManager(), v).isEmpty();
}

bool FieldBinding::isValid()
{
    if (!ready())
    {
        return true;
    }

    return m_form->getFieldValueValid(m_recordUid, m_fieldUid);
}

bool FieldBinding::isCalculated()
{
    if (!ready())
    {
        return false;
    }

    return m_form->getFieldValueCalculated(m_recordUid, m_fieldUid);
}

bool FieldBinding::required()
{
    if (!ready())
    {
        return true;
    }

    return m_form->getFieldRequired(m_recordUid, m_fieldUid);
}

QVariant FieldBinding::getValue(const QVariant& defaultValue)
{
    if (!ready())
    {
        return defaultValue;
    }

    return m_form->getFieldValue(m_recordUid, m_fieldUid, defaultValue);
}

void FieldBinding::setValue(const QVariant& value)
{
    if (!ready())
    {
        return;
    }

    m_form->setFieldValue(m_recordUid, m_fieldUid, value);
}

void FieldBinding::resetValue()
{
    if (!ready())
    {
        return;
    }

    m_form->resetFieldValue(m_recordUid, m_fieldUid);
}

QVariant FieldBinding::getControlState(const QString& name)
{
    return getControlState(name, QVariant());
}

QVariant FieldBinding::getControlState(const QString& name, const QVariant& defaultValue)
{
    if (!ready())
    {
        return QVariant();
    }

    return m_form->getControlState(m_recordUid, m_fieldUid, name, defaultValue);
}

void FieldBinding::setControlState(const QString& name, const QVariant& value)
{
    if (!ready())
    {
        return;
    }

    m_form->setControlState(m_recordUid, m_fieldUid, name, value);
}

QString FieldBinding::fieldName()
{
    if (!ready())
    {
        return QString();
    }

    return m_form->getFieldName(m_recordUid, m_fieldUid);
}

QUrl FieldBinding::fieldIcon()
{
    if (!ready())
    {
        return QUrl();
    }

    return m_form->getFieldIcon(m_fieldUid);
}

QString FieldBinding::getValueElementName(const QVariant& defaultValue)
{
    if (!ready())
    {
        return QString();
    }

    return m_form->getElementName(getValue(defaultValue).toString());
}

QString FieldBinding::valueElementName()
{
    if (!ready())
    {
        return QString();
    }

    return getValueElementName("");
}

QUrl FieldBinding::valueElementIcon()
{
    if (!ready())
    {
        return QUrl();
    }

    if (!qobject_cast<StringField*>(field()))
    {
        return QUrl();
    }

    if (field()->listElementUid().isEmpty())
    {
        return QUrl();
    }

    auto value = getValue("").toString();
    return value.isEmpty() ? QUrl() : m_form->getElementIcon(value, true);
}

QString FieldBinding::displayValue()
{
    if (!ready())
    {
        return QString();
    }

    return m_form->getFieldDisplayValue(m_recordUid, m_fieldUid);
}

QString FieldBinding::hint()
{
    if (!ready())
    {
        return QString();
    }

    return m_form->getFieldHint(m_recordUid, m_fieldUid);
}

QUrl FieldBinding::hintIcon()
{
    if (!ready())
    {
        return QString();
    }

    return m_form->getFieldHintIcon(m_fieldUid);
}

QUrl FieldBinding::hintLink()
{
    if (!ready())
    {
        return QUrl();
    }

    return m_form->getFieldHintLink(m_fieldUid);
}

QString FieldBinding::sectionName()
{
    if (!ready())
    {
        return QString();
    }

    auto sectionElementUid = field()->sectionElementUid();
    if (sectionElementUid.isEmpty())
    {
        return QString();
    }

    return m_form->getElementName(sectionElementUid);
}

QUrl FieldBinding::sectionIcon()
{
    if (!ready())
    {
        return QUrl();
    }

    auto sectionElementUid = field()->sectionElementUid();
    if (sectionElementUid.isEmpty())
    {
        return QUrl();
    }

    return m_form->getElementIcon(sectionElementUid, true);
}

void FieldBinding::onFieldValueChanged(const QString& recordUid, const QString& fieldUid, const QVariant& /*oldValue*/, const QVariant& /*newValue*/)
{
    if (!ready())
    {
        return;
    }

    if (m_recordUid != recordUid)
    {
        return;
    }

    if (m_fieldUid == fieldUid)
    {
        emit changed();
        return;
    }

    // Trigger re-evaluating the required field. 
    if (field()->required().userType() == QMetaType::QString)
    {
        emit changed();
        return;
    }
}

void FieldBinding::onRecordChildValueChanged(const QString& recordUid, const QString& fieldUid)
{
    if (!ready())
    {
        return;
    }

    if (recordUid == m_recordUid && fieldUid == fieldUid)
    {
        emit changed();
    }
}


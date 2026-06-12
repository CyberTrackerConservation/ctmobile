#pragma once
#include "pch.h"
#include "Fields.h"
#include "Form.h"

namespace CyberTracker {

class FieldBinding: public QObject, public QQmlParserStatus
{
    Q_OBJECT

    Q_INTERFACES (QQmlParserStatus)

    QML_WRITABLE_AUTO_PROPERTY (QString, recordUid)
    QML_WRITABLE_AUTO_PROPERTY (QString, fieldUid)

    QML_READONLY_AUTO_PROPERTY (bool, complete)

    Q_PROPERTY (QVariant field READ fieldAsVariant NOTIFY changed) // Variant so QML can use field specific properties dynamically
    Q_PROPERTY (QString fieldType READ fieldType NOTIFY changed)
    Q_PROPERTY (bool fieldIsList READ fieldIsList NOTIFY changed)
    Q_PROPERTY (QString fieldName READ fieldName NOTIFY changed)
    Q_PROPERTY (QString fieldIcon READ fieldIcon NOTIFY changed)
    Q_PROPERTY (QVariant fieldDefaultValue READ fieldDefaultValue NOTIFY changed)
    Q_PROPERTY (QString sectionName READ sectionName NOTIFY changed)
    Q_PROPERTY (QString sectionIcon READ sectionIcon NOTIFY changed)
    Q_PROPERTY (bool required READ required NOTIFY changed)

    Q_PROPERTY (QVariant value READ value NOTIFY changed)
    Q_PROPERTY (bool isEmpty READ isEmpty NOTIFY changed)
    Q_PROPERTY (bool isValid READ isValid NOTIFY changed)
    Q_PROPERTY (bool isCalculated READ isCalculated NOTIFY changed)
    Q_PROPERTY (QString valueElementName READ valueElementName NOTIFY changed)
    Q_PROPERTY (QString valueElementIcon READ valueElementIcon NOTIFY changed)
    Q_PROPERTY (QString displayValue READ displayValue NOTIFY changed)
    Q_PROPERTY (QString hint READ hint NOTIFY changed)
    Q_PROPERTY (QString hintIcon READ hintIcon NOTIFY changed)
    Q_PROPERTY (QString hintLink READ hintLink NOTIFY changed)

    Q_PROPERTY (bool visible READ visible NOTIFY changed)

public:
    FieldBinding(QObject* parent = nullptr);
    FieldBinding(QObject* parent, const QString& recordUid, const QString& fieldUid);
    virtual ~FieldBinding();
    Q_INVOKABLE void reset();

    void classBegin() override;
    void componentComplete() override;

    Q_INVOKABLE QVariant getValue(const QVariant& defaultValue = QVariant());
    Q_INVOKABLE void setValue(const QVariant& value);
    Q_INVOKABLE void resetValue();
    Q_INVOKABLE void sendValueChange();

    Q_INVOKABLE QString getValueElementName(const QVariant& defaultValue = QVariant());

    Q_INVOKABLE QVariant getControlState(const QString& name);
    Q_INVOKABLE QVariant getControlState(const QString& name, const QVariant& defaultValue);
    Q_INVOKABLE void setControlState(const QString& name, const QVariant& value);

    BaseField* field();
    QString fieldType();
    QVariant fieldAsVariant();
    bool fieldIsList();
    QString fieldName();
    QString fieldIcon();
    QVariant fieldDefaultValue();
    QString sectionName();
    QString sectionIcon();
    bool required();
    QVariant value();
    bool isEmpty();
    bool isValid();
    bool isCalculated();
    QString valueElementName();
    QString valueElementIcon();
    QString displayValue();
    QString hint();
    QString hintIcon();
    QString hintLink();
    bool visible();

signals:
    void changed();

private:
    Form* m_form = nullptr;
    bool m_usedBeforeComplete = false;
    bool ready();

private slots:
    void onFieldValueChanged(const QString& recordUid, const QString& fieldUid, const QVariant& oldValue, const QVariant& newValue);
    void onRecordChildValueChanged(const QString& recordUid, const QString& fieldUid);
};

} // namespace CyberTracker
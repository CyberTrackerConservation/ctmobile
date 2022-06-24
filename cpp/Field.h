#pragma once
#include "pch.h"
#include "Element.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BaseField

class BaseField: public QObject
{
    Q_OBJECT

    QML_WRITABLE_AUTO_PROPERTY (QString, uid)
    QML_WRITABLE_AUTO_PROPERTY (QString, exportUid)
    QML_WRITABLE_AUTO_PROPERTY (QString, nameElementUid)
    QML_WRITABLE_AUTO_PROPERTY (QString, hintElementUid)
    QML_WRITABLE_AUTO_PROPERTY (QString, hintLink)
    QML_WRITABLE_AUTO_PROPERTY (QString, sectionElementUid)
    QML_WRITABLE_AUTO_PROPERTY (QString, listElementUid)
    QML_WRITABLE_AUTO_PROPERTY (QString, listFilterByField)
    QML_WRITABLE_AUTO_PROPERTY (QString, listFilterByTag)
    QML_WRITABLE_AUTO_PROPERTY (bool, listFlatten)
    QML_WRITABLE_AUTO_PROPERTY (bool, listSorted)
    QML_WRITABLE_AUTO_PROPERTY (bool, listOther)
    QML_WRITABLE_AUTO_PROPERTY (QVariantMap, tag)
    QML_WRITABLE_AUTO_PROPERTY (QVariant, defaultValue)
    QML_WRITABLE_AUTO_PROPERTY (QString, color)  
    QML_WRITABLE_AUTO_PROPERTY (bool, hidden)
    QML_WRITABLE_AUTO_PROPERTY (bool, readonly)
    QML_WRITABLE_AUTO_PROPERTY (QVariant, required)
    QML_WRITABLE_AUTO_PROPERTY (QString, relevant)
    QML_WRITABLE_AUTO_PROPERTY (QString, constraint)
    QML_WRITABLE_AUTO_PROPERTY (QString, constraintElementUid)
    QML_WRITABLE_AUTO_PROPERTY (QString, formula)
    QML_WRITABLE_AUTO_PROPERTY (QVariantMap, parameters)
    QML_WRITABLE_AUTO_PROPERTY (QVariant, appearance) // deprecated.
    Q_PROPERTY (bool isRecordField READ isRecordField CONSTANT)

public:
    explicit BaseField(QObject* parent = nullptr);
    virtual ~BaseField();

    virtual BaseField* parentField() const;
    virtual bool isRecordField() const;

    virtual void addTag(const QString& key, const QVariant& value);
    virtual void removeTag(const QString& key);
    virtual QVariant getTagValue(const QString& key, const QVariant& defaultValue = QVariant()) const;

    virtual QVariant getParameter(const QString& key, const QVariant& defaultValue = QVariant()) const;
    virtual void addParameter(const QString& key, const QVariant& value);

    virtual void appendAttachments(const QVariant& value, QStringList* attachmentsOut) const;

    virtual QString toXml(const QVariant& value) const;
    virtual QVariant fromXml(const QVariant& value) const;

    virtual QVariant save(const QVariant& value) const;
    virtual QVariant load(const QVariant& value) const;

    virtual void toQml(QTextStream& stream, int depth) const;
    virtual QString toDisplayValue(ElementManager* elementManager, const QVariant& value) const;

    virtual bool isValid(const QVariant& value, bool required) const;

    virtual QString safeExportUid() const;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RecordField

class RecordField: public BaseField
{
    Q_OBJECT

    Q_PROPERTY (QQmlListProperty<BaseField> fields READ fields CONSTANT)
    Q_CLASSINFO ("DefaultProperty", "fields")
    QML_WRITABLE_AUTO_PROPERTY (QString, masterFieldUid)
    QML_WRITABLE_AUTO_PROPERTY (int, matrixCount)
    QML_WRITABLE_AUTO_PROPERTY (int, minCount)
    QML_WRITABLE_AUTO_PROPERTY (int, maxCount)
    QML_WRITABLE_AUTO_PROPERTY (QString, version)
    QML_WRITABLE_AUTO_PROPERTY (QString, repeatCount)
    Q_PROPERTY (bool group READ group CONSTANT)
    Q_PROPERTY (bool dynamic READ dynamic CONSTANT)
    Q_PROPERTY (bool wizardFieldList READ wizardFieldList CONSTANT)

public:
    explicit RecordField(QObject* parent = nullptr);
    ~RecordField() override;

    bool isRecordField() const override;
    static const RecordField* fromField(const BaseField* field);

    QQmlListProperty<BaseField> fields();
    int fieldCount() const;
    BaseField* field(int) const;
    void enumFields(const std::function<void(BaseField* field)>& callback) const;
    void insertField(int index, BaseField* field);
    void appendField(BaseField* field);
    void detachField(BaseField* field);
    void removeFirstField();
    void removeLastField();
    void clearFields();
    bool group() const;
    bool dynamic() const;
    bool wizardFieldList() const;

    int fieldIndex(BaseField* field) const;

    QVariant save(const QVariant& value) const override;
    QVariant load(const QVariant& value) const override;
    void toQml(QTextStream& stream, int depth) const override;
    QString toDisplayValue(ElementManager* elementManager, const QVariant& value) const override;

private:
    QList<BaseField*> m_fields;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StaticField

class StaticField: public BaseField
{
    Q_OBJECT

public:
    explicit StaticField(QObject* parent = nullptr);

    void toQml(QTextStream& stream, int depth) const;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StringField

class StringField: public BaseField
{
    Q_OBJECT

    QML_WRITABLE_AUTO_PROPERTY (QString, inputMask)
    QML_WRITABLE_AUTO_PROPERTY (bool, multiLine)
    QML_WRITABLE_AUTO_PROPERTY (int, maxLength)
    QML_WRITABLE_AUTO_PROPERTY (bool, barcode)
    QML_WRITABLE_AUTO_PROPERTY (bool, numbers)

public:
    explicit StringField(QObject* parent = nullptr);

    QVariant save(const QVariant& value) const override;
    QVariant load(const QVariant& value) const override;

    void toQml(QTextStream& stream, int depth) const override;
    QString toDisplayValue(ElementManager* elementManager, const QVariant& value) const override;

    QString toXml(const QVariant& value) const override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NumberField

class NumberField: public BaseField
{
    Q_OBJECT

    QML_WRITABLE_AUTO_PROPERTY (QString, inputMask)
    QML_WRITABLE_DOUBLE_PROPERTY (minValue)
    QML_WRITABLE_DOUBLE_PROPERTY (maxValue)
    QML_WRITABLE_AUTO_PROPERTY (int, decimals)
    QML_WRITABLE_DOUBLE_PROPERTY (step)

public:
    explicit NumberField(QObject* parent = nullptr);

    QVariant save(const QVariant& value) const override;
    QVariant load(const QVariant& value) const override;
    void toQml(QTextStream& stream, int depth) const override;
    QString toDisplayValue(ElementManager* elementManager, const QVariant& value) const override;

    bool isValid(const QVariant& value, bool required) const override;

    // For number lists so the UI can validate an individual number.
    Q_INVOKABLE bool checkSingleValue(const QVariant& value) const;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CheckField

class CheckField: public BaseField
{
    Q_OBJECT

public:
    explicit CheckField(QObject* parent = nullptr);

    QString toXml(const QVariant& value) const override;
    QVariant save(const QVariant& value) const override;
    QVariant load(const QVariant& value) const override;
    void toQml(QTextStream& stream, int depth) const override;
    QString toDisplayValue(ElementManager* elementManager, const QVariant& value) const override;
    bool isValid(const QVariant& value, bool required) const override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AcknowledgeField

class AcknowledgeField: public BaseField
{
    Q_OBJECT

public:
    explicit AcknowledgeField(QObject* parent = nullptr);

    QString toXml(const QVariant& value) const override;
    QVariant fromXml(const QVariant& value) const override;
    QVariant save(const QVariant& value) const override;
    QVariant load(const QVariant& value) const override;
    void toQml(QTextStream& stream, int depth) const override;
    QString toDisplayValue(ElementManager* elementManager, const QVariant& value) const override;
    bool isValid(const QVariant& value, bool required) const override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LocationField

class LocationField: public BaseField
{
    Q_OBJECT

    QML_WRITABLE_AUTO_PROPERTY (int, fixCount)
    QML_WRITABLE_AUTO_PROPERTY (bool, allowManual)

public:
    explicit LocationField(QObject* parent = nullptr);

    QString toXml(const QVariant& value) const override;
    QVariant fromXml(const QVariant& value) const override;
    QVariant save(const QVariant& value) const override;
    QVariant load(const QVariant& value) const override;
    void toQml(QTextStream& stream, int depth) const override;
    QString toDisplayValue(ElementManager* elementManager, const QVariant& value) const override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LineField

class LineField: public BaseField
{
    Q_OBJECT
    QML_WRITABLE_AUTO_PROPERTY (bool, allowManual)

public:
    explicit LineField(QObject* parent = nullptr);

    QString toXml(const QVariant& value) const override;
    QVariant fromXml(const QVariant& value) const override;
    QVariant save(const QVariant& value) const override;
    QVariant load(const QVariant& value) const override;
    void toQml(QTextStream& stream, int depth) const override;
    QString toDisplayValue(ElementManager* elementManager, const QVariant& value) const override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AreaField

class AreaField: public BaseField
{
    Q_OBJECT
    QML_WRITABLE_AUTO_PROPERTY (bool, allowManual)

public:
    explicit AreaField(QObject* parent = nullptr);

    QString toXml(const QVariant& value) const override;
    QVariant fromXml(const QVariant& value) const override;
    QVariant save(const QVariant& value) const override;
    QVariant load(const QVariant& value) const override;
    void toQml(QTextStream& stream, int depth) const override;
    QString toDisplayValue(ElementManager* elementManager, const QVariant& value) const override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DateField

class DateField: public BaseField
{
    Q_OBJECT

    QML_WRITABLE_AUTO_PROPERTY (QString, minDate)
    QML_WRITABLE_AUTO_PROPERTY (QString, maxDate)

public:
    explicit DateField(QObject* parent = nullptr);

    QString toXml(const QVariant& value) const override;
    QVariant fromXml(const QVariant& value) const override;
    QVariant save(const QVariant& value) const override;
    QVariant load(const QVariant& value) const override;
    void toQml(QTextStream& stream, int depth) const override;
    QString toDisplayValue(ElementManager* elementManager, const QVariant& value) const override;
    bool isValid(const QVariant& value, bool required) const override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DateTimeField

class DateTimeField: public BaseField
{
    Q_OBJECT

    QML_WRITABLE_AUTO_PROPERTY (QString, minDate)
    QML_WRITABLE_AUTO_PROPERTY (QString, maxDate)

public:
    explicit DateTimeField(QObject* parent = nullptr);

    QVariant save(const QVariant& value) const override;
    QVariant load(const QVariant& value) const override;
    void toQml(QTextStream& stream, int depth) const override;
    QString toDisplayValue(ElementManager* elementManager, const QVariant& value) const override;
    bool isValid(const QVariant& value, bool required) const override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TimeField

class TimeField: public BaseField
{
    Q_OBJECT

public:
    explicit TimeField(QObject* parent = nullptr);

    QString toXml(const QVariant& value) const override;
    QVariant fromXml(const QVariant& value) const override;
    QVariant save(const QVariant& value) const override;
    QVariant load(const QVariant& value) const override;
    void toQml(QTextStream& stream, int depth) const override;
    QString toDisplayValue(ElementManager* elementManager, const QVariant& value) const override;
    bool isValid(const QVariant& value, bool required) const override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ListField

class ListField: public BaseField
{
    Q_OBJECT

public:
    ListField(QObject* parent = nullptr);

    QVariant save(const QVariant& value) const override;
    QVariant load(const QVariant& value) const override;
    QString toDisplayValue(ElementManager* elementManager, const QVariant& value) const override;
    void toQml(QTextStream& stream, int depth) const override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PhotoField

class PhotoField: public BaseField
{
    Q_OBJECT

    QML_WRITABLE_AUTO_PROPERTY (int, minCount)
    QML_WRITABLE_AUTO_PROPERTY (int, maxCount)
    QML_WRITABLE_AUTO_PROPERTY (int, resolutionX)
    QML_WRITABLE_AUTO_PROPERTY (int, resolutionY)

public:
    explicit PhotoField(QObject* parent = nullptr);

    void appendAttachments(const QVariant& value, QStringList* attachmentsOut) const override;
    QVariant save(const QVariant& value) const override;
    QVariant load(const QVariant& value) const override;
    QString toXml(const QVariant& value) const override;
    void toQml(QTextStream& stream, int depth) const override;
    QString toDisplayValue(ElementManager* elementManager, const QVariant& value) const override;
    bool isValid(const QVariant& value, bool required) const override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AudioField

class AudioField: public BaseField
{
    Q_OBJECT

    QML_WRITABLE_AUTO_PROPERTY (int, maxSeconds)
    QML_WRITABLE_AUTO_PROPERTY (int, sampleRate)

public:
    explicit AudioField(QObject* parent = nullptr);

    void appendAttachments(const QVariant& value, QStringList* attachmentsOut) const override;
    QVariant save(const QVariant& value) const override;
    QVariant load(const QVariant& value) const override;
    QString toXml(const QVariant& value) const override;
    void toQml(QTextStream& stream, int depth) const override;
    QString toDisplayValue(ElementManager* elementManager, const QVariant& value) const override;
    bool isValid(const QVariant& value, bool required) const override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SketchField

class SketchField: public BaseField
{
    Q_OBJECT

public:
    explicit SketchField(QObject* parent = nullptr);

    void appendAttachments(const QVariant& value, QStringList* attachmentsOut) const override;
    QVariant save(const QVariant& value) const override;
    QVariant load(const QVariant& value) const override;
    void toQml(QTextStream& stream, int depth) const override;
    QString toDisplayValue(ElementManager* elementManager, const QVariant& value) const override;
    QString toXml(const QVariant& value) const override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CalculateField

class CalculateField: public BaseField
{
    Q_OBJECT

    QML_WRITABLE_AUTO_PROPERTY (bool, number)

public:
    explicit CalculateField(QObject* parent = nullptr);

    QVariant save(const QVariant& value) const override;
    QVariant load(const QVariant& value) const override;
    void toQml(QTextStream& stream, int depth) const override;
    QString toDisplayValue(ElementManager* elementManager, const QVariant& value) const override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FieldManager

class FieldManager: public QObject
{
    Q_OBJECT
    Q_PROPERTY(RecordField* rootField READ rootField CONSTANT)

public:
    explicit FieldManager(QObject* parent = nullptr);
    ~FieldManager();

    RecordField* rootField() const;

    BaseField* getField(const QString& fieldUid) const;
    QString getFieldType(const QString& fieldUid) const;

    void appendField(RecordField* parent, BaseField* field);

    QStringList buildFieldList(const QString& recordFieldUid, const QString& filterTagName = QString(), const QVariant& filterTagValue = QVariant()) const;

    void loadFromQmlFile(const QString& filePath);
    void saveToQmlFile(const QString& filePath);

    void completeUpdate();

private:
    QString m_cacheKey;
    RecordField* m_rootField;
    QMap<QString, BaseField*> m_fieldMap;
    void populateMap(BaseField*);
};

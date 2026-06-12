#pragma once
#include "pch.h"
#include "Fields.h"
#include "Element.h"

namespace CyberTracker {

class Schema: public QObject
{
    Q_OBJECT

    Q_PROPERTY(QQmlListProperty<QObject> items READ items CONSTANT)
    Q_CLASSINFO("DefaultProperty", "items")

    QML_WRITABLE_AUTO_PROPERTY (QString, title)
    QML_WRITABLE_AUTO_PROPERTY (QString, subtitle)
    QML_WRITABLE_AUTO_PROPERTY (QString, icon)

    QML_WRITABLE_AUTO_PROPERTY (QString, version)
    QML_WRITABLE_AUTO_PROPERTY (QVariantList, languages)
    QML_WRITABLE_AUTO_PROPERTY (int, languageIndex)
    QML_WRITABLE_AUTO_PROPERTY (QVariantMap, settings)
    QML_WRITABLE_AUTO_PROPERTY (QString, rendererName)

    Q_PROPERTY(Element* rootElement READ rootElement CONSTANT)
    Q_PROPERTY(RecordField* rootField READ rootField CONSTANT)

public:
    static constexpr char TR_NONE[] = "tr:None";
    static constexpr char TR_YES[] = "tr:Yes";
    static constexpr char TR_NO[] = "tr:No";
    static constexpr char TR_OTHER[] = "tr:Other";
    static constexpr char TR_EMPTY[] = "tr:Empty";
    static constexpr char TR_LOCATION[] = "tr:Location";
    static constexpr char TR_PHOTOS[] = "tr:Photos";
    static constexpr char TR_NOTES[] = "tr:Notes";
    static constexpr char TR_DISTANCE[] = "tr:Distance";
    static constexpr char TR_BEARING[] = "tr:Bearing";
    static constexpr char TR_AUDIO_NOTE[] = "tr:Audio note";
    static constexpr char TR_SAMPLING_UNIT[] = "tr:Sampling unit";
    static constexpr char TR_OBSERVER[] = "tr:Observer";
    static constexpr char TR_CATEGORIES[] = "tr:Categories";
    static constexpr char TR_TRANSPORT_TYPE[] = "tr:Transport type";
    static constexpr char TR_IS_ARMED[] = "tr:Is armed";
    static constexpr char TR_TEAM[] = "tr:Team";
    static constexpr char TR_STATION[] = "tr:Station";
    static constexpr char TR_MANDATE[] = "tr:Mandate";
    static constexpr char TR_OBJECTIVE[] = "tr:Objective";
    static constexpr char TR_COMMENT[] = "tr:Comment";
    static constexpr char TR_EMPLOYEES[] = "tr:Employees";
    static constexpr char TR_LEADER[] = "tr:Leader";
    static constexpr char TR_PILOT[] = "tr:Pilot";
    static constexpr char TR_INCIDENT_TYPE[] = "tr:IncidentType";

public:
    Schema(QObject* parent = nullptr);
    ~Schema();

    QQmlListProperty<QObject> items();

    Element* rootElement() const;
    RecordField* rootField() const;

    static Schema* acquire(const QString& hash);
    static void release(const QString hash);
    static QStringList cached();
    static void garbageCollectCache();

    void saveToQmlFile(const QString& filePath) const;

    QVariant getSetting(const QString& name, const QVariant& defaultValue = QVariant()) const;
    void setSetting(const QString& name, const QVariant& value);

    // Project.
    void setProjectInfo(const QString& title, const QString& subtitle, const QString& icon);

    // Element.
    Element* getElement(const QString& elementUid) const;
    QString getElementName(const QString& elementUid, const QString& languageCode) const;
    QString getElementIcon(const QString& elementUid, bool walkBack = false) const;
    void enumElementChildren(const QString& parentElementUid, bool leavesOnly, const std::function<void(Element*)>& callback) const;
    QStringList getLeafElementUids(const QString& parentElementUid, bool ignoreHidden = true) const;
    QList<Element*> getLeafElements(const QString& parentElementUid, bool ignoreHidden = true) const;
    void sortByIndex(QStringList& elementUids) const;
    void appendElement(Element* element);

    // Field.
    BaseField* getField(const QString& fieldUid) const;
    QString getFieldName(const QString& fieldUid, const QString& languageCode) const;
    QString getFieldIcon(const QString& fieldUid) const;
    QString getFieldType(const QString& fieldUid) const;
    void appendField(BaseField* field);
    QStringList buildFieldList(const QString& recordFieldUid, const QString& filterTagName = QString(), const QVariant& filterTagValue = QVariant()) const;
    QStringList buildFieldList(const QString& recordFieldUid, std::function<bool(BaseField* field)> filter) const;

private:
    QObjectList m_items;

    QString translater(const QString& trName) const;

    Element* m_rootElement = nullptr;
    mutable QMap<QString, Element*> m_elementMap;
    void populateElementMap(Element*) const;

    RecordField* m_rootField = nullptr;
    mutable QMap<QString, BaseField*> m_fieldMap;
    void populateFieldMap(BaseField*) const;
};

} // namespace CyberTracker

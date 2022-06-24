#pragma once
#include "pch.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Element

class Element: public QObject
{
    Q_OBJECT

    Q_PROPERTY (QQmlListProperty<Element> elements READ elements CONSTANT)
    Q_CLASSINFO ("DefaultProperty", "elements")

    Q_PROPERTY (Element* parentElement READ parentElement CONSTANT)
    Q_PROPERTY (bool hasChildren READ hasChildren CONSTANT)

    QML_WRITABLE_AUTO_PROPERTY (int, listIndex)
    QML_WRITABLE_AUTO_PROPERTY (QString, uid)
    QML_WRITABLE_AUTO_PROPERTY (QString, exportUid)
    QML_WRITABLE_AUTO_PROPERTY (QString, name)
    QML_WRITABLE_AUTO_PROPERTY (QJsonObject, names)
    QML_WRITABLE_AUTO_PROPERTY (QString, icon)
    QML_WRITABLE_AUTO_PROPERTY (QVariantMap, tag)
    QML_WRITABLE_AUTO_PROPERTY (QColor, color)
    QML_WRITABLE_AUTO_PROPERTY (QStringList, elementUids)
    QML_WRITABLE_AUTO_PROPERTY (QStringList, fieldUids)
    QML_WRITABLE_AUTO_PROPERTY (bool, hidden)
    QML_WRITABLE_AUTO_PROPERTY (bool, other)

public:
    Element(QObject* parent = nullptr);
    ~Element();

    Q_INVOKABLE QQmlListProperty<Element> elements();
    Q_INVOKABLE QVariant getTagValue(const QString& key, const QVariant& defaultValue = QVariant());

    bool hasChildren() const;

    int elementCount() const;
    Element* elementAt(int index) const;
    Element* parentElement() const;

    void addTag(const QString& key, const QVariant& value);
    void removeTag(const QString& key);

    void appendElement(Element* element);
    void appendElementUid(const QString& elementUid);

    void appendFieldUid(const QString& fieldUid);
    void removeFieldUid(const QString& fieldUid);

    QString safeExportUid();

    static void toQml(QTextStream& stream, Element* element, int depth);

private:
    QList<Element*> m_elements;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ElementManager

class ElementManager: public QObject
{
    Q_OBJECT

    Q_PROPERTY(Element* rootElement READ rootElement CONSTANT)

public:
    ElementManager(QObject* parent = nullptr);
    ~ElementManager();

    Element* rootElement() const;

    Element* getElement(const QString& elementUid) const;
    QString getElementName(const QString& elementUid) const;
    QString getElementName(const QString& elementUid, const QString& languageCode) const;
    QUrl getElementIcon(const QString& elementUid, bool walkBack = false) const;

    void enumChildren(const QString& parentElementUid, bool leavesOnly, const std::function<void(Element*)>& callback) const;
    QStringList getLeafElementUids(const QString& parentElementUid);
    QList<Element*> getLeafElements(const QString& parentElementUid);

    void appendElement(Element* parent, Element* element);

    void loadFromQmlFile(const QString& filePath);
    void saveToQmlFile(const QString& filePath);

    void sortByIndex(QStringList& elementUids) const;

    void completeUpdate();

private:
    QString m_cacheKey;
    Element* m_rootElement;
    QMap<QString, Element*> m_elementMap;
    void populateMap(Element*);
};

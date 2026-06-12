#pragma once
#include "pch.h"

namespace CyberTracker {

class Element: public QObject
{
    Q_OBJECT

    Q_PROPERTY (QQmlListProperty<Element> elements READ elements CONSTANT)
    Q_CLASSINFO ("DefaultProperty", "elements")

    Q_PROPERTY (Element* parentElement READ parentElement CONSTANT)
    Q_PROPERTY (bool hasChildren READ hasChildren CONSTANT)
    Q_PROPERTY (int elementCount READ elementCount CONSTANT)

    QML_WRITABLE_AUTO_PROPERTY (int, listIndex)
    QML_WRITABLE_AUTO_PROPERTY (QString, uid)
    QML_WRITABLE_AUTO_PROPERTY (QString, exportUid)
    QML_WRITABLE_AUTO_PROPERTY (QString, name)
    QML_WRITABLE_AUTO_PROPERTY (QJsonObject, names)
    QML_WRITABLE_AUTO_PROPERTY (QString, icon)
    QML_WRITABLE_AUTO_PROPERTY (QString, audio)
    QML_WRITABLE_AUTO_PROPERTY (QVariantMap, tag)
    QML_WRITABLE_AUTO_PROPERTY (QColor, color)
    QML_WRITABLE_AUTO_PROPERTY (QStringList, elementUids)
    QML_WRITABLE_AUTO_PROPERTY (QStringList, fieldUids)
    QML_WRITABLE_AUTO_PROPERTY (QStringList, aliases)
    QML_WRITABLE_AUTO_PROPERTY (bool, hidden)
    QML_WRITABLE_AUTO_PROPERTY (bool, other)

public:
    Element(QObject* parent = nullptr);
    ~Element();

    Q_INVOKABLE QQmlListProperty<Element> elements();
    Q_INVOKABLE QVariant getTagValue(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void setTagValue(const QString& key, const QVariant& value);

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

    static void toQml(const QString& path, QTextStream& stream, Element* element, int depth);

private:
    QList<Element*> m_elements;
};

} // namespace CyberTracker

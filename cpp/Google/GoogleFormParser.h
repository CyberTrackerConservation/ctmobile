#pragma once
#include "pch.h"
#include "Element.h"
#include "Field.h"

class GoogleFormParser: public QObject
{
    Q_OBJECT

public:
    GoogleFormParser(QObject* parent = nullptr);

    static bool parse(const QString& formFilePath, const QString& targetFolder, QString* parserErrorOut);
    static QString parseParams(QVariantMap* params, const QString& description);

private:
    QString m_targetFolder;
    ElementManager m_elementManager;
    FieldManager m_fieldManager;
    QVariantMap m_settings;
    QStringList m_summaryTexts;
    QStringList m_summaryIcons;
    QVariantList m_saveTargets;
    QString m_snapLocation;

    bool execute(const QString& formFilePath, const QString& targetFolder, QString* parserErrorOut);

    void processForm(const QVariantMap& formMap);

    QString findNextPageBreakItemId(const QVariantList& items, const QString& itemId) const;
    bool isForwardLink(const QVariantList& items, const QString& currItemId, const QString& gotoItemId) const;
    QString downloadFile(const QString& url, const QString& targetName);
    Element* parseChoices(const QVariantList& items, const QString& itemId, const QVariantList& choices, bool addOther, bool* hasNavOut = nullptr);
    RecordField* parseQuestionGrid(const QVariantList& items, const QString& itemId, const QVariantMap& item, const QVariantMap& params);
    BaseField* parseQuestion(const QVariantList& items, const QString& itemId, const QVariantMap& item, const QVariantMap& params);
    void postProcess(RecordField* recordField);
};

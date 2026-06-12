#pragma once
#include "pch.h"
#include "Schema.h"
#include "Project.h"
#include <xlnt/xlnt.hpp>

namespace CyberTracker {

class XlsFormParser
{
public:
    XlsFormParser();

    static void configureProject(Project* project, Schema* schema);
    static void configureProject(const QString& projectUid, Schema* schema);
    static bool parse(const QString& xlsxFilePath, Schema* schema, QString* errorStringOut);

   bool execute(const QString& xlsxFilePath, Schema* schema);

private:
    QString m_folderPath;
    Schema* m_schema = nullptr;
    QVariantList m_languages;
    bool m_requireUsername = false;
    bool m_requireCamera = false;
    bool m_requireAudio = false;
    bool m_requireLocation = false;
    bool m_requireBackgroundLocation = false;
    QHash<QString, bool> m_choices;
    QMap<QString, Element*> m_fieldElements;
    QTextDocument m_textDocument;

    struct SurveyRow
    {
        QString type;
        QString name;
        QVariantMap label;
        QVariantMap hint;
        QString appearance;
        QString required;
        QVariantMap requiredMessage;
        QString readonly;
        QVariant defaultValue;
        QString calculation;
        QString constraint;
        QVariantMap constraintMessage;
        QString relevant;
        QString choiceFilter;
        QString repeatCount;
        QString mediaAudio;
        QString mediaImage;
        QString bodyAccept;
        QString bindType;
        QString bindEsriFieldType;
        QString bindEsriFieldLength;
        QString bindEsriFieldAlias;
        QString bodyEsriStyle;
        QString bindEsriParameters;
        QVariantMap parameters;
        QStringList aliases;
    };

    struct ChoiceRow
    {
        QString listName;
        QString name;
        QVariantMap label;
        QString image;
        QVariantMap tags;
        QStringList aliases;
    };

    void parseStringToMap(const QString& header, const QString& value, QVariantMap* languagesOut, bool removeMarkdown);
    QString findAsset(const QString& name) const;
    static QString parseCTHeader(const QString& header);
    QVariantMap parseParameters(const QString& params) const;
    static QVariantMap settingsRowFromSheet(const xlnt::worksheet& sheet);
    SurveyRow surveyRowFromSheet(const xlnt::worksheet& sheet, const xlnt::cell_vector& row);
    ChoiceRow choiceRowFromSheet(const xlnt::worksheet& sheet, const xlnt::cell_vector& row);

    void processSurvey(const xlnt::worksheet& sheet, int* startRowIndex, RecordField* recordField, Element* rootElement);
    void processChoices(const xlnt::worksheet& sheet, int startRowIndex, Element* rootElement);
    void postProcess(RecordField* recordField);
};

} // namespace CyberTracker

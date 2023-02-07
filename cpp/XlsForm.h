#pragma once
#include "pch.h"
#include "Element.h"
#include "Field.h"
#include "Project.h"
#include <xlnt/xlnt.hpp>

class XlsFormParser
{
public:
    XlsFormParser();

    static void configureProject(Project* project, const QVariantMap& settings);
    static void configureProject(const QString& projectUid, const QVariantMap& settings);
    static bool parseSettings(const QString& xlsxFilePath, QVariantMap* settingsOut);
    static bool getFormVersion(const QString& xlsxFilePath, QString* versionOut);
    static bool parse(const QString& xlsxFilePath, const QString& targetFolder, const QString& projectUid, QString* errorStringOut);

    bool execute(const QString& xlsxFilePath, const QString& targetFolder, const QString& updateProjectUid = "");

private:
    QString m_folderPath;
    ElementManager m_elementManager;
    FieldManager m_fieldManager;
    QVariantList m_languages;
    bool m_requireUsername = false;
    QVariantMap m_settings;
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
    };

    struct ChoiceRow
    {
        QString listName;
        QString name;
        QVariantMap label;
        QString image;
        QVariantMap tags;
    };

    void parseStringToMap(const QString& header, const QString& value, QVariantMap* languagesOut, bool removeMarkdown);
    QString findAsset(const QString& name) const;
    static QVariantMap parseCT(const QVariantMap& parameters, const QString& header, const QString& value);
    QVariantMap parseParameters(const QString& params) const;
    static QVariantMap settingsRowFromSheet(const xlnt::worksheet& sheet);
    SurveyRow surveyRowFromSheet(const xlnt::worksheet& sheet, const xlnt::cell_vector& row);
    ChoiceRow choiceRowFromSheet(const xlnt::worksheet& sheet, const xlnt::cell_vector& row);

    void processSurvey(const xlnt::worksheet& sheet, int* startRowIndex, RecordField* recordField, Element* rootElement);
    void processChoices(const xlnt::worksheet& sheet, int startRowIndex, Element* rootElement);
    void postProcess(RecordField* recordField);
};

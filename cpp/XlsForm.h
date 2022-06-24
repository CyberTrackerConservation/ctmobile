#pragma once
#include "pch.h"
#include "Element.h"
#include "Field.h"
#include <xlnt/xlnt.hpp>

namespace XlsFormUtils
{
bool parseXlsForm(const QString& xlsxFilePath, const QString& targetFolder, QString* errorStringOut);
}

class XlsFormParser
{
public:
    XlsFormParser();
    bool execute(const QString& xlsxFilePath, const QString& targetFolder);

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
    QString findAsset(const QString& name);
    QVariantMap parseParameters(const QString& params);
    SurveyRow surveyRowFromSheet(const xlnt::worksheet& sheet, const xlnt::cell_vector& row);
    ChoiceRow choiceRowFromSheet(const xlnt::worksheet& sheet, const xlnt::cell_vector& row);
    QVariantMap settingsRowFromSheet(const xlnt::worksheet& sheet);

    void processSurvey(const xlnt::worksheet& sheet, int* startRowIndex, RecordField* recordField, Element* rootElement);
    void processChoices(const xlnt::worksheet& sheet, int startRowIndex, Element* rootElement);
    void postProcess(RecordField* recordField);
};

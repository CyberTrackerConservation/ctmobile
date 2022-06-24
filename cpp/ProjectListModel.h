#pragma once
#include "pch.h"
#include "Project.h"

class ProjectListModel: public VariantListModel, public QQmlParserStatus
{
    Q_OBJECT

    Q_INTERFACES (QQmlParserStatus)

public:
    ProjectListModel(QObject* parent = nullptr);
    ~ProjectListModel();

    void classBegin() override;
    void componentComplete() override;

private:
    QStringList m_projectUids;

private slots:
    void rebuild();

private:
    ProjectManager* m_projectManager = nullptr;
    QList<std::shared_ptr<Project>> m_projects;
};

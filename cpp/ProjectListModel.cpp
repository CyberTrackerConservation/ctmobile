#include "ProjectListModel.h"
#include "App.h"

ProjectListModel::ProjectListModel(QObject* parent): VariantListModel(parent)
{
}

ProjectListModel::~ProjectListModel()
{
}

void ProjectListModel::classBegin()
{
}

void ProjectListModel::componentComplete()
{
    m_projectManager = App::instance()->projectManager();
    
    connect(m_projectManager, &ProjectManager::projectsChanged, this, &ProjectListModel::rebuild);

    connect(m_projectManager, &ProjectManager::projectSettingChanged, this, [&](const QString& projectUid, const QString& /*name*/, const QVariant& /*value*/)
    {
        change(m_projectUids.indexOf(projectUid));
    });

    rebuild();
}

void ProjectListModel::rebuild()
{
    clear();
    m_projectUids.clear();

    m_projects.clear();
    m_projects = m_projectManager->buildList();

    auto list = QVariantList();
    for (auto it = m_projects.constBegin(); it != m_projects.constEnd(); it++)
    {
        m_projectUids.append(it->get()->uid());
        list.append(QVariant::fromValue<Project*>(it->get()));
    }

    appendList(list);
}

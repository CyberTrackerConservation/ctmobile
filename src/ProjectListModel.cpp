#include "ProjectListModel.h"
#include "App.h"

namespace CyberTracker {

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
    connect(App::instance()->settings(), &Settings::projectsChanged, this, &ProjectListModel::rebuild);

    connect(m_projectManager, &ProjectManager::projectSettingChanged, this, [&](const QString& projectUid, const QString& /*name*/, const QVariant& /*value*/)
    {
        change(m_projectUids.indexOf(projectUid));
    });

    rebuild();
}

void ProjectListModel::rebuild()
{
    if (m_rebuilding)
    {
        return;
    }

    m_rebuilding = true;

    clear();

    m_projectUids.clear();
    m_projects.clear();
    m_projects = m_projectManager->buildListPtr();

    auto projectList = QVariantList();
    for (auto projectIt = m_projects.constBegin(); projectIt != m_projects.constEnd(); projectIt++)
    {
        auto project = projectIt->get();
        project->setParent(this);
        projectList.append(QVariant::fromValue<Project*>(project));
        m_projectUids.append(project->uid());
    }

    appendList(projectList);

    m_rebuilding = false;

    emit changed();
}

} // namespace CyberTracker

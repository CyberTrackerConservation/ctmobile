#include "Goto.h"
#include "App.h"

GotoManager::GotoManager(QObject* parent): QObject(parent)
{
    m_gotoHeading = 0;
    m_gotoPathIndex = 0;

    loadState();
}

GotoManager::~GotoManager()
{
}

void GotoManager::loadState()
{
    auto state = App::instance()->settings()->gotoManager();
    m_gotoTitle = state.value("title", QString()).toString();
    m_gotoPath = state.value("path", QVariantList()).toList();
    m_gotoPathIndex = state.value("pathIndex", 0).toInt();
}

void GotoManager::saveState()
{
    App::instance()->settings()->set_gotoManager(QVariantMap
    {
        { "title", m_gotoTitle },
        { "path", m_gotoPath },
        { "pathIndex", m_gotoPathIndex }
    });
}

void GotoManager::setTarget(const QString& title, const QVariantList& path, int pathIndex)
{
    pathIndex = std::min(pathIndex, path.count() - 1);
    pathIndex = std::max(pathIndex, 0);

    update_gotoTitle(title);
    update_gotoPath(path);
    update_gotoPathIndex(pathIndex);

    saveState();

    emit gotoTargetChanged(title);
}

void GotoManager::recalculate(Location* location)
{
    if (m_gotoPath.isEmpty())
    {
        auto text = m_gotoPath.isEmpty() ? "-" : tr("No fix");
        update_gotoTitle(text);
        update_gotoPath(QVariantList());
        update_gotoTitleText(text);
        update_gotoLocationText(text);
        update_gotoDistanceText(text);
        update_gotoHeadingText(text);
        update_gotoHeading(0);
        saveState();

        return;
    }

    auto gotoPoint = m_gotoPath[m_gotoPathIndex].toList();
    auto gotoX = gotoPoint[0].toDouble();
    auto gotoY = gotoPoint[1].toDouble();
    auto distance = static_cast<int>(location->distanceTo(gotoX, gotoY));;

    auto titleText = m_gotoTitle;
    if (m_gotoPath.count() > 1)
    {
        // Advance to the next point.
        if (distance < 40 && m_gotoPathIndex < m_gotoPath.count() - 1)
        {
            update_gotoPathIndex(m_gotoPathIndex++);
            recalculate(location);
            return;
        }

        titleText += " (" + QString::number(m_gotoPathIndex + 1) + "/" + QString::number(m_gotoPath.count()) + ")";
    }

    // Update the public properties.
    update_gotoTitleText(titleText);
    update_gotoLocationText(App::instance()->getLocationText(gotoX, gotoY));
    update_gotoDistanceText(App::instance()->getDistanceText(distance));
    update_gotoLocation(QVariantMap {{"x", gotoX}, {"y", gotoY}});

    auto heading = static_cast<int>(location->headingTo(gotoX, gotoY));
    update_gotoHeadingText(Utils::headingToText(heading));

    auto direction = std::isnan(location->direction()) ? 0 : static_cast<int>(location->direction());
    update_gotoHeading((360 + heading - direction) % 360);

    saveState();
}

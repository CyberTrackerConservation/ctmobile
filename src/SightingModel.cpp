#include "SightingModel.h"
#include "App.h"

namespace CyberTracker {

SightingModel::SightingModel(QObject* parent): VariantListModel(parent), m_sighting(nullptr)
{
}

SightingModel::~SightingModel()
{
}

void SightingModel::classBegin()
{
}

void SightingModel::componentComplete()
{
    connect(this, &SightingModel::sightingUidChanged, this, &SightingModel::rebuild);
    connect(this, &SightingModel::sightingChanged, this, &SightingModel::rebuild);

    auto app = App::instance();
    connect(app, &App::sightingsChanged, this, &SightingModel::rebuild);
    connect(app, &App::sightingSaved, this, &SightingModel::changed);
    connect(app, &App::sightingDeleted, this, &SightingModel::removed);
    connect(app, &App::sightingModified, this, &SightingModel::changed);
    connect(app, &App::sightingFlagsChanged, this, &SightingModel::changed);
    connect(app->settings(), &Settings::languageCodeChanged, this, &SightingModel::rebuild);

    m_form = Form::parentForm(this);
    if (m_form)
    {
        connect(m_form, &Form::languageCodeChanged, this, &SightingModel::rebuild);
    }

    rebuild();
}

void SightingModel::rebuild()
{
    clear();

    if (m_sighting)
    {
        appendList(App::instance()->createRendererPtr(m_sighting->schema()->rendererName())->buildSightingModel(m_sighting));
        return;
    }

    if (m_sightingUid.isEmpty())
    {
        return;
    }

    if (!App::instance()->database()->testData("", m_sightingUid))
    {
        set_sightingUid("");
        return;
    }

    auto dbSighting = Database::DbData();
    App::instance()->database()->loadData("", m_sightingUid, &dbSighting);
    auto languageCode = m_form ? m_form->languageCode() : App::instance()->settings()->languageCode();
    auto sighting = std::unique_ptr<Sighting>(new Sighting(dbSighting.data, dbSighting.stateSpace, Schema::acquire(dbSighting.schemaHash), languageCode));
    appendList(App::instance()->createRendererPtr(sighting->schema()->rendererName())->buildSightingModel(sighting.get()));
    Schema::release(dbSighting.schemaHash);
}

void SightingModel::changed(const QString& /*projectUid*/, const QString& sightingUid)
{
    if (sightingUid == m_sightingUid)
    {
        rebuild();
    }
}

void SightingModel::removed(const QString& /*projectUid*/, const QString& sightingUid)
{
    if (sightingUid == m_sightingUid)
    {
        set_sightingUid("");
    }
}

} // namespace CyberTracker

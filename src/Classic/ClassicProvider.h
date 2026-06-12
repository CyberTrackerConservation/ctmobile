#pragma once
#include "pch.h"
#include "ProviderInterface.h"
#include "ClassicView.h"

namespace CyberTracker {

class ClassicProvider : public Provider
{
    Q_OBJECT

    QML_READONLY_AUTO_PROPERTY (ClassicView*, classicView)

    QML_WRITABLE_AUTO_PROPERTY (QByteArray, stateBlob)
    QML_WRITABLE_AUTO_PROPERTY (QStringList, stateAttachments)

    Q_PROPERTY (Sighting* currentSighting READ currentSighting CONSTANT)
    QML_READONLY_AUTO_PROPERTY (QString, currentSightingSummaryText)

public:
    static constexpr char PROVIDER_NAME[] = "Classic";

public:
    ClassicProvider(Form* form);
    ~ClassicProvider();

    static int version();

    void registerClassicView(ClassicView* classicView);
    void unregisterClassicView();

    Sighting* currentSighting();

    bool supportLocationTrackField() const override;

    bool connectToProject() override;

    QVariantMap saveState(QStringList* attachments) override;
    void loadState(const QVariantMap& data) override;

    bool resetPageStackOnConnect() const override;
    QString getStartPage() const override;

    bool canSnapTrackToNewSighting() const override;
    bool canSubmitData() const override;
    void submitData() override;

    void editSighting(const QString& stateSpace, const QString& sightingUid) const override;

private:
    Sighting* m_currentSighting = nullptr;
};

} // namespace CyberTracker

#pragma once
#include "pch.h"
#include "Sighting.h"

class Wizard: public QObject
{
    Q_OBJECT

    Q_PROPERTY (QString pageUrl READ pageUrl CONSTANT)
    Q_PROPERTY (QString indexPageUrl READ indexPageUrl CONSTANT)

    QML_READONLY_AUTO_PROPERTY (bool, canNext)
    QML_READONLY_AUTO_PROPERTY (bool, canSave)
    QML_READONLY_AUTO_PROPERTY (bool, canSkip)
    QML_READONLY_AUTO_PROPERTY (bool, lastPage)

public:
    explicit Wizard(QObject* parent = nullptr);

    void updateButtons();

    Q_INVOKABLE QString pageUrl() const;
    Q_INVOKABLE QString indexPageUrl() const;

    Q_INVOKABLE void reset();
    Q_INVOKABLE void first(const QString& recordUid, int transition = /*StackView.Immediate*/ 0);
    Q_INVOKABLE void edit(const QString& recordUid, const QString& fieldUid = QString(), int transition = /*StackView.Immediate*/ 0);
    Q_INVOKABLE void back(int transition = /*StackView.PopTransition*/ 3);
    Q_INVOKABLE void next(const QString& recordUid, const QString& fieldUid = QString(), int transition = /*StackView.Immediate*/ 0);
    Q_INVOKABLE void home(int transition = /*StackView.PopTransition*/ 3);
    Q_INVOKABLE void skip(const QString& targetFieldUid, int transition = /*StackView.Immediate*/ 0);
    Q_INVOKABLE bool save(const QString& targetFieldUid = QString(), int transition = /*StackView.Immediate*/ 0);
    Q_INVOKABLE void gotoField(const QString& targetFieldUid, int transition = /*StackView.Immediate*/ 0);
    Q_INVOKABLE void index(const QString& recordUid, const QString& fieldUid = QString(), int transition = /*StackView.Immediate*/ 0);
    Q_INVOKABLE QString error(const QString& recordUid, const QString& fieldUid = QString()) const;
    Q_INVOKABLE QVariantList findSaveTargets();

private:
    Sighting* m_sighting = nullptr;
    
    bool advance(const QString& recordUid, const QString& fieldUid, QVariantList* wizardPageStackOut, QString* errorOut, bool* lastPageOut);
    void reverse(QVariantList* wizardPageStackOut, const std::function<bool(const QString& recordUid, const QString& fieldUid)>& done);
};

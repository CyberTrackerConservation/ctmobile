#pragma once
#include "pch.h"
#include "Sighting.h"

class Wizard: public QObject
{
    Q_OBJECT

    Q_PROPERTY (QString pageUrl READ pageUrl CONSTANT)
    Q_PROPERTY (QString indexPageUrl READ indexPageUrl CONSTANT)
    Q_PROPERTY (QString optionsPageUrl READ optionsPageUrl CONSTANT)
    Q_PROPERTY (QString recordUid READ recordUid CONSTANT)
    Q_PROPERTY (QVariantMap rules READ rules CONSTANT)
    Q_PROPERTY (QStringList fieldUids READ fieldUids CONSTANT)
    Q_PROPERTY (bool valid READ valid CONSTANT)
    Q_PROPERTY (bool autoSave READ autoSave CONSTANT)
    Q_PROPERTY (bool immersive READ immersive CONSTANT)
    Q_PROPERTY (int popCount READ popCount CONSTANT)
    Q_PROPERTY (QString header READ header CONSTANT)
    Q_PROPERTY (QString banner READ banner CONSTANT)
    Q_PROPERTY (QString lastPageRecordUid READ lastPageRecordUid CONSTANT)
    Q_PROPERTY (QString lastPageFieldUid READ lastPageFieldUid CONSTANT)

    QML_READONLY_AUTO_PROPERTY (bool, canBack)
    QML_READONLY_AUTO_PROPERTY (bool, canNext)
    QML_READONLY_AUTO_PROPERTY (bool, canSave)
    QML_READONLY_AUTO_PROPERTY (bool, canSkip)
    QML_READONLY_AUTO_PROPERTY (bool, lastPage)

public:
    explicit Wizard(QObject* parent = nullptr);

    void updateButtons();

    static QString pageUrl();
    static QString indexPageUrl();
    static QString optionsPageUrl();
    static const int TRANSITION_NO_REBUILD = -1;

    bool ready() const;
    QString recordUid() const;
    QVariantMap rules() const;
    QStringList fieldUids() const;
    bool valid() const;
    bool autoSave() const;
    bool immersive() const;
    int popCount() const;
    QString header() const;
    QString banner() const;
    QString lastPageRecordUid() const;
    QString lastPageFieldUid() const;

    Q_INVOKABLE void init(const QString& recordUid);
    Q_INVOKABLE void reset();
    Q_INVOKABLE void first(const QString& recordUid, int transition = /*StackView.Immediate*/ 0);
    Q_INVOKABLE void edit(const QString& recordUid, const QString& fieldUid = QString(), int transition = /*StackView.Immediate*/ 0);
    Q_INVOKABLE void back(int transition = /*StackView.PopTransition*/ 3);
    Q_INVOKABLE void next(const QString& recordUid, const QString& fieldUid = QString(), int transition = /*StackView.Immediate*/ 0);
    Q_INVOKABLE void home(int transition = /*StackView.Immediate*/ 0);
    Q_INVOKABLE void skip(const QString& targetFieldUid, int transition = /*StackView.Immediate*/ 0);
    Q_INVOKABLE bool save(const QString& targetFieldUid = QString(), int transition = /*StackView.Immediate*/ 0);
    Q_INVOKABLE void gotoField(const QString& targetFieldUid, int transition = /*StackView.Immediate*/ 0);
    Q_INVOKABLE void index(const QString& recordUid, const QString& fieldUid = QString(), bool highlightInvalid = false, int transition = /*StackView.Immediate*/ 0);
    Q_INVOKABLE void options(const QString& recordUid, const QString& fieldUid = QString(), int transition = /*StackView.Immediate*/ 0);
    Q_INVOKABLE QString error(const QString& recordUid, const QString& fieldUid = QString()) const;

signals:
    void rebuildPage(int transition);

private:
    Sighting* m_sighting = nullptr;
    
    QVariantMap lastPageParams() const;
    bool advance(const QString& recordUid, const QString& fieldUid, QVariantList* wizardPageStackOut, QString* errorOut, bool* lastPageOut);
    void reverse(QVariantList* wizardPageStackOut, const std::function<bool(const QString& recordUid, const QString& fieldUid)>& done);
};

#pragma once
#include "pch.h"
#include <fxApplication.h>
#include "Form.h"

using namespace CyberTracker;

class CctSession;
class CctSighting;

class ClassicView : public QQuickPaintedItem
{
    Q_OBJECT

    QML_READONLY_AUTO_PROPERTY(Form*, form)
    QML_READONLY_AUTO_PROPERTY(QString, projectUid)
    QML_WRITABLE_AUTO_PROPERTY(int, initialWidth)
    QML_WRITABLE_AUTO_PROPERTY(int, initialHeight)
    QML_READONLY_AUTO_PROPERTY(QVariantList, gotoPoints)
    QML_READONLY_AUTO_PROPERTY(QVariantList, historyPoints)
    QML_READONLY_AUTO_PROPERTY(QVariantList, historyLines)

public:
    ClassicView(QQuickItem *parent = Q_NULLPTR);
    virtual ~ClassicView() override;

    void setImage(QImage* image);

    void classBegin() override;
    void componentComplete() override;

    virtual void paint(QPainter *painter) override;

    void connectSession();
    void disconnectSession();

    CctSession *session() const { return m_session; }

    static Sighting* makeSighting(Form* form, CctSighting* pSighting, CfxResource* pResource);

    void saveSighting(CctSighting* pSighting);

    void editSighting(const QString& sightingUid);
    void deleteSighting(const QString& sightingUid);

    Q_INVOKABLE QString getTextEditValue(int controlId);
    Q_INVOKABLE void setTextEditValue(int controlId, const QString& text);

    Q_INVOKABLE void confirmResult(int controlId, bool confirmed);

    bool canSubmitData() const;
    bool submitData();

    void saveState();
    void loadState();

signals:
    void setInputMethodVisible(bool visible);

    void showConfirm(int controlId, const QString& message);

    void showCameraDialog(int resolutionX, int resolutionY, bool simpleMode);
    void showBarcodeDialog();
    void showExports();
    void showUploadQueue();
    void showMapPage();
    void showSightingEditorPage(bool isExportMode);
    void showTextEditorPage(int controlId);
    void showGPSReaderPage(int saveTarget, int skipTimeout, bool skipAllowed);

    void shareData();
    void submitDataClicked();

    void shutdown();

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void touchEvent(QTouchEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent * event) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void keyPressEvent(QKeyEvent * event) override;
    void keyReleaseEvent(QKeyEvent * event) override;

    void connectSession(const char *pRoot, const char *Name, int Width, int Height, int Dpi, int ScaleX, int ScaleY, int ScaleBorder, int ScaleFont, int ScaleHitSize, bool UseResourceScale);
    
    void buildSources();

private:
    QString m_projectDir;
    QString m_projectApp;
    CfxApplication* m_application = nullptr;
    CctSession* m_session = nullptr;
    char* m_path = nullptr;
    char* m_file = nullptr;
    int m_dpi = 96;
    int m_scaleX = 100;
    int m_scaleY = 100;
    int m_scaleBorder = 100;
    int m_scaleFont = 100;
    int m_scaleHitSize = 100;
    QImage m_image;
    QMutex m_imageLock;
};

extern ClassicView *GetClassicView(CfxPersistent *pPersistent);

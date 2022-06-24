#pragma once
#include "pch.h"

#include <QQuickPaintedItem>
#include <QQuickItem>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QImage>
#include <QMutex>

#include <PropertyHelpers.h>
#include <ctMain.h>

class CtClassicSessionItem : public QQuickPaintedItem
{
    Q_OBJECT

    QML_READONLY_AUTO_PROPERTY(QString, projectUid)
    QML_WRITABLE_AUTO_PROPERTY(QString, lastEditorText)
    QML_WRITABLE_AUTO_PROPERTY(bool, blockResize)

public:
    CtClassicSessionItem(QQuickItem *parent = Q_NULLPTR);
    virtual ~CtClassicSessionItem() override;

    void setImage(QImage* image);

    void classBegin() override;
    void componentComplete() override;

    Q_INVOKABLE void connectToProject(const QString& projectUid);
    Q_INVOKABLE void disconnectFromProject();

    virtual void paint(QPainter *painter) override;

signals:
    void setInputMethodVisible(bool visible);
    void showEditControl(const QVariantMap& params);
    void hideEditControl();
    void setEditControlFont(const QVariantMap& params);

    void showCameraDialog(int resolutionX, int resolutionY);
    void showBarcodeDialog();
    void showSkyplot(int x, int y, int width, int height, bool visible);
    void showExports();

    void shutdown();

protected:
    void touchEvent(QTouchEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent * event) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void keyPressEvent(QKeyEvent * event) override;
    void keyReleaseEvent(QKeyEvent * event) override;
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;

    void connectSession(const CHAR *pRoot, const CHAR *Name, UINT Width, UINT Height, UINT Dpi, UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleFont, UINT ScaleHitSize, BOOL UseResourceScale);
    void connectSession();
    void disconnectSession();

private:
    QString m_projectDir;
    QString m_projectApp;
    CctApplication* m_application = nullptr;
    CHAR* m_path = nullptr;
    CHAR* m_file = nullptr;
    int m_dpi = 96;
    int m_scaleX = 100;
    int m_scaleY = 100;
    int m_scaleBorder = 100;
    int m_scaleFont = 100;
    int m_scaleHitSize = 100;
    QImage m_image;
    QMutex m_imageLock;
};

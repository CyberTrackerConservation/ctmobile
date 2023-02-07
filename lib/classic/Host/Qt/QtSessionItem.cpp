#include "QtSessionItem.h"
#include "QtHost.h"
#include "App.h"

CtClassicSessionItem::CtClassicSessionItem(QQuickItem *parent)
    : QQuickPaintedItem(parent), m_ownerForm(nullptr)
{
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptTouchEvents(true);
    setFlag(QQuickItem::ItemHasContents, true);
    m_blockResize = false;

    connect(this, &CtClassicSessionItem::blockResizeChanged, [&]()
    {
        auto image = ((CCanvas_Qt *)m_application->GetHost()->GetCanvas())->GetImage();
        if (!m_blockResize && (width() != image->width()-1 || height() != image->height()-1))
        {
            //connectSession();
        }
    });
}

CtClassicSessionItem::~CtClassicSessionItem()
{
    disconnectSession();
}

void CtClassicSessionItem::classBegin()
{
    //QQmlParserStatus::classBegin();
}

void CtClassicSessionItem::componentComplete()
{
    QQuickPaintedItem::componentComplete();

    //QQmlParserStatus::componentComplete();
}

void CtClassicSessionItem::connectToProject(Form* form)
{
    m_ownerForm = form;

    m_projectDir = Utils::classicRoot();
    m_projectUid = form->projectUid();
    m_projectApp = form->project()->connectorParams()["app"].toString();

    m_dpi = qMax(96, App::instance()->dpi());
    qDebug() << "Width: " << width() << " Height: " << height() << " DPI: " << m_dpi;

    int autoScale = qMin(500, qMax(100, m_dpi));
    autoScale = App::instance()->scaleByFontSize(autoScale);

    m_scaleX = m_scaleY = m_scaleBorder = autoScale;
    m_scaleHitSize = (42 * autoScale) / 100;
    m_scaleFont = App::instance()->settings()->fontSize();
    m_blockResize = false;

    connectSession();
}

void CtClassicSessionItem::disconnectFromProject()
{
    disconnectSession();
}

void CtClassicSessionItem::connectSession()
{
    if ((width() > 0.0) && (height() > 0.0) && !m_projectDir.isEmpty())
    {
        connectSession(m_projectDir.toStdString().c_str(), m_projectApp.toStdString().c_str(), width() + 1, height() + 1, m_dpi, m_scaleX, m_scaleY, m_scaleBorder, m_scaleFont, m_scaleHitSize, FALSE);
    }
}

void CtClassicSessionItem::connectSession(const CHAR *pRoot, const CHAR *Name, UINT Width, UINT Height, UINT Dpi, UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleFont, UINT ScaleHitSize, BOOL UseResourceScale)
{
    disconnectSession();

    // Create the host
    FXPROFILE profile = {};

    profile.Magic = MAGIC_PROFILE;
    profile.Width = Width;
    profile.Height = Height;
    profile.BitDepth = 16;
    profile.DPI = Dpi;
    profile.ScaleX = ScaleX;
    profile.ScaleY = ScaleY;
    profile.ScaleBorder = ScaleBorder;
    profile.ScaleFont = ScaleFont;
    profile.ScaleHitSize = ScaleHitSize;
    profile.Zoom = 100;
    profile.AliasIndex = 0;

    CHost_Qt *host = new CHost_Qt(NULL, this, &profile, pRoot);
    host->SetBounds(0, 0, profile.Width, profile.Height);
    host->SetUseResourceScale(UseResourceScale);

    m_application = new CctApplication(host);

    // Strip extension
    CHAR Name2[MAX_PATH];
    strcpy(Name2, Name);
    CHAR *t = strrchr(Name2, '.');
    if (t != NULL)
    {
        *t = '\0';
    }

    m_application->Run(Name2);
    m_application->GetSession()->RestartWaypointTimer();
}

void CtClassicSessionItem::disconnectSession()
{
    if (m_application == NULL)
    {
        return;
    }

    m_application->Shutdown();
    delete m_application;
    m_application = NULL;
}

void CtClassicSessionItem::setImage(QImage* image)
{
    m_imageLock.lock();
    m_image = *image;
    m_imageLock.unlock();
}

void CtClassicSessionItem::paint(QPainter *painter)
{
    if (!m_application)
    {
        return;
    }

    if (m_imageLock.tryLock())
    {
        painter->drawImage(0, 0, m_image);
        m_imageLock.unlock();
    }
}

void CtClassicSessionItem::touchEvent(QTouchEvent *event)
{
    QQuickPaintedItem::touchEvent(event);

    if (event->touchPoints().size() != 1)
    {
        return;
    }
}

void CtClassicSessionItem::mousePressEvent(QMouseEvent *event)
{
    QQuickPaintedItem::mousePressEvent(event);
    if (event->button() == Qt::LeftButton)
    {
        if (m_application)
        {
            event->accept();
            m_application->GetEngine()->PenDown(event->pos().x(), event->pos().y());
        }
    }
}

void CtClassicSessionItem::mouseMoveEvent(QMouseEvent *event)
{
    QQuickPaintedItem::mouseMoveEvent(event);
    if (m_application)
    {
        event->accept();
        m_application->GetEngine()->PenMove(event->pos().x(), event->pos().y());
    }
}

void CtClassicSessionItem::mouseReleaseEvent(QMouseEvent *event)
{
    QQuickPaintedItem::mouseReleaseEvent(event);
    if (event->button() == Qt::LeftButton)
    {
        if (m_application)
        {
            event->accept();
            m_application->GetEngine()->PenUp(event->pos().x(), event->pos().y());
        }
    }
}

void CtClassicSessionItem::wheelEvent(QWheelEvent *event)
{
    QQuickPaintedItem::wheelEvent(event);
}

void CtClassicSessionItem::hoverMoveEvent(QHoverEvent *event)
{
    QQuickPaintedItem::hoverMoveEvent(event);
}

void CtClassicSessionItem::keyPressEvent(QKeyEvent * event)
{
    QQuickPaintedItem::keyPressEvent(event);
    if (m_application)
    {
        m_application->GetEngine()->KeyDown(event->key());
    }
}

void CtClassicSessionItem::keyReleaseEvent(QKeyEvent * event)
{
    QQuickPaintedItem::keyReleaseEvent(event);

    if (m_application)
    {
        m_application->GetEngine()->KeyUp(event->key());
    }
}

void CtClassicSessionItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickPaintedItem::geometryChanged(newGeometry, oldGeometry);

    if (!m_blockResize)
    {
        //connectSession();
    }
}

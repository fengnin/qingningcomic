#include "components/ImageViewerDialog.h"
#include "components/EditorStyles.h"
#include "api/SharedNetworkManager.h"
#include "utils/Logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QKeyEvent>
#include <QFileInfo>
#include <QApplication>
#include <QScreen>
#include <QNetworkReply>
#include <QEventLoop>

constexpr double ImageViewerDialog::MIN_SCALE;
constexpr double ImageViewerDialog::MAX_SCALE;
constexpr double ImageViewerDialog::SCALE_STEP;

namespace {
    const QString DIALOG_STYLE = R"(
        QDialog {
            background: #f2000000;
        }
    )";
    
    const QString TOOLBAR_STYLE = R"(
        QWidget {
            background: #e61e1e1e;
            border-radius: 8px;
        }
    )";
    
    const QString BUTTON_STYLE = R"(
        QPushButton {
            background: transparent;
            color: white;
            border: none;
            border-radius: 6px;
            padding: 8px;
            font-size: 14px;
        }
        QPushButton:hover {
            background: #26ffffff;
        }
        QPushButton:pressed {
            background: #40ffffff;
        }
    )";
    
    const QString LABEL_STYLE = R"(
        QLabel {
            color: white;
            font-size: 13px;
            padding: 4px 8px;
        }
    )";
    
    const QString SCROLL_STYLE = R"(
        QScrollArea {
            border: none;
            background: transparent;
        }
        QScrollArea > QWidget > QWidget {
            background: transparent;
        }
    )" + EditorStyles::scrollBarStyle();
    
    const int TOOLBAR_HEIGHT = 50;
    const int BUTTON_SIZE = 36;
}

ImageViewerDialog::ImageViewerDialog(QWidget *parent)
    : QDialog(parent)
    , m_scrollArea(nullptr)
    , m_imageLabel(nullptr)
    , m_zoomInBtn(nullptr)
    , m_zoomOutBtn(nullptr)
    , m_resetBtn(nullptr)
    , m_rotateLeftBtn(nullptr)
    , m_rotateRightBtn(nullptr)
    , m_closeBtn(nullptr)
    , m_zoomLabel(nullptr)
    , m_scaleFactor(1.0)
    , m_rotation(0)
    , m_dragging(false)
{
    setupUI();
    setupConnections();
    
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setStyleSheet(DIALOG_STYLE);
    
    QScreen *screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();
    setFixedSize(screenGeometry.size());
}

ImageViewerDialog::~ImageViewerDialog()
{
}

void ImageViewerDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    m_scrollArea = new QScrollArea();
    m_scrollArea->setStyleSheet(SCROLL_STYLE);
    m_scrollArea->setWidgetResizable(false);
    m_scrollArea->setAlignment(Qt::AlignCenter);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    m_imageLabel = new QLabel();
    m_imageLabel->setStyleSheet("background: transparent;");
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setScaledContents(false);
    m_imageLabel->setMouseTracking(true);
    m_scrollArea->setWidget(m_imageLabel);
    
    QWidget *toolbar = new QWidget();
    toolbar->setFixedHeight(TOOLBAR_HEIGHT);
    toolbar->setStyleSheet(TOOLBAR_STYLE);
    
    QHBoxLayout *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(16, 8, 16, 8);
    toolbarLayout->setSpacing(8);
    
    m_zoomInBtn = new QPushButton(QString::fromUtf8("+"));
    m_zoomInBtn->setFixedHeight(BUTTON_SIZE);
    m_zoomInBtn->setStyleSheet(BUTTON_STYLE);
    m_zoomInBtn->setCursor(Qt::PointingHandCursor);
    m_zoomInBtn->setToolTip(QString::fromUtf8("\u653e\u5927"));

    m_zoomOutBtn = new QPushButton(QString::fromUtf8("-"));
    m_zoomOutBtn->setFixedHeight(BUTTON_SIZE);
    m_zoomOutBtn->setStyleSheet(BUTTON_STYLE);
    m_zoomOutBtn->setCursor(Qt::PointingHandCursor);
    m_zoomOutBtn->setToolTip(QString::fromUtf8("\u7f29\u5c0f"));

    m_resetBtn = new QPushButton(QString::fromUtf8("\u91cd\u7f6e"));
    m_resetBtn->setFixedHeight(BUTTON_SIZE);
    m_resetBtn->setStyleSheet(BUTTON_STYLE);
    m_resetBtn->setCursor(Qt::PointingHandCursor);
    m_resetBtn->setToolTip(QString::fromUtf8("\u91cd\u7f6e\u89c6\u56fe"));

    m_rotateLeftBtn = new QPushButton(QString::fromUtf8("\u2190"));
    m_rotateLeftBtn->setFixedHeight(BUTTON_SIZE);
    m_rotateLeftBtn->setStyleSheet(BUTTON_STYLE);
    m_rotateLeftBtn->setCursor(Qt::PointingHandCursor);
    m_rotateLeftBtn->setToolTip(QString::fromUtf8("\u5de6\u8f6c90\u00b0"));

    m_rotateRightBtn = new QPushButton(QString::fromUtf8("\u2192"));
    m_rotateRightBtn->setFixedHeight(BUTTON_SIZE);
    m_rotateRightBtn->setStyleSheet(BUTTON_STYLE);
    m_rotateRightBtn->setCursor(Qt::PointingHandCursor);
    m_rotateRightBtn->setToolTip(QString::fromUtf8("\u53f3\u8f6c90\u00b0"));
    
    m_zoomLabel = new QLabel("100%");
    m_zoomLabel->setStyleSheet(LABEL_STYLE);
    m_zoomLabel->setMinimumWidth(60);
    m_zoomLabel->setAlignment(Qt::AlignCenter);
    
    toolbarLayout->addWidget(m_zoomOutBtn);
    toolbarLayout->addWidget(m_zoomInBtn);
    toolbarLayout->addWidget(m_resetBtn);
    toolbarLayout->addSpacing(16);
    toolbarLayout->addWidget(m_rotateLeftBtn);
    toolbarLayout->addWidget(m_rotateRightBtn);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(m_zoomLabel);
    toolbarLayout->addStretch();
    
    m_closeBtn = new QPushButton(QString::fromUtf8("\u5173\u95ed"));
    m_closeBtn->setFixedSize(60, BUTTON_SIZE);
    m_closeBtn->setStyleSheet(BUTTON_STYLE + "QPushButton { color: #ef4444; }");
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setToolTip(QString::fromUtf8("\u5173\u95ed\u7a97\u53e3"));
    toolbarLayout->addWidget(m_closeBtn);
    
    mainLayout->addWidget(m_scrollArea, 1);
    mainLayout->addWidget(toolbar);
}

void ImageViewerDialog::setupConnections()
{
    connect(m_zoomInBtn, &QPushButton::clicked, this, &ImageViewerDialog::zoomIn);
    connect(m_zoomOutBtn, &QPushButton::clicked, this, &ImageViewerDialog::zoomOut);
    connect(m_resetBtn, &QPushButton::clicked, this, &ImageViewerDialog::resetZoom);
    connect(m_rotateLeftBtn, &QPushButton::clicked, this, &ImageViewerDialog::rotateLeft);
    connect(m_rotateRightBtn, &QPushButton::clicked, this, &ImageViewerDialog::rotateRight);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::close);
}

void ImageViewerDialog::setImage(const QString &imagePath)
{
    QPixmap pixmap;
    
    if (imagePath.startsWith("http://") || imagePath.startsWith("https://")) {
        if (!loadFromNetworkUrl(imagePath, pixmap)) {
            return;
        }
    } else {
        if (!loadFromLocalFile(imagePath, pixmap)) {
            return;
        }
    }
    
    setImage(pixmap);
}

bool ImageViewerDialog::loadFromNetworkUrl(const QString &url, QPixmap &outPixmap)
{
    QNetworkRequest request{QUrl(url)};
    QNetworkReply* reply = SharedNetworkManager::instance()->manager()->get(request);
    
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    if (reply->error() != QNetworkReply::NoError) {
        LOG_WARNING("ImageViewerDialog", QString("Network error: %1").arg(reply->errorString()));
        m_imageLabel->setText(QString::fromUtf8("\u52a0\u8f7d\u5931\u8d25\uff1a\u7f51\u7edc\u9519\u8bef"));
        reply->deleteLater();
        return false;
    }
    
    QByteArray imageData = reply->readAll();
    reply->deleteLater();
    
    if (!outPixmap.loadFromData(imageData)) {
        LOG_WARNING("ImageViewerDialog", "Failed to parse image data");
        m_imageLabel->setText(QString::fromUtf8("加载失败：图片格式错误"));
        return false;
    }
    
    return true;
}

bool ImageViewerDialog::loadFromLocalFile(const QString &path, QPixmap &outPixmap)
{
    QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        LOG_WARNING("ImageViewerDialog", QString("File not found: %1").arg(path));
        m_imageLabel->setText(QString::fromUtf8("加载失败：文件不存在"));
        return false;
    }
    
    outPixmap = QPixmap(path);
    if (outPixmap.isNull()) {
        LOG_WARNING("ImageViewerDialog", QString("Failed to load image: %1").arg(path));
        m_imageLabel->setText(QString::fromUtf8("加载失败：图片无法打开"));
        return false;
    }
    
    return true;
}

void ImageViewerDialog::setImage(const QPixmap &pixmap)
{
    m_originalPixmap = pixmap;
    m_scaleFactor = 1.0;
    m_rotation = 0;
    
    QSize screenSize = size();
    double scaleX = (screenSize.width() - 100) / (double)pixmap.width();
    double scaleY = (screenSize.height() - TOOLBAR_HEIGHT - 100) / (double)pixmap.height();
    m_scaleFactor = qMin(scaleX, scaleY);
    m_scaleFactor = qMin(m_scaleFactor, 1.0);
    
    updateImage();
    updateZoomLabel();
}

void ImageViewerDialog::updateImage()
{
    if (m_originalPixmap.isNull()) return;
    
    QSize scaledSize = m_originalPixmap.size() * m_scaleFactor;
    QPixmap scaledPixmap = m_originalPixmap.scaled(
        scaledSize, 
        Qt::KeepAspectRatio, 
        Qt::SmoothTransformation
    );
    
    if (m_rotation != 0) {
        QTransform transform;
        transform.rotate(m_rotation);
        scaledPixmap = scaledPixmap.transformed(transform, Qt::SmoothTransformation);
    }
    
    m_imageLabel->setPixmap(scaledPixmap);
    m_imageLabel->resize(scaledPixmap.size());
    
    QScrollBar *hBar = m_scrollArea->horizontalScrollBar();
    QScrollBar *vBar = m_scrollArea->verticalScrollBar();
    hBar->setValue((hBar->maximum() - hBar->minimum()) / 2);
    vBar->setValue((vBar->maximum() - vBar->minimum()) / 2);
}

void ImageViewerDialog::updateZoomLabel()
{
    m_zoomLabel->setText(QString::number(qRound(m_scaleFactor * 100)) + "%");
}

void ImageViewerDialog::zoomIn()
{
    if (m_scaleFactor < MAX_SCALE) {
        m_scaleFactor = qMin(m_scaleFactor + SCALE_STEP, MAX_SCALE);
        updateImage();
        updateZoomLabel();
    }
}

void ImageViewerDialog::zoomOut()
{
    if (m_scaleFactor > MIN_SCALE) {
        m_scaleFactor = qMax(m_scaleFactor - SCALE_STEP, MIN_SCALE);
        updateImage();
        updateZoomLabel();
    }
}

void ImageViewerDialog::resetZoom()
{
    m_scaleFactor = 1.0;
    m_rotation = 0;
    updateImage();
    updateZoomLabel();
}

void ImageViewerDialog::rotateLeft()
{
    m_rotation = (m_rotation - 90 + 360) % 360;
    updateImage();
}

void ImageViewerDialog::rotateRight()
{
    m_rotation = (m_rotation + 90) % 360;
    updateImage();
}

void ImageViewerDialog::closeDialog()
{
    close();
}

void ImageViewerDialog::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().y() > 0) {
        zoomIn();
    } else {
        zoomOut();
    }
    event->accept();
}

void ImageViewerDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QWidget *clickedWidget = childAt(event->pos());
        
        if (clickedWidget == m_scrollArea || clickedWidget == nullptr) {
            close();
            return;
        }
        
        m_dragging = true;
        m_lastPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
    QDialog::mousePressEvent(event);
}

void ImageViewerDialog::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging) {
        QPoint delta = event->pos() - m_lastPos;
        m_lastPos = event->pos();
        
        QScrollBar *hBar = m_scrollArea->horizontalScrollBar();
        QScrollBar *vBar = m_scrollArea->verticalScrollBar();
        hBar->setValue(hBar->value() - delta.x());
        vBar->setValue(vBar->value() - delta.y());
    }
    QDialog::mouseMoveEvent(event);
}

void ImageViewerDialog::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        setCursor(Qt::ArrowCursor);
    }
    QDialog::mouseReleaseEvent(event);
}

void ImageViewerDialog::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
        case Qt::Key_Escape:
            close();
            break;
        case Qt::Key_Plus:
        case Qt::Key_Equal:
            zoomIn();
            break;
        case Qt::Key_Minus:
            zoomOut();
            break;
        case Qt::Key_0:
            resetZoom();
            break;
        case Qt::Key_L:
            rotateLeft();
            break;
        case Qt::Key_R:
            rotateRight();
            break;
        default:
            QDialog::keyPressEvent(event);
    }
}

void ImageViewerDialog::showImage(QWidget *parent, const QString &imagePath)
{
    ImageViewerDialog dialog(parent);
    dialog.setImage(imagePath);
    dialog.exec();
}

void ImageViewerDialog::showImage(QWidget *parent, const QPixmap &pixmap)
{
    ImageViewerDialog dialog(parent);
    dialog.setImage(pixmap);
    dialog.exec();
}

#include "components/PanelCard.h"
#include "components/PanelEditorWidget.h"
#include "components/EditorStyles.h"
#include "utils/AsyncImageLoader.h"
#include "data/FileStorage.h"
#include "utils/Logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QEvent>
#include <QGraphicsDropShadowEffect>
#include <QPixmap>
#include <QCryptographicHash>
#include <QFileInfo>

namespace {
    using EditorStyles::TRANSPARENT_BG;
    
    namespace Text {
        const QString PREVIEW = QString::fromUtf8("预览");
        const QString LOADING = QString::fromUtf8("加载中...");
        const QString LOAD_FAILED = QString::fromUtf8("加载失败");
    }
    
    namespace Size {
        constexpr int CARD_WIDTH = 220;
        constexpr int CARD_HEIGHT = 400;
        constexpr int PREVIEW_WIDTH = 180;
        constexpr int PREVIEW_HEIGHT = 190;
        constexpr int DESC_HEIGHT = 110;
    }
    
    QString simplifyRatio(int w, int h)
    {
        if (w <= 0 || h <= 0) return "?";
        int a = w, b = h;
        while (b != 0) {
            int t = b;
            b = a % b;
            a = t;
        }
        return QString("%1:%2").arg(w / a).arg(h / a);
    }
    
    template<typename LayoutType>
    QWidget* createContainerWidget()
    {
        QWidget *container = new QWidget();
        container->setStyleSheet(TRANSPARENT_BG);
        LayoutType *layout = new LayoutType(container);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        return container;
    }
}

PanelCard::PanelCard(int chapterNumber, int panelNumber, const QString &description, QWidget *parent)
    : EditorCardBase(parent)
    , m_chapterNumber(chapterNumber)
    , m_panelNumber(panelNumber)
    , m_description(description)
{
    setupUI();
}

PanelCard::~PanelCard()
{
    if (AsyncImageLoader::instance()) {
        AsyncImageLoader::instance()->disconnect(this);
    }
}

void PanelCard::setupUI()
{
    setupCardAppearance();
    setupMainLayout();
    setCursor(Qt::PointingHandCursor);
}

void PanelCard::setupCardAppearance()
{
    setObjectName("panelCard");
    setFrameShape(QFrame::NoFrame);
    setFixedSize(Size::CARD_WIDTH, Size::CARD_HEIGHT);
    setStyleSheet(EditorStyles::panelCardStyle());
    
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(12);
    shadow->setColor(QColor(0, 0, 0, 20));
    shadow->setOffset(0, 2);
    setGraphicsEffect(shadow);
}

void PanelCard::setupMainLayout()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);
    
    layout->addWidget(createPreviewSection());
    layout->addWidget(createNumberLabel());
    layout->addWidget(createDescriptionLabel(), 1);
}

void PanelCard::setupEditorCard()
{
    m_overlayWidget = createOverlay();
    m_overlayWidget->installEventFilter(this);
    
    m_editorWidget = new PanelEditorWidget(m_overlayWidget);
    m_editorWidget->setPanelInfo(m_chapterNumber, m_panelNumber, m_panelId);
    m_editorWidget->setSceneDescription(m_description);
    m_editorWidget->setPreviewUrl(m_previewUrl);
    
    connect(m_editorWidget, &PanelEditorWidget::sceneDescriptionChanged,
            this, &PanelCard::onSceneDescriptionChanged);
    connect(m_editorWidget, &PanelEditorWidget::editSubmitted,
            this, &PanelCard::onEditSubmitted);
    connect(m_editorWidget, &PanelEditorWidget::imageGenerated,
            this, &PanelCard::onImageGenerated);
    connect(m_editorWidget, &PanelEditorWidget::closed,
            this, &PanelCard::onEditorClosed);
    
    m_editorCard = m_editorWidget;
}

void PanelCard::syncDataToEditor()
{
    if (m_editorWidget) {
        m_editorWidget->setSceneDescription(m_description);
        m_editorWidget->setPreviewUrl(m_previewUrl);
    }
}

QWidget* PanelCard::createPreviewSection()
{
    QWidget *container = createContainerWidget<QVBoxLayout>();
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(container->layout());
    
    QLabel *titleLabel = new QLabel(QString::fromUtf8("面板 %1-%2").arg(m_chapterNumber).arg(m_panelNumber));
    titleLabel->setStyleSheet(EditorStyles::panelTitleLabelStyle());
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);
    
    m_previewLabel = new QLabel();
    m_previewLabel->setFixedSize(Size::PREVIEW_WIDTH, Size::PREVIEW_HEIGHT);
    m_previewLabel->setStyleSheet(EditorStyles::panelPreviewStyle());
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setText(Text::PREVIEW);
    m_previewLabel->installEventFilter(this);
    m_previewLabel->setCursor(Qt::PointingHandCursor);
    layout->addWidget(m_previewLabel);
    
    m_sizeLabel = new QLabel();
    m_sizeLabel->setAlignment(Qt::AlignCenter);
    m_sizeLabel->setStyleSheet("font-size: 10px; color: #9CA3AF; background: transparent; padding: 0px; margin: 0px;");
    m_sizeLabel->hide();
    layout->addWidget(m_sizeLabel);
    
    return container;
}

QWidget* PanelCard::createNumberLabel()
{
    QWidget *container = createContainerWidget<QHBoxLayout>();
    QHBoxLayout *layout = qobject_cast<QHBoxLayout*>(container->layout());
    
    m_numberLabel = new QLabel(QString::fromUtf8("面板 %1-%2").arg(m_chapterNumber).arg(m_panelNumber));
    m_numberLabel->setStyleSheet(EditorStyles::panelNumberLabelStyle());
    layout->addWidget(m_numberLabel);
    
    return container;
}

QWidget* PanelCard::createDescriptionLabel()
{
    QWidget *container = createContainerWidget<QVBoxLayout>();
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(container->layout());
    
    m_descLabel = new QLabel();
    m_descLabel->setWordWrap(true);
    m_descLabel->setText(m_description);
    m_descLabel->setStyleSheet(EditorStyles::panelDescLabelStyle());
    m_descLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_descLabel->setMinimumHeight(Size::DESC_HEIGHT);
    m_descLabel->setMaximumHeight(Size::DESC_HEIGHT);
    layout->addWidget(m_descLabel);
    
    return container;
}

void PanelCard::setDescription(const QString &description)
{
    m_description = description;
    if (m_descLabel) {
        m_descLabel->setText(description);
    }
}

void PanelCard::setPreviewUrl(const QString &url)
{
    m_previewUrl = url;
    m_currentImagePath = url;
    
    if (url.isEmpty()) {
        setPreviewState(PreviewState::Empty);
        m_previewLabel->setCursor(Qt::ArrowCursor);
        return;
    }

    QString loadPath = url;
    if (!url.startsWith("http://") && !url.startsWith("https://")) {
        const QFileInfo relativeInfo(url);
        if (!relativeInfo.exists() && FileStorage::instance()) {
            const QString fullPath = FileStorage::instance()->getFullPath(url);
            if (QFileInfo(fullPath).exists()) {
                loadPath = fullPath;
                m_currentImagePath = fullPath;
            }
        }
    }
    
    QString urlHash = QString(QCryptographicHash::hash(loadPath.toUtf8(), QCryptographicHash::Md5).toHex());
    QString id = QString("panel_%1_%2_%3")
        .arg(m_chapterNumber).arg(m_panelNumber).arg(urlHash);
    QSize targetSize(Size::PREVIEW_WIDTH, Size::PREVIEW_HEIGHT);
    QString cacheKey = AsyncImageLoader::makeCacheKey(id, targetSize);

    QPixmap cachedPixmap;
    if (AsyncImageLoader::instance()->findCached(cacheKey, &cachedPixmap)) {
        setPreviewPixmap(cachedPixmap);
        setPreviewState(PreviewState::Loaded);
        m_previewLabel->setCursor(Qt::PointingHandCursor);
        return;
    }

    setPreviewState(PreviewState::Loading);
    m_previewLabel->setCursor(Qt::ArrowCursor);
    m_loadingImageId = id;

    disconnect(AsyncImageLoader::instance(), nullptr, this, nullptr);
    connect(AsyncImageLoader::instance(), &AsyncImageLoader::imageLoaded,
            this, &PanelCard::onAsyncImageLoaded);

    AsyncImageLoader::instance()->loadAsync(id, loadPath, targetSize);
}

void PanelCard::onAsyncImageLoaded(const QString& id, const QString& cacheKey, const QPixmap& pixmap)
{
    if (id != m_loadingImageId) {
        return;
    }

    if (!pixmap.isNull()) {
        AsyncImageLoader::instance()->insertCache(cacheKey, pixmap);
        setPreviewPixmap(pixmap);
        setPreviewState(PreviewState::Loaded);
        m_previewLabel->setCursor(Qt::PointingHandCursor);
    } else {
        setPreviewState(PreviewState::Error);
        m_previewLabel->setCursor(Qt::ArrowCursor);
    }
}

QPixmap PanelCard::loadPixmapFromUrl(const QString &url)
{
    QPixmap pixmap;
    
    if (url.startsWith("data:image")) {
        QString base64Data = url.section(",", 1);
        QByteArray imageData = QByteArray::fromBase64(base64Data.toLatin1());
        pixmap.loadFromData(imageData);
    } else {
        pixmap.load(url);
    }
    
    return pixmap;
}

void PanelCard::setPreviewPixmap(const QPixmap &pixmap)
{
    if (m_previewLabel) {
        m_previewLabel->setPixmap(pixmap);
        m_previewLabel->setText("");
        m_previewLabel->setStyleSheet("background: transparent; border: none;");
    }
    
    if (m_editorWidget) {
        m_editorWidget->setPreviewPixmap(pixmap);
    }
}

void PanelCard::setImageSize(int width, int height)
{
    if (!m_sizeLabel) return;
    
    if (width <= 0 || height <= 0) {
        m_sizeLabel->hide();
        return;
    }
    
    QString ratio = simplifyRatio(width, height);
    m_sizeLabel->setText(QString("%1x%2 (%3)").arg(width).arg(height).arg(ratio));
    m_sizeLabel->show();
}

void PanelCard::setPreviewText(const QString &text)
{
    if (m_previewLabel) {
        m_previewLabel->setText(text);
        m_previewLabel->setPixmap(QPixmap());
    }
}

void PanelCard::setPreviewState(PreviewState state)
{
    m_previewState = state;
    
    switch (state) {
        case PreviewState::Empty:
            setPreviewText(Text::PREVIEW);
            m_previewLabel->setStyleSheet(EditorStyles::panelPreviewStyle());
            break;
        case PreviewState::Loading:
            setPreviewText(Text::LOADING);
            m_previewLabel->setStyleSheet(EditorStyles::panelPreviewStyle());
            break;
        case PreviewState::Loaded:
            break;
        case PreviewState::Error:
            setPreviewText(Text::LOAD_FAILED);
            m_previewLabel->setStyleSheet(EditorStyles::panelPreviewStyle());
            break;
    }
}

bool PanelCard::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        if (watched == m_overlayWidget) {
            hideEditorCard();
            return true;
        }
        if (watched == m_previewLabel && !m_currentImagePath.isEmpty()) {
            emit imageClicked(m_currentImagePath);
            return true;
        }
    }
    return EditorCardBase::eventFilter(watched, event);
}

void PanelCard::mousePressEvent(QMouseEvent *event)
{
    EditorCardBase::mousePressEvent(event);
    showEditorCard();
    emit clicked(m_panelNumber);
}

void PanelCard::onSceneDescriptionChanged(const QString &description)
{
    m_description = description;
    if (m_descLabel) {
        m_descLabel->setText(description);
    }
    emit dataChanged(m_panelNumber, description);
}

void PanelCard::onEditSubmitted(int editMode, const QString &instruction, const QString &maskPath)
{
    Q_UNUSED(editMode)
    Q_UNUSED(instruction)
    Q_UNUSED(maskPath)
}

void PanelCard::onImageGenerated(const QString &imageUrl)
{
    setPreviewUrl(imageUrl);
    emit imageGenerated(m_panelNumber, imageUrl);
}

void PanelCard::onEditorClosed()
{
    hideEditorCard();
}

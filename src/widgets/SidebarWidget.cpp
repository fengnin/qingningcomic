#include "widgets/SidebarWidget.h"
#include "widgets/FortuneCookieWidget.h"
#include <QPainter>
#include <QLinearGradient>
#include <QFontMetrics>

SidebarWidget::SidebarWidget(QWidget *parent)
    : QWidget(parent)
    , m_navList(nullptr)
    , m_activeIndex(0)
{
    setMinimumWidth(SidebarStyle::MIN_WIDTH);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAutoFillBackground(false);
    setAttribute(Qt::WA_StyledBackground, false);
    initUI();
}

void SidebarWidget::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setupMargins(mainLayout, 0, 0, 0, 0);
    
    createBrandSection(mainLayout);
    createNavSection(mainLayout);
    createFortuneSection(mainLayout);
    createFooterSection(mainLayout);
    
    setActiveNav(0);
}

void SidebarWidget::createBrandSection(QVBoxLayout *parentLayout)
{
    parentLayout->addSpacing(20);
    
    QWidget *brandContainer = createTransparentWidget();
    QHBoxLayout *brandLayout = new QHBoxLayout(brandContainer);
    setupMargins(brandLayout, 16, 8, 12, 4, 6);
    
    QLabel *logoIcon = createLabel(QStringLiteral("🍋"),
        "font-size: 28px; background: transparent; border: none; outline: none; min-width: 48px; max-width: 48px; min-height: 48px; max-height: 48px;");
    logoIcon->setFont(QFont(QStringLiteral("Segoe UI Emoji"), 28));
    
    QWidget *textContainer = createTransparentWidget();
    QVBoxLayout *textLayout = new QVBoxLayout(textContainer);
    setupMargins(textLayout, 0, 2, 0, 0, 1);
    
    QLabel *brandName = createLabel(QString::fromUtf8("青柠漫画"),
        "font-size: 18px; font-weight: bold; color: #451a03; background: transparent;", 18, true);
    
    QLabel *brandSubtitle = createLabel(QString::fromUtf8("全流程创作工作台"),
        "font-size: 12px; color: #5c3d1e; background: transparent;", 12);
    
    textLayout->addWidget(brandName);
    textLayout->addWidget(brandSubtitle);
    
    brandLayout->addWidget(logoIcon);
    brandLayout->addWidget(textContainer);
    brandLayout->addStretch();
    
    parentLayout->addWidget(brandContainer, 0, Qt::AlignTop | Qt::AlignLeft);
    parentLayout->addSpacing(16);
}

void SidebarWidget::createNavSection(QVBoxLayout *parentLayout)
{
    m_navList = new QListWidget(this);
    m_navList->setObjectName("navList");
    m_navList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_navList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_navList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_navList->setFocusPolicy(Qt::NoFocus);
    m_navList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_navList->setFrameShape(QFrame::NoFrame);
    m_navList->setSpacing(3);
    m_navList->setIconSize(QSize(SidebarStyle::NAV_ICON_SIZE, SidebarStyle::NAV_ICON_SIZE));
    m_navList->setStyleSheet("QListWidget { background: transparent; border: none; }");
    m_navList->setAttribute(Qt::WA_TranslucentBackground);
    
    connect(m_navList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        int index = item->data(Qt::UserRole).toInt();
        if (index != m_activeIndex && index >= 0 && index < m_navItems.size()) {
            setActiveNav(index);
            emit navItemClicked(m_navItems[index].pageIndex, m_navItems[index].breadcrumb);
        }
    });
    
    addNavItem({QStringLiteral("🏠"), QString::fromUtf8("总览"), QString::fromUtf8("总览"), 0});
    addNavItem({QStringLiteral("📄"), QString::fromUtf8("上传作品"), QString::fromUtf8("上传作品"), 1});
    addNavItem({QString::fromUtf8("📚"), QString::fromUtf8("项目空间"), QString::fromUtf8("项目空间"), 2});
    addNavItem({QStringLiteral("📦"), QString::fromUtf8("导出中心"), QString::fromUtf8("导出中心"), 3});
    
    parentLayout->addWidget(m_navList, 1);
}

void SidebarWidget::createFortuneSection(QVBoxLayout *parentLayout)
{
    FortuneCookieWidget *fortuneCookie = new FortuneCookieWidget();
    parentLayout->addWidget(fortuneCookie);
    parentLayout->addSpacing(10);
}

void SidebarWidget::createFooterSection(QVBoxLayout *parentLayout)
{
    QWidget *footerContainer = createTransparentWidget();
    QHBoxLayout *footerLayout = new QHBoxLayout(footerContainer);
    setupMargins(footerLayout, 16, 10, 12, 10, 5);
    
    QLabel *footerLabel = createLabel(
        QString::fromUtf8("连接故事、角色、分镜的统一控制中心"),
        "background: transparent; color: " + SidebarStyle::Color::FOOTER_LABEL + "; font-size: 13px;", 13);
    footerLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    
    footerLayout->addWidget(footerLabel, 1);
    
    parentLayout->addWidget(footerContainer, 0, Qt::AlignBottom);
}

void SidebarWidget::addNavItem(const NavItemData &itemData)
{
    if (!m_navList) return;
    
    m_navItems.append(itemData);
    
    QListWidgetItem *item = new QListWidgetItem(m_navList);
    item->setData(Qt::UserRole, m_navItems.size() - 1);
    item->setSizeHint(QSize(0, SidebarStyle::NAV_ITEM_HEIGHT));
    
    QWidget *itemWidget = createTransparentWidget();
    QHBoxLayout *layout = new QHBoxLayout(itemWidget);
    setupMargins(layout, 12, 0, 12, 0, 10);
    
    QLabel *iconLabel = createLabel(itemData.icon,
        "font-size: 22px; background: transparent; border: none;", 22);
    iconLabel->setObjectName("navIcon");
    iconLabel->setFixedWidth(32);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setFont(QFont(QStringLiteral("Segoe UI Emoji"), 22));
    
    QLabel *textLabel = createLabel(itemData.text,
        "font-size: 14px; background: transparent; color: #78350f; border: none;", 14);
    textLabel->setObjectName("navText");
    textLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    
    layout->addWidget(iconLabel);
    layout->addWidget(textLabel, 1);
    
    m_navList->setItemWidget(item, itemWidget);
}

void SidebarWidget::updateNavItemStyle(QWidget *itemWidget, bool isActive)
{
    if (!itemWidget) return;
    
    QLabel *textLabel = itemWidget->findChild<QLabel*>("navText");
    if (textLabel) {
        if (isActive) {
            textLabel->setStyleSheet(
                "font-size: 14px; background: transparent; color: #92400e; border: none;");
        } else {
            textLabel->setStyleSheet(
                "font-size: 14px; background: transparent; color: #78350f; border: none;");
        }
    }
    
    if (isActive) {
        itemWidget->setStyleSheet(
            "QWidget { background: #fef3c7; border-left: 4px solid #f59e0b; border-radius: 0 8px 8px 0; }");
    } else {
        itemWidget->setStyleSheet(
            "QWidget { background: transparent; border-left: 4px solid transparent; border-radius: 0 8px 8px 0; }");
    }
}

void SidebarWidget::applyLabelStyles(QWidget *itemWidget, bool isActive)
{
    updateNavItemStyle(itemWidget, isActive);
}

void SidebarWidget::setActiveNav(int index)
{
    m_activeIndex = index;
    
    for (int i = 0; i < m_navList->count(); ++i) {
        QListWidgetItem *listItem = m_navList->item(i);
        QWidget *widget = m_navList->itemWidget(listItem);
        updateNavItemStyle(widget, (i == index));
    }
    
    if (index >= 0 && index < m_navList->count()) {
        m_navList->setCurrentRow(index);
    }
}

void SidebarWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    QLinearGradient gradient(0, 0, 0, height());
    gradient.setColorAt(0, QColor(255, 251, 235));
    gradient.setColorAt(1, QColor(254, 243, 199));
    painter.fillRect(rect(), gradient);
}

QWidget* SidebarWidget::createTransparentWidget() const
{
    QWidget *widget = new QWidget();
    widget->setStyleSheet("background: transparent;");
    return widget;
}

QLabel* SidebarWidget::createLabel(const QString &text, const QString &style, int fontSize, bool bold) const
{
    QLabel *label = new QLabel(text);
    QFont font(bold ? QStringLiteral("Microsoft YaHei") : QStringLiteral("Segoe UI"), fontSize > 0 ? fontSize : -1);
    if (bold) font.setBold(true);
    label->setFont(font);
    label->setStyleSheet(style);
    label->setWordWrap(false);
    return label;
}

void SidebarWidget::setupMargins(QBoxLayout *layout, int left, int top, int right, int bottom, int spacing) const
{
    layout->setContentsMargins(left, top, right, bottom);
    if (spacing > 0) layout->setSpacing(spacing);
}

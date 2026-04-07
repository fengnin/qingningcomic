#ifndef SIDEBARWIDGET_H
#define SIDEBARWIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QListWidgetItem>
#include <QVector>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPaintEvent>

class FortuneCookieWidget;

struct NavItemData {
    QString icon;
    QString text;
    QString breadcrumb;
    int pageIndex;
};

namespace SidebarStyle {
    constexpr int MIN_WIDTH = 260;
    constexpr int NAV_ITEM_HEIGHT = 56;
    constexpr int NAV_ICON_SIZE = 24;
    
    namespace Color {
        const QString GRADIENT_START = "#fffbef";
        const QString GRADIENT_END = "#fef3c7";
        const QString BRAND_NAME = "#451a03";
        const QString BRAND_SUBTITLE = "#5c3d1e";
        const QString FOOTER_LABEL = "#b45309";
    }
}

class SidebarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SidebarWidget(QWidget *parent = nullptr);
    
signals:
    void navItemClicked(int pageIndex, const QString &breadcrumb);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void initUI();
    void createBrandSection(QVBoxLayout *parentLayout);
    void createNavSection(QVBoxLayout *parentLayout);
    void createFortuneSection(QVBoxLayout *parentLayout);
    void createFooterSection(QVBoxLayout *parentLayout);
    
    QWidget* createTransparentWidget() const;
    QLabel* createLabel(const QString &text, const QString &style, int fontSize = -1, bool bold = false) const;
    void setupMargins(QBoxLayout *layout, int left, int top, int right, int bottom, int spacing = 0) const;
    
    void addNavItem(const NavItemData &itemData);
    void updateNavItemStyle(QWidget *itemWidget, bool isActive);
    void applyLabelStyles(QWidget *itemWidget, bool isActive);
    void setActiveNav(int index);
    
    QListWidget *m_navList;
    QVector<NavItemData> m_navItems;
    int m_activeIndex;
};

#endif // SIDEBARWIDGET_H

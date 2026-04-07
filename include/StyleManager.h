#ifndef STYLEMANAGER_H
#define STYLEMANAGER_H

#include <QString>
#include <QMap>

/**
 * @brief 样式管理器
 * 
 * 集中管理应用程序的样式和颜色常量。
 */
class StyleManager
{
public:
    static const QString SIDEBAR_GRADIENT_START;
    static const QString SIDEBAR_GRADIENT_END;
    
    static const QString NAV_ITEM_ACTIVE_BG;
    static const QString NAV_ITEM_ACTIVE_TEXT;
    static const QString NAV_ITEM_ACTIVE_INDICATOR;
    static const QString NAV_ITEM_INACTIVE_TEXT;
    
    static const QString HEADER_BACKGROUND;
    static const QString HEADER_STATUS_BG;
    static const QString HEADER_STATUS_TEXT;
    static const QString HEADER_META_TEXT;
    
    static const QString PRIMARY_COLOR;
    static const QString PRIMARY_COLOR_HOVER;
    static const QString PRIMARY_TEXT;
    
    static const QString NEUTRAL_WHITE;
    static const QString NEUTRAL_GRAY_50;
    static const QString NEUTRAL_GRAY_200;
    static const QString NEUTRAL_GRAY_500;
    static const QString NEUTRAL_GRAY_700;
    static const QString NEUTRAL_GRAY_900;
    
    static const QString STATUS_SUCCESS_BACKGROUND;
    static const QString STATUS_SUCCESS_TEXT;
    static const QString STATUS_WARNING_BACKGROUND;
    static const QString STATUS_WARNING_TEXT;
    static const QString STATUS_ERROR_BACKGROUND;
    static const QString STATUS_ERROR_TEXT;
    static const QString STATUS_INFO_BACKGROUND;
    static const QString STATUS_INFO_TEXT;
    
    static const QString CHAPTER_ACTIVE_BORDER;
    static const QString CHAPTER_ACTIVE_BG;
    static const QString CHAPTER_INACTIVE_BORDER;
    static const QString CHAPTER_INACTIVE_BG;
    static const QString CHAPTER_META_TEXT;
    
    static const QString BUTTON_PRIMARY_BG;
    static const QString BUTTON_PRIMARY_HOVER;
    static const QString BUTTON_SECONDARY_BG;
    static const QString BUTTON_SECONDARY_HOVER;
    static const QString BUTTON_REFRESH_BG;
    static const QString BUTTON_REFRESH_HOVER;
    
    static QString getPrimaryButtonStyle();
    static QString getSecondaryButtonStyle();
    static QString getInputStyle();
    static QString getCardStyle();
    static QString getStatusLabelStyle(const QString &status);
    static QString getPageBackgroundStyle();
    static QString getSidebarStyle();
    static QString getSectionCardStyle();
    static QString getChapterButtonActiveStyle();
    static QString getChapterButtonInactiveStyle();
    static QString getChapterButtonStyle(bool isActive);
    static QString getHeaderButtonStyle();
    static QString getRefreshButtonStyle();
    static QString getFeatureCardStyle();
    
    static QString getNavItemActiveStyle();
    static QString getNavItemInactiveStyle();
    static QString getNavListStyle();
    
    static QString getNavItemWidgetActiveStyle();
    static QString getNavItemWidgetInactiveStyle();
    static QString getNavIconLabelStyle(bool isActive);
    static QString getNavTextLabelStyle(bool isActive);

private:
    StyleManager() = delete;
    ~StyleManager() = delete;
    
    struct StatusColors {
        QString background;
        QString text;
    };
    
    static const QMap<QString, StatusColors>& getStatusColorMap();
    static QString buildButtonStyle(const QString& bgColor, const QString& textColor, 
                                    const QString& hoverColor, const QString& padding = "10px 20px");
    static QString buildChapterButtonStyle(const QString& bgColor, const QString& borderColor);
};

#endif // STYLEMANAGER_H

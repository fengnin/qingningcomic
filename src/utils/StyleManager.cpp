#include "utils/StyleManager.h"

const QString StyleManager::SIDEBAR_GRADIENT_START = "#fffbef";
const QString StyleManager::SIDEBAR_GRADIENT_END = "#fef3c7";

const QString StyleManager::NAV_ITEM_ACTIVE_BG = "#fef3c7";
const QString StyleManager::NAV_ITEM_ACTIVE_TEXT = "#92400e";
const QString StyleManager::NAV_ITEM_ACTIVE_INDICATOR = "#f59e0b";
const QString StyleManager::NAV_ITEM_INACTIVE_TEXT = "#78350f";

const QString StyleManager::HEADER_BACKGROUND = "#fffbeb";
const QString StyleManager::HEADER_STATUS_BG = "#fef3c7";
const QString StyleManager::HEADER_STATUS_TEXT = "#92400e";
const QString StyleManager::HEADER_META_TEXT = "#57534e";

const QString StyleManager::PRIMARY_COLOR = "#f59e0b";
const QString StyleManager::PRIMARY_COLOR_HOVER = "#d97706";
const QString StyleManager::PRIMARY_TEXT = "#78350f";

const QString StyleManager::NEUTRAL_WHITE = "#ffffff";
const QString StyleManager::NEUTRAL_GRAY_50 = "#fafafa";
const QString StyleManager::NEUTRAL_GRAY_200 = "#e5e5e5";
const QString StyleManager::NEUTRAL_GRAY_500 = "#737373";
const QString StyleManager::NEUTRAL_GRAY_700 = "#404040";
const QString StyleManager::NEUTRAL_GRAY_900 = "#171717";

const QString StyleManager::STATUS_SUCCESS_BACKGROUND = "#dcfce7";
const QString StyleManager::STATUS_SUCCESS_TEXT = "#166534";
const QString StyleManager::STATUS_WARNING_BACKGROUND = "#fef3c7";
const QString StyleManager::STATUS_WARNING_TEXT = "#92400e";
const QString StyleManager::STATUS_ERROR_BACKGROUND = "#fee2e2";
const QString StyleManager::STATUS_ERROR_TEXT = "#b91c1c";
const QString StyleManager::STATUS_INFO_BACKGROUND = "#fef3c7";
const QString StyleManager::STATUS_INFO_TEXT = "#92400e";

const QString StyleManager::CHAPTER_ACTIVE_BORDER = "#f59e0b";
const QString StyleManager::CHAPTER_ACTIVE_BG = "#fffbeb";
const QString StyleManager::CHAPTER_INACTIVE_BORDER = "#e5e5e5";
const QString StyleManager::CHAPTER_INACTIVE_BG = "#ffffff";
const QString StyleManager::CHAPTER_META_TEXT = "#737373";

const QString StyleManager::BUTTON_PRIMARY_BG = "#f59e0b";
const QString StyleManager::BUTTON_PRIMARY_HOVER = "#d97706";
const QString StyleManager::BUTTON_SECONDARY_BG = "#fbbf24";
const QString StyleManager::BUTTON_SECONDARY_HOVER = "#f59e0b";
const QString StyleManager::BUTTON_REFRESH_BG = "#fffbeb";
const QString StyleManager::BUTTON_REFRESH_HOVER = "#fef3c7";

const QMap<QString, StyleManager::StatusColors>& StyleManager::getStatusColorMap()
{
    static const QMap<QString, StatusColors> colorMap = {
        {"success", {STATUS_SUCCESS_BACKGROUND, STATUS_SUCCESS_TEXT}},
        {"warning", {STATUS_WARNING_BACKGROUND, STATUS_WARNING_TEXT}},
        {"error",   {STATUS_ERROR_BACKGROUND, STATUS_ERROR_TEXT}},
        {"info",    {STATUS_INFO_BACKGROUND, STATUS_INFO_TEXT}}
    };
    return colorMap;
}

QString StyleManager::buildButtonStyle(const QString& bgColor, const QString& textColor, 
                                        const QString& hoverColor, const QString& padding)
{
    return QString(
        "QPushButton {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: none;"
        "  padding: %3;"
        "  border-radius: 6px;"
        "  font-size: 14px;"
        "  font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "  background-color: %4;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #e0e0e0;"
        "  color: #9e9e9e;"
        "}"
    ).arg(bgColor, textColor, padding, hoverColor);
}

QString StyleManager::buildChapterButtonStyle(const QString& bgColor, const QString& borderColor)
{
    return QString(
        "QPushButton {"
        "  background-color: %1;"
        "  border: 1px solid %2;"
        "  border-radius: 8px;"
        "  padding: 16px;"
        "  text-align: left;"
        "}"
    ).arg(bgColor, borderColor);
}

QString StyleManager::getPrimaryButtonStyle()
{
    return buildButtonStyle(BUTTON_PRIMARY_BG, PRIMARY_TEXT, BUTTON_PRIMARY_HOVER);
}

QString StyleManager::getSecondaryButtonStyle()
{
    return buildButtonStyle(BUTTON_SECONDARY_BG, PRIMARY_TEXT, BUTTON_SECONDARY_HOVER);
}

QString StyleManager::getInputStyle()
{
    return "QLineEdit, QTextEdit, QComboBox {"
           "  border: 1px solid #d4d4d4;"
           "  border-radius: 8px;"
           "  padding: 10px 12px;"
           "  font-size: 14px;"
           "  background-color: #ffffff;"
           "  color: #171717;"
           "}"
           "QLineEdit:focus, QTextEdit:focus, QComboBox:focus {"
           "  border-color: #f59e0b;"
           "}"
           "QComboBox::drop-down {"
           "  border: none;"
           "  width: 20px;"
           "}"
           "QComboBox::down-arrow {"
           "  width: 12px;"
           "  height: 12px;"
           "}";
}

QString StyleManager::getCardStyle()
{
    return "QWidget {"
           "  background-color: #ffffff;"
           "  border-radius: 12px;"
           "  border: 1px solid #e5e5e5;"
           "}";
}

QString StyleManager::getStatusLabelStyle(const QString &status)
{
    const auto& colorMap = getStatusColorMap();
    StatusColors colors = colorMap.value(status, {STATUS_INFO_BACKGROUND, STATUS_INFO_TEXT});
    
    return QString("QLabel {"
                  "  background-color: %1;"
                  "  color: %2;"
                  "  padding: 4px 8px;"
                  "  border-radius: 4px;"
                  "  font-size: 12px;"
                  "}").arg(colors.background, colors.text);
}

QString StyleManager::getPageBackgroundStyle()
{
    return "QWidget { background-color: #fafafa; }";
}

QString StyleManager::getSidebarStyle()
{
    return QString("QWidget {"
           "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
           "    stop:0 %1,"
           "    stop:1 %2);"
           "}").arg(SIDEBAR_GRADIENT_START, SIDEBAR_GRADIENT_END);
}

QString StyleManager::getSectionCardStyle()
{
    return "QWidget {"
           "  background-color: #ffffff;"
           "  border-radius: 12px;"
           "  border: none;"
           "}";
}

QString StyleManager::getChapterButtonActiveStyle()
{
    return buildChapterButtonStyle(CHAPTER_ACTIVE_BG, CHAPTER_ACTIVE_BORDER);
}

QString StyleManager::getChapterButtonInactiveStyle()
{
    return buildChapterButtonStyle(CHAPTER_INACTIVE_BG, CHAPTER_INACTIVE_BORDER);
}

QString StyleManager::getChapterButtonStyle(bool isActive)
{
    return isActive ? getChapterButtonActiveStyle() : getChapterButtonInactiveStyle();
}

QString StyleManager::getHeaderButtonStyle()
{
    return "QPushButton {"
           "  background-color: transparent;"
           "  color: #404040;"
           "  border: 1px solid #d4d4d4;"
           "  padding: 8px 16px;"
           "  border-radius: 6px;"
           "  font-size: 14px;"
           "}";
}

QString StyleManager::getRefreshButtonStyle()
{
    return QString("QPushButton {"
           "  background-color: %1;"
           "  color: #404040;"
           "  border: none;"
           "  padding: 8px 16px;"
           "  border-radius: 4px;"
           "  font-size: 13px;"
           "}"
           "QPushButton:hover {"
           "  background-color: %2;"
           "}").arg(BUTTON_REFRESH_BG, BUTTON_REFRESH_HOVER);
}

QString StyleManager::getFeatureCardStyle()
{
    return "QWidget {"
           "  background-color: #ffffff;"
           "  border-radius: 12px;"
           "  border: 1px solid #e5e5e5;"
           "}";
}

QString StyleManager::getNavItemActiveStyle()
{
    return QString("QPushButton {"
           "  background-color: %1;"
           "  color: %2;"
           "  border: none;"
           "  border-left: 4px solid %3;"
           "  padding: 0 25px;"
           "  min-height: 50px;"
           "  max-height: 50px;"
           "  text-align: left;"
           "  font-size: 14px;"
           "  font-weight: 500;"
           "  border-radius: 0 8px 8px 0;"
           "  margin: 2px 12px 2px 0;"
           "}").arg(NAV_ITEM_ACTIVE_BG, NAV_ITEM_ACTIVE_TEXT, NAV_ITEM_ACTIVE_INDICATOR);
}

QString StyleManager::getNavItemInactiveStyle()
{
    return "QPushButton {"
           "  background-color: transparent;"
           "  color: #78350f;"
           "  border: none;"
           "  border-left: 4px solid transparent;"
           "  padding: 0 25px;"
           "  min-height: 50px;"
           "  max-height: 50px;"
           "  text-align: left;"
           "  font-size: 14px;"
           "  border-radius: 0 8px 8px 0;"
           "  margin: 2px 12px 2px 0;"
           "}"
           "QPushButton:hover {"
           "  background-color: #fef3c7;"
           "}";
}

QString StyleManager::getNavListStyle()
{
    return "QListWidget {"
           "  background: transparent;"
           "  border: none;"
           "  outline: none;"
           "}"
           "QListWidget::item {"
           "  background: transparent;"
           "  border: none;"
           "  padding: 0px;"
           "  margin: 0px;"
           "  outline: none;"
           "}"
           "QListWidget::item:selected,"
           "QListWidget::item:hover,"
           "QListWidget::item:focus {"
           "  background: transparent;"
           "  border: none;"
           "  outline: none;"
           "}";
}

QString StyleManager::getNavItemWidgetActiveStyle()
{
    return QString("QWidget {"
           "  background-color: %1;"
           "  border-left: 4px solid %2;"
           "  border-radius: 0 8px 8px 0;"
           "  margin-left: 0px;"
           "  margin-right: 12px;"
           "}").arg(NAV_ITEM_ACTIVE_BG, NAV_ITEM_ACTIVE_INDICATOR);
}

QString StyleManager::getNavItemWidgetInactiveStyle()
{
    return "background: transparent;";
}

QString StyleManager::getNavIconLabelStyle(bool isActive)
{
    QString color = isActive ? NAV_ITEM_ACTIVE_TEXT : NAV_ITEM_INACTIVE_TEXT;
    return QString("font-size: 24px; background: transparent; "
                   "min-width: 24px; max-width: 24px; "
                   "min-height: 24px; max-height: 24px; "
                   "color: %1; border: none; outline: none;").arg(color);
}

QString StyleManager::getNavTextLabelStyle(bool isActive)
{
    QString color = isActive ? NAV_ITEM_ACTIVE_TEXT : NAV_ITEM_INACTIVE_TEXT;
    QString weight = isActive ? "font-weight: 500;" : "";
    return QString("font-size: 14px; background: transparent; color: %1; %2").arg(color, weight);
}

#ifndef EDITOR_STYLES_H
#define EDITOR_STYLES_H

#include <QString>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QLayout>
#include <QPixmap>

namespace EditorStyles {

QString scrollBarStyle();
QString scrollAreaStyle();
QString closeButtonStyle();
QString saveButtonStyle();
QString cancelButtonStyle();
QString editButtonStyle();
QString deleteButtonStyle();
QString textEditStyle();
QString lineEditStyle();
QString comboBoxStyle();
QString modeButtonStyle();
QString modeButtonActiveStyle();
QString primaryButtonStyle();
QString secondaryButtonStyle();
QString featureButtonStyle();
QString refreshButtonStyle();
QString cardStyle();
QString inputStyle();
QString spinBoxStyle();
QString novelCardEditButtonStyle();
QString novelCardDeleteButtonStyle();
QString chapterCardActiveStyle();
QString chapterCardInactiveStyle();

// 上传页面样式
QString uploadInputStyle();
QString uploadTextEditStyle();
QString uploadPrimaryButtonStyle();
QString uploadSecondaryButtonStyle();
QString uploadErrorLabelStyle();
QString uploadSuccessLabelStyle();
QString uploadProgressBarStyle();
QString uploadCardStyle();
QString uploadPageBackgroundStyle();
QString uploadScrollAreaStyle();
QString uploadTipsSectionStyle();
QString uploadHeaderIconStyle();
QString uploadDialogStyle();
QString uploadDialogCancelButtonStyle();
QString uploadDialogOkButtonStyle();
QString uploadSpinButtonStyle();
QString uploadSpinContainerStyle();
QString uploadChapterValueEditStyle();
QString uploadFormLabelStyle();
QString uploadFormHintStyle();
QString uploadTipsTitleStyle();
QString uploadTipsItemStyle();
QString uploadDialogTitleStyle();
QString uploadWaitingLabelStyle();
QString uploadPageTitleStyle();

// 小说详情页样式
QString horizontalScrollAreaStyle();
QString storyboardCardStyle();
QString panelPreviewCardStyle();

// 圣经卡片样式
QString bibleCardStyle();
QString bibleItemStyle();
QString bibleImagePlaceholderStyle();
QString bibleUploadButtonStyle();
QString bibleDeleteButtonStyle();
QString bibleDetailLabelStyle();
QString bibleNameLabelStyle();
QString bibleInputFieldLabelStyle();
QString bibleEditorOverlayStyle();
QString bibleEditorTitleStyle();
QString bibleImageHasImageStyle();

// 小说详情页Section样式
QString headerSectionStyle();
QString chapterSectionStyle();

// 分镜卡片样式
QString panelCardStyle();
QString panelPreviewStyle();
QString panelTitleLabelStyle();
QString panelNumberLabelStyle();
QString panelDescLabelStyle();
QString panelEditorPreviewStyle();
QString panelEditorInfoLabelStyle();
QString storyboardItemStyle();
QString storyboardInfoTitleStyle();
QString storyboardInfoContentStyle();
QString storyboardComboBoxStyle();
QString storyboardComboBoxBtnStyle();
QString storyboardArrowButtonStyle();

// Dashboard页面样式
QString dashboardPageBackgroundStyle();
QString dashboardHeroSectionStyle();
QString dashboardCardStyle();
QString dashboardJobItemStyle();
QString dashboardStepNumberStyle();
QString dashboardTimelineDotStyle();
QString dashboardStatCardStyle(const QString &bgColor);
QString dashboardTitleStyle();
QString dashboardDescStyle();
QString dashboardCardTitleStyle();
QString dashboardCardSubtitleStyle();
QString dashboardJobTypeStyle();
QString dashboardJobMetaStyle();
QString dashboardTimelineTitleStyle();
QString dashboardTimelineMetaStyle();
QString dashboardStepTitleStyle();
QString dashboardStepDescStyle();
QString dashboardFooterLabelStyle();
QString dashboardFilterLabelStyle();
QString dashboardIconStyle();
QString dashboardActiveJobsFooterStyle();
QString dashboardStatCardLabelStyle();
QString dashboardStatCardValueStyle();
QString dashboardEmptyStateStyle();

namespace Constants {
    constexpr int BTN_HEIGHT = 40;
    constexpr int CARD_PADDING = 24;
    constexpr int CARD_SPACING = 16;
    constexpr int INPUT_HEIGHT = 36;
    constexpr int BTN_JUMP_WIDTH = 80;
    constexpr int BTN_CREATE_WIDTH = 160;
    constexpr int GRID_COLUMNS = 3;
    constexpr int STAT_CARD_WIDTH = 120;
    constexpr int STAT_CARD_HEIGHT = 80;
    constexpr int STAT_CARD_SPACING = 15;
    constexpr int FEATURE_CARD_SPACING = 20;
    
    // 上传页面相关常量
    constexpr int UPLOAD_CARD_MAX_WIDTH = 800;
    constexpr int UPLOAD_CARD_MIN_WIDTH = 500;
    constexpr int UPLOAD_INPUT_HEIGHT = 50;
    constexpr int UPLOAD_TEXT_EDIT_HEIGHT = 220;
    constexpr int UPLOAD_TEXT_LIMIT = 6000;
    constexpr int UPLOAD_CHAPTER_MIN = 1;
    constexpr int UPLOAD_CHAPTER_MAX = 9999;
    
    // Dashboard页面相关常量
    constexpr int DASHBOARD_STAT_CARD_WIDTH = 100;
    constexpr int DASHBOARD_STAT_CARD_HEIGHT = 80;
    constexpr int DASHBOARD_JOB_ITEM_HEIGHT = 60;
    constexpr int DASHBOARD_JOB_ITEM_SPACING = 16;
    constexpr int DASHBOARD_JOB_LIST_MIN_HEIGHT = 200;
    constexpr int DASHBOARD_TIMELINE_ITEM_HEIGHT = 50;
    constexpr int DASHBOARD_STEP_NUMBER_SIZE = 32;
    constexpr int DASHBOARD_TIMELINE_DOT_SIZE = 18;
    constexpr int DASHBOARD_ICON_SIZE = 48;
    constexpr int DASHBOARD_MAX_OVERVIEW_JOBS = 5;
    constexpr int DASHBOARD_MAX_ACTIVE_JOBS = 3;
    
    // Dashboard字体大小
    constexpr int DASHBOARD_SIZE_TITLE = 28;
    constexpr int DASHBOARD_SIZE_CARD_TITLE = 18;
    constexpr int DASHBOARD_SIZE_NORMAL = 14;
    constexpr int DASHBOARD_SIZE_SMALL = 13;
    constexpr int DASHBOARD_SIZE_TINY = 12;
    constexpr int DASHBOARD_SIZE_STAT_VALUE = 24;
    constexpr int DASHBOARD_SIZE_STEP_NUMBER = 17;
}

namespace Colors {
    const QString PRIMARY = "#84cc16";
    const QString PRIMARY_HOVER = "#65a30d";
    const QString PRIMARY_DARK = "#4d7c0f";
    const QString SECONDARY = "#facc15";
    const QString SECONDARY_HOVER = "#eab308";
    const QString SECONDARY_DARK = "#ca8a04";
    const QString SUCCESS = "#22C55E";
    const QString SUCCESS_HOVER = "#16A34A";
    const QString ERROR = "#EF4444";
    const QString ERROR_HOVER = "#DC2626";
    const QString TEXT_PRIMARY = "#365314";
    const QString TEXT_SECONDARY = "#57534e";
    const QString TEXT_MUTED = "#a3a3a3";
    const QString BORDER = "#d4d4d4";
    const QString BORDER_LIGHT = "#e5e5e5";
    const QString BACKGROUND = "#f7fee7";
    const QString BACKGROUND_LIGHT = "#ecfccb";
    const QString WHITE = "#FFFFFF";
    const QString TRANSPARENT = "transparent";
    
    const QString COLOR_SUCCESS = "#22C55E";
    const QString COLOR_SUCCESS_HOVER = "#16A34A";
    const QString COLOR_ERROR = "#EF4444";
    const QString COLOR_ERROR_HOVER = "#DC2626";
    const QString COLOR_CANCEL_BG = "#f7fee7";
    const QString COLOR_CANCEL_HOVER = "#ecfccb";
    
    const QString DASHBOARD_TEXT_PRIMARY = "#1e293b";
    const QString DASHBOARD_TEXT_SECONDARY = "#64748b";
    const QString DASHBOARD_TEXT_MUTED = "#94a3b8";
    const QString DASHBOARD_JOB_TYPE_BG = "#2684cc16";
    const QString DASHBOARD_JOB_TYPE_TEXT = "#4d7c0f";
    const QString DASHBOARD_FOOTER_BG = "#146366f1";
    const QString DASHBOARD_STAT_TOTAL_BG = "#FEE5EC";
    const QString DASHBOARD_STAT_RUNNING_BG = "#F3E8FF";
    const QString DASHBOARD_STAT_COMPLETED_BG = "#E6F7EF";
    const QString DASHBOARD_STAT_FAILED_BG = "#FEE5EC";
}

namespace UI {
    QWidget* createTransparentWidget();
    QLabel* createLabel(const QString &text, const QString &style, int fontSize = -1, bool bold = false);
    QPushButton* createButton(const QString &text, const QString &style, int width = -1, int height = Constants::BTN_HEIGHT);
    QLineEdit* createLineEdit(const QString &placeholder, int height = Constants::INPUT_HEIGHT);
    void setupLayout(QLayout *layout, int left, int top, int right, int bottom, int spacing = 0);
}

constexpr const char* TRANSPARENT_BG = "background: transparent;";

}

// 导出图标轮转：每次调用返回下一个表情图标，9 种轮流出现
QString getRotatingExportIcon();

// 将 SVG 资源渲染为指定尺寸的 QPixmap
QPixmap renderSvg(const QString &resourcePath, int size);

#endif // EDITOR_STYLES_H

#include "components/EditorStyles.h"

namespace EditorStyles {

QString scrollBarStyle()
{
    return R"(
        QScrollBar:vertical {
            background: #F3F4F6;
            width: 8px;
            margin: 4px 2px 4px 0;
            border-radius: 4px;
        }
        QScrollBar::handle:vertical {
            background: #D1D5DB;
            border-radius: 4px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover {
            background: #9CA3AF;
        }
        QScrollBar::handle:vertical:pressed {
            background: #84cc16;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
            background: transparent;
        }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: transparent;
        }
        QScrollBar:horizontal {
            background: #F3F4F6;
            height: 8px;
            margin: 0 2px 4px 4px;
            border-radius: 4px;
        }
        QScrollBar::handle:horizontal {
            background: #D1D5DB;
            border-radius: 4px;
            min-width: 30px;
        }
        QScrollBar::handle:horizontal:hover {
            background: #9CA3AF;
        }
        QScrollBar::handle:horizontal:pressed {
            background: #84cc16;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
            background: transparent;
        }
        QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
            background: transparent;
        }
    )";
}

QString scrollAreaStyle()
{
    return R"(
        QScrollArea {
            background: transparent;
            border: none;
        }
    )" + scrollBarStyle();
}

QString closeButtonStyle()
{
    return R"(
        QPushButton {
            border: none;
            background: transparent;
            color: #9CA3AF;
            font-size: 16px;
            border-radius: 14px;
        }
        QPushButton:hover {
            background: #F3F4F6;
            color: #6B7280;
        }
        QPushButton:pressed {
            background: #E5E7EB;
        }
    )";
}

QString saveButtonStyle()
{
    return QString(R"(
        QPushButton {
            padding: 0 28px;
            font-size: 14px;
            font-weight: 600;
            border: none;
            border-radius: 10px;
            background: %1;
            color: white;
            min-height: 40px;
        }
        QPushButton:hover { background: %2; }
        QPushButton:pressed { background: %3; }
    )").arg(Colors::PRIMARY, Colors::PRIMARY_HOVER, Colors::PRIMARY_DARK);
}

QString cancelButtonStyle()
{
    return R"(
        QPushButton {
            padding: 0 28px;
            font-size: 14px;
            font-weight: 500;
            border: 1px solid #E5E7EB;
            border-radius: 10px;
            background: white;
            color: #6B7280;
            min-height: 40px;
        }
        QPushButton:hover {
            background: #F9FAFB;
            border-color: #D1D5DB;
            color: #374151;
        }
    )";
}

QString editButtonStyle()
{
    return R"(
        QPushButton {
            padding: 6px 16px;
            border-radius: 6px;
            border: 1px solid #99a3e635;
            background: #ebf7fee7;
            color: #4d7c0f;
            font-size: 13px;
        }
        QPushButton:hover {
            background: #66a3e635;
            border-color: #b384cc16;
        }
    )";
}

QString deleteButtonStyle()
{
    return R"(
        QPushButton {
            padding: 6px 16px;
            border-radius: 6px;
            border: 1px solid #4def4444;
            background: #14ef4444;
            color: #dc2626;
            font-size: 13px;
        }
        QPushButton:hover {
            background: #26ef4444;
            border-color: #80ef4444;
        }
    )";
}

QString novelCardEditButtonStyle()
{
    return R"(
        QPushButton {
            padding: 6px 16px;
            border-radius: 6px;
            border: 1px solid #99a3e635;
            background: #ebf7fee7;
            color: #4d7c0f;
            font-size: 13px;
        }
        QPushButton:hover {
            background: #66a3e635;
            border-color: #b384cc16;
        }
    )";
}

QString novelCardDeleteButtonStyle()
{
    return R"(
        QPushButton {
            padding: 6px 16px;
            border-radius: 6px;
            border: 1px solid #4def4444;
            background: #14ef4444;
            color: #dc2626;
            font-size: 13px;
        }
        QPushButton:hover {
            background: #26ef4444;
            border-color: #80ef4444;
        }
    )";
}

QString textEditStyle()
{
    return R"(
        QTextEdit {
            border-radius: 10px;
            border: 1px solid #d4d4d4;
            background: white;
            padding: 10px 14px;
            font-size: 13px;
            color: #374151;
        }
        QTextEdit:focus {
            border-color: #84cc16;
        }
    )" + scrollBarStyle();
}

QString lineEditStyle()
{
    return R"(
        QLineEdit {
            border-radius: 10px;
            border: 1px solid #d4d4d4;
            background: white;
            padding: 0 14px;
            font-size: 13px;
            color: #374151;
        }
        QLineEdit:focus {
            border-color: #84cc16;
        }
        QLineEdit::placeholder {
            color: #a3a3a3;
        }
    )";
}

QString comboBoxStyle()
{
    return R"(
        QComboBox {
            padding: 0 12px;
            font-size: 13px;
            border: 1px solid #d4d4d4;
            border-radius: 10px;
            background: white;
            height: 40px;
            color: #374151;
        }
        QComboBox:hover {
            border-color: #84cc16;
        }
        QComboBox::drop-down {
            border: none;
            width: 32px;
            subcontrol-position: center right;
        }
        QComboBox::down-arrow {
            image: none;
        }
        QComboBox QAbstractItemView {
            background: white;
            border: 1px solid #d4d4d4;
            border-radius: 10px;
            selection-background-color: #2684cc16;
            selection-color: #4d7c0f;
            padding: 6px;
        }
        QComboBox QAbstractItemView::item {
            height: 36px;
            padding: 0 14px;
            border-radius: 8px;
            color: #374151;
        }
        QComboBox QAbstractItemView::item:hover { 
            background: #2684cc16; 
        }
    )";
}

QString modeButtonStyle()
{
    return R"(
        QPushButton {
            padding: 0 12px;
            font-size: 12px;
            border: 1px solid #E5E7EB;
            border-radius: 6px;
            background: white;
            color: #333333;
            height: 32px;
        }
        QPushButton:hover {
            background: #F9FAFB;
        }
    )";
}

QString modeButtonActiveStyle()
{
    return R"(
        QPushButton {
            padding: 0 12px;
            font-size: 12px;
            border: 2px solid #84cc16;
            border-radius: 6px;
            background: #f7fee7;
            color: #4d7c0f;
            height: 32px;
        }
    )";
}

QString primaryButtonStyle()
{
    return QString(R"(
        QPushButton {
            border-radius: 10px;
            border: none;
            background: %1;
            color: white;
            font-size: 14px;
            font-weight: 600;
        }
        QPushButton:hover { background: %2; }
        QPushButton:pressed { background: %3; }
    )").arg(Colors::PRIMARY, Colors::PRIMARY_HOVER, Colors::PRIMARY_DARK);
}

QString secondaryButtonStyle()
{
    return R"(
        QPushButton {
            border-radius: 10px;
            border: 1px solid #d4d4d4;
            background: white;
            color: #374151;
            font-size: 14px;
            font-weight: 500;
        }
        QPushButton:hover { 
            background: #f7fee7;
            border-color: #84cc16;
            color: #4d7c0f;
        }
    )";
}

QString featureButtonStyle()
{
    return QString(R"(
        QPushButton {
            border-radius: 8px;
            border: none;
            background: %1;
            color: white;
            font-size: 14px;
            padding: 8px 20px;
            min-height: 24px;
        }
        QPushButton:hover { background: %2; }
    )").arg(Colors::SECONDARY, Colors::SECONDARY_HOVER);
}

QString refreshButtonStyle()
{
    return R"(
        QPushButton {
            border-radius: 8px;
            border: 1px solid #d4d4d4;
            background: #f7fee7;
            color: #57534e;
            font-size: 14px;
            padding: 8px 16px;
        }
        QPushButton:hover { background: #ecfccb; }
    )";
}

QString cardStyle()
{
    return R"(
        #featureCard, #novelCard, #chapterCard, #editorCard {
            background: #faffffff;
            border-radius: 18px;
            border: 1px solid #14000000;
        }
        #featureCard:hover, #novelCard:hover, #chapterCard:hover {
            border: 1px solid #8084cc16;
        }
    )";
}

QString inputStyle()
{
    return R"(
        QLineEdit {
            border-radius: 8px;
            border: 1px solid #d4d4d4;
            padding: 0 8px;
            font-size: 14px;
            background: white;
        }
        QLineEdit:focus {
            border-color: #84cc16;
        }
        QTextEdit {
            border-radius: 8px;
            border: 1px solid #d4d4d4;
            padding: 8px;
            font-size: 14px;
            background: white;
        }
        QTextEdit:focus {
            border-color: #84cc16;
        }
    )";
}

QString spinBoxStyle()
{
    return R"(
        QSpinBox {
            border-radius: 10px 0 0 10px;
            border: 1px solid #d4d4d4;
            border-right: none;
            background: white;
            padding: 0 14px;
            font-size: 13px;
            color: #374151;
        }
        QSpinBox:focus {
            border-color: #84cc16;
        }
        QSpinBox::up-button, QSpinBox::down-button {
            width: 24px;
            border: none;
            background: transparent;
        }
        QSpinBox::up-arrow {
            image: none;
        }
        QSpinBox::down-arrow {
            image: none;
        }
    )";
}

QString chapterCardActiveStyle()
{
    return "#chapterCard { background-color: #f7fee7; border: 2px solid #84cc16; border-radius: 12px; }";
}

QString chapterCardInactiveStyle()
{
    return "#chapterCard { background-color: #FFFFFF; border: 1px solid #d4d4d4; border-radius: 12px; }"
           "#chapterCard:hover { border-color: #84cc16; }";
}

// ========== 上传页面样式 ==========

QString uploadInputStyle()
{
    return R"(
        QLineEdit {
            padding: 14px 16px;
            font-size: 15px;
            border: 2px solid #d4d4d4;
            border-radius: 12px;
            background: #ffffff;
            color: #1e293b;
        }
        QLineEdit:focus {
            border-color: #84cc16;
            background: #f7fee7;
        }
        QLineEdit::placeholder {
            color: #94a3b8;
        }
    )";
}

QString uploadTextEditStyle()
{
    return R"(
        QTextEdit {
            padding: 14px 16px;
            font-size: 14px;
            font-family: 'Microsoft YaHei', sans-serif;
            border: 2px solid #d4d4d4;
            border-radius: 12px;
            background: #ffffff;
            color: #1e293b;
            line-height: 1.6;
        }
        QTextEdit:focus {
            border-color: #84cc16;
            background: #f7fee7;
        }
    )";
}

QString uploadPrimaryButtonStyle()
{
    return QString(R"(
        QPushButton {
            padding: 14px 32px;
            font-size: 15px;
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 %1, stop:1 %2);
            color: white;
            border: none;
            border-radius: 12px;
            font-weight: bold;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 %2, stop:1 %3);
        }
        QPushButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 %3, stop:1 #3f6212);
        }
        QPushButton:disabled {
            background: #cbd5e1;
            color: #94a3b8;
        }
    )").arg(Colors::PRIMARY, Colors::PRIMARY_HOVER, Colors::PRIMARY_DARK);
}

QString uploadSecondaryButtonStyle()
{
    return R"(
        QPushButton {
            padding: 14px 24px;
            border-radius: 12px;
            border: 2px solid #d4d4d4;
            background: #ffffff;
            color: #4d7c0f;
            font-weight: 600;
            font-size: 14px;
        }
        QPushButton:hover {
            background: #f7fee7;
            border-color: #84cc16;
        }
        QPushButton:pressed {
            background: #ecfccb;
        }
    )";
}

QString uploadErrorLabelStyle()
{
    return R"(
        QLabel {
            padding: 14px 18px;
            background: #14ef4444;
            border: 1px solid #33ef4444;
            border-radius: 12px;
            color: #dc2626;
            font-size: 14px;
        }
    )";
}

QString uploadSuccessLabelStyle()
{
    return R"(
        QLabel {
            padding: 14px 18px;
            background: #1410b981;
            border: 1px solid #3310b981;
            border-radius: 12px;
            color: #059669;
            font-size: 14px;
        }
    )";
}

QString uploadProgressBarStyle()
{
    return R"(
        QProgressBar {
            background: #e2e8f0;
            border-radius: 3px;
            border: none;
        }
        QProgressBar::chunk {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #84cc16, stop:1 #65a30d);
            border-radius: 3px;
        }
    )";
}

QString uploadCardStyle()
{
    return R"(
        #uploadContainer {
            background: #f2ffffff;
            border-radius: 24px;
            border: 1px solid #1a6366f1;
        }
    )";
}

QString uploadPageBackgroundStyle()
{
    return R"(
        #novelUploadPage {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #fafbff,
                stop:0.5 #f5f3ff,
                stop:1 #ede9fe);
        }
    )";
}

QString uploadScrollAreaStyle()
{
    return R"(
        QScrollArea {
            border: none;
            background: transparent;
        }
    )" + scrollBarStyle();
}

QString uploadTipsSectionStyle()
{
    return R"(
        #tipsSection {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #0a6366f1,
                stop:1 #0a8b5cf6);
            border-radius: 16px;
            border: 1px solid #1a6366f1;
        }
    )";
}

QString uploadHeaderIconStyle()
{
    return R"(
        background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
            stop:0 #84cc16, stop:1 #65a30d);
        border-radius: 14px;
        color: white;
        font-size: 24px;
    )";
}

QString uploadDialogStyle()
{
    return R"(
        QDialog {
            background: qlineargradient(x1:0, y1:0, x2:1,
                stop:0 #fafbff, stop:1 #f5f3ff);
            border-radius: 16px;
            border: 1px solid #3d6366f1;
        }
    )";
}

QString uploadDialogCancelButtonStyle()
{
    return R"(
        QPushButton {
            background: #f1f5f9;
            color: #64748b;
            border: none;
            border-radius: 10px;
            font-size: 14px;
            font-weight: 500;
        }
        QPushButton:hover {
            background: #e2e8f0;
        }
    )";
}

QString uploadDialogOkButtonStyle()
{
    return QString(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, stop:0 %1, stop:1 %2);
            color: white;
            border: none;
            border-radius: 10px;
            font-size: 14px;
            font-weight: 600;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, stop:0 %2, stop:1 %3);
        }
    )").arg(Colors::PRIMARY, Colors::PRIMARY_HOVER, Colors::PRIMARY_DARK);
}

QString uploadSpinButtonStyle()
{
    return R"(
        QPushButton {
            background: transparent;
            border: none;
            color: #4d7c0f;
            font-size: 10px;
        }
        QPushButton:hover {
            background: #2684cc16;
            border-radius: 4px;
        }
    )";
}

QString uploadSpinContainerStyle()
{
    return R"(
        QWidget {
            background: white;
            border: 2px solid #e2e8f0;
            border-radius: 12px;
        }
    )";
}

QString uploadChapterValueEditStyle()
{
    return R"(
        QLineEdit {
            border: none;
            background: transparent;
            font-size: 18px;
            font-weight: bold;
            color: #1e293b;
        }
    )";
}

QString uploadFormLabelStyle()
{
    return "font-size: 14px; font-weight: 600; color: #374151; background: transparent;";
}

QString uploadFormHintStyle()
{
    return "font-size: 12px; color: #94a3b8; background: transparent;";
}

QString uploadTipsTitleStyle()
{
    return "font-size: 15px; font-weight: 600; color: #4d7c0f; background: transparent;";
}

QString uploadTipsItemStyle()
{
    return "font-size: 13px; color: #64748b; background: transparent;";
}

QString uploadDialogTitleStyle()
{
    return "font-size: 18px; font-weight: bold; color: #1e293b; background: transparent;";
}

QString uploadWaitingLabelStyle()
{
    return "color: #f59e0b; font-size: 14px; background: transparent;";
}

QString uploadPageTitleStyle()
{
    return "font-size: 26px; font-weight: bold; color: #1e293b; background: transparent;";
}

// ========== 小说详情页样式 ==========

QString horizontalScrollAreaStyle()
{
    return R"(
        QScrollArea {
            border: none;
            background: transparent;
        }
    )" + scrollBarStyle();
}

QString storyboardCardStyle()
{
    return R"(
        #storyboardCard {
            background-color: #fafbff;
            border-radius: 16px;
            border: 1px solid #E8E8F0;
        }
    )";
}

QString panelPreviewCardStyle()
{
    return R"(
        #panelPreviewCard {
            background-color: #fafbff;
            border-radius: 16px;
            border: 1px solid #E8E8F0;
        }
    )";
}

// ========== 圣经卡片样式 ==========

QString bibleCardStyle()
{
    return "#bibleCard { background-color: #fafbff; border-radius: 12px; border: 1px solid #E8E8F0; }";
}

QString bibleItemStyle()
{
    return R"(
        #bibleItem {
            background-color: #fdfdff;
            border-radius: 16px;
            border: 1px solid #E8E8F0;
        }
        #bibleItem:hover {
            border-color: #84cc16;
        }
    )";
}

QString bibleImagePlaceholderStyle()
{
    return "background: #F5F5F5; border-radius: 12px; border: 1px dashed #D1D5DB;";
}

QString bibleUploadButtonStyle()
{
    return R"(
        QPushButton {
            border-radius: 12px;
            border: 1px dashed #D1D5DB;
            background: transparent;
            color: #64748b;
            font-size: 12px;
        }
        QPushButton:hover {
            border-color: #84cc16;
            color: #4d7c0f;
        }
    )";
}

QString bibleDeleteButtonStyle()
{
    return R"(
        QPushButton {
            border: none;
            background: transparent;
            color: #EF4444;
            font-size: 11px;
        }
        QPushButton:hover {
            color: #DC2626;
        }
    )";
}

QString bibleDetailLabelStyle()
{
    return "font-size: 13px; color: #64748b; background: transparent;";
}

QString bibleNameLabelStyle()
{
    return "font-size: 15px; color: #333333; background: transparent;";
}

QString bibleInputFieldLabelStyle()
{
    return "font-size: 12px; color: #6B7280; background: transparent;";
}

QString bibleEditorOverlayStyle()
{
    return "background: #66000000;";
}

QString bibleEditorTitleStyle()
{
    return "font-size: 16px; font-weight: bold; color: #333333; background: transparent;";
}

QString bibleImageHasImageStyle()
{
    return "background: #E8F5E9; border-radius: 12px; border: 1px solid #C8E6C9;";
}

QString headerSectionStyle()
{
    return "#headerSection { background-color: #EEF2FF; border-radius: 12px; border: none; }";
}

QString chapterSectionStyle()
{
    return "#chapterSection { background-color: #ffffff; border-radius: 12px; border: 1px solid #f0f0f0; }";
}

// ========== 分镜卡片样式 ==========

QString panelCardStyle()
{
    return R"(
        #panelCard {
            background: white;
            border-radius: 16px;
            border: 1px solid #E8E8F0;
        }
        #panelCard:hover {
            border-color: #84cc16;
        }
    )";
}

QString panelPreviewStyle()
{
    return R"(
        background: #F9FAFB;
        border: 1px solid #E5E7EB;
        border-bottom-left-radius: 8px;
        border-bottom-right-radius: 8px;
        border-top: none;
    )";
}

QString panelTitleLabelStyle()
{
    return R"(
        font-size: 11px;
        font-weight: bold;
        color: #6B7280;
        background: #0d000000;
        padding: 4px 8px;
        border-top-left-radius: 8px;
        border-top-right-radius: 8px;
        border: none;
    )";
}

QString panelNumberLabelStyle()
{
    return "font-size: 16px; font-weight: bold; color: #333333; background: transparent; border: none;";
}

QString panelDescLabelStyle()
{
    return "font-size: 14px; color: #333333; background: transparent; line-height: 1.4; border: none;";
}

QString panelEditorPreviewStyle()
{
    return "background: #F3F4F6; border-radius: 8px; border: 1px dashed #D1D5DB;";
}

QString panelEditorInfoLabelStyle()
{
    return "font-size: 12px; color: #6B7280; background: transparent;";
}

QString storyboardItemStyle()
{
    return R"(
        #storyboardItem {
            background: white;
            border-radius: 16px;
            border: 1px solid #E8E8F0;
        }
        #storyboardItem:hover {
            border-color: #84cc16;
        }
    )";
}

QString storyboardInfoTitleStyle()
{
    return "font-size: 13px; color: #6B7280; background: transparent;";
}

QString storyboardInfoContentStyle()
{
    return "font-size: 13px; color: #333333; background: transparent;";
}

QString storyboardComboBoxStyle()
{
    return R"(
        QComboBox {
            border-radius: 10px 0 0 10px;
            border: 1px solid #d4d4d4;
            border-right: none;
            background: white;
            padding: 0 14px;
            font-size: 13px;
            color: #374151;
        }
        QComboBox:hover {
            border-color: #84cc16;
        }
        QComboBox::drop-down { border: none; width: 0; }
        QComboBox::down-arrow { image: none; }
        QComboBox QAbstractItemView {
            background: white;
            border: 1px solid #d4d4d4;
            border-radius: 10px;
            selection-background-color: #2684cc16;
            selection-color: #4d7c0f;
            padding: 6px;
        }
        QComboBox QAbstractItemView::item {
            height: 36px;
            padding: 0 14px;
            border-radius: 8px;
            color: #374151;
        }
        QComboBox QAbstractItemView::item:hover { 
            background: #2684cc16; 
        }
    )";
}

QString storyboardComboBoxBtnStyle()
{
    return R"(
        QWidget {
            background: white;
            border: 1px solid #d4d4d4;
            border-left: none;
            border-radius: 0 10px 10px 0;
        }
    )";
}

QString storyboardArrowButtonStyle()
{
    return R"(
        QPushButton {
            border: none;
            background: transparent;
            color: #4d7c0f;
            font-size: 10px;
            border-radius: 4px;
        }
        QPushButton:hover { 
            background: #2684cc16; 
        }
        QPushButton:pressed { 
            background: #4084cc16; 
        }
    )";
}

// ========== Dashboard页面样式 ==========

QString dashboardPageBackgroundStyle()
{
    return R"(
        #dashboardPage {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #fafbff,
                stop:0.45 #f5f3ff,
                stop:1 #ede9fe);
        }
    )";
}

QString dashboardHeroSectionStyle()
{
    return R"(
        #heroSection {
            background: #faffffff;
            border-radius: 28px;
            border: 1px solid #cce2e8f0;
        }
    )";
}

QString dashboardCardStyle()
{
    return R"(
        #cardStandard, #cardGuide {
            background: #faffffff;
            border-radius: 24px;
            border: 1px solid #cce2e8f0;
        }
    )";
}

QString dashboardJobItemStyle()
{
    return R"(
        #jobItem {
            background: #f2f8fafc;
            border-radius: 16px;
            border: 1px solid #99e2e8f0;
        }
    )";
}

QString dashboardStepNumberStyle()
{
    return R"(
        QLabel {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #2684cc16,
                stop:1 #1aa3e635);
            border-radius: 12px;
            color: #4d7c0f;
            font-weight: 700;
            font-size: 17px;
        }
    )";
}

QString dashboardTimelineDotStyle()
{
    return QString(R"(
        QLabel {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 %1, stop:1 %2);
            border-radius: 9px;
            border: 2px solid white;
        }
    )").arg(Colors::PRIMARY, Colors::PRIMARY_HOVER);
}

QString dashboardStatCardStyle(const QString &bgColor)
{
    return QString("#statCard { background-color: %1; border-radius: 12px; }").arg(bgColor);
}

QString dashboardTitleStyle()
{
    return QString("font-size: %1px; font-weight: bold; color: %2; background: transparent;")
        .arg(Constants::DASHBOARD_SIZE_TITLE).arg(Colors::DASHBOARD_TEXT_PRIMARY);
}

QString dashboardDescStyle()
{
    return QString("font-size: %1px; color: %2; background: transparent;")
        .arg(Constants::DASHBOARD_SIZE_NORMAL).arg(Colors::DASHBOARD_TEXT_SECONDARY);
}

QString dashboardCardTitleStyle()
{
    return QString("font-size: %1px; font-weight: bold; color: %2; background: transparent;")
        .arg(Constants::DASHBOARD_SIZE_CARD_TITLE).arg(Colors::DASHBOARD_TEXT_PRIMARY);
}

QString dashboardCardSubtitleStyle()
{
    return QString("font-size: %1px; color: %2; background: transparent;")
        .arg(Constants::DASHBOARD_SIZE_TINY).arg(Colors::DASHBOARD_TEXT_MUTED);
}

QString dashboardJobTypeStyle()
{
    return QString("padding: 4px 8px; border-radius: 10px; font-size: %1px; font-weight: 600; background: %2; color: %3;")
        .arg(Constants::DASHBOARD_SIZE_TINY).arg(Colors::DASHBOARD_JOB_TYPE_BG).arg(Colors::DASHBOARD_JOB_TYPE_TEXT);
}

QString dashboardJobMetaStyle()
{
    return QString("font-size: %1px; color: %2; background: transparent;")
        .arg(Constants::DASHBOARD_SIZE_SMALL).arg(Colors::DASHBOARD_TEXT_SECONDARY);
}

QString dashboardTimelineTitleStyle()
{
    return QString("font-weight: bold; color: %1; font-size: %2px; background: transparent;")
        .arg(Colors::DASHBOARD_TEXT_PRIMARY).arg(Constants::DASHBOARD_SIZE_NORMAL + 1);
}

QString dashboardTimelineMetaStyle()
{
    return QString("font-size: %1px; color: %2; background: transparent;")
        .arg(Constants::DASHBOARD_SIZE_TINY).arg(Colors::DASHBOARD_TEXT_MUTED);
}

QString dashboardStepTitleStyle()
{
    return QString("font-weight: bold; color: %1; font-size: %2px; background: transparent;")
        .arg(Colors::DASHBOARD_TEXT_PRIMARY).arg(Constants::DASHBOARD_SIZE_NORMAL + 1);
}

QString dashboardStepDescStyle()
{
    return QString("font-size: %1px; color: %2; background: transparent;")
        .arg(Constants::DASHBOARD_SIZE_SMALL).arg(Colors::DASHBOARD_TEXT_SECONDARY);
}

QString dashboardFooterLabelStyle()
{
    return QString("font-size: %1px; color: %2;")
        .arg(Constants::DASHBOARD_SIZE_SMALL).arg(Colors::DASHBOARD_TEXT_SECONDARY);
}

QString dashboardFilterLabelStyle()
{
    return QString("font-size: %1px; color: #333333; background: transparent;")
        .arg(Constants::DASHBOARD_SIZE_NORMAL);
}

QString dashboardIconStyle()
{
    return QString(R"(
        background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
            stop:0 %1, stop:1 %2);
        border-radius: 14px;
        color: white;
        font-size: 24px;
    )").arg(Colors::PRIMARY, Colors::PRIMARY_HOVER);
}

QString dashboardActiveJobsFooterStyle()
{
    return QString("#activeJobsFooter { background: %1; border-radius: 12px; }")
        .arg(Colors::DASHBOARD_FOOTER_BG);
}

QString dashboardStatCardLabelStyle()
{
    return "font-size: 12px; color: #666666;";
}

QString dashboardStatCardValueStyle()
{
    return QString("font-size: %1px; color: #333333;")
        .arg(Constants::DASHBOARD_SIZE_STAT_VALUE);
}

QString dashboardEmptyStateStyle()
{
    return "color: #4d1e293b; font-size: 14px;";
}

namespace UI {

QWidget* createTransparentWidget()
{
    QWidget *widget = new QWidget();
    widget->setStyleSheet(TRANSPARENT_BG);
    return widget;
}

QLabel* createLabel(const QString &text, const QString &style, int fontSize, bool bold)
{
    QLabel *label = new QLabel(text);
    label->setStyleSheet(style);
    if (fontSize > 0) {
        QFont font("Microsoft YaHei", fontSize, bold ? QFont::Bold : QFont::Normal);
        label->setFont(font);
    }
    return label;
}

QPushButton* createButton(const QString &text, const QString &style, int width, int height)
{
    QPushButton *btn = new QPushButton(text);
    btn->setStyleSheet(style);
    btn->setCursor(Qt::PointingHandCursor);
    if (width > 0 && height > 0) {
        btn->setFixedSize(width, height);
    } else if (height > 0) {
        btn->setFixedHeight(height);
    }
    return btn;
}

QLineEdit* createLineEdit(const QString &placeholder, int height)
{
    QLineEdit *edit = new QLineEdit();
    edit->setPlaceholderText(placeholder);
    edit->setStyleSheet(inputStyle());
    edit->setFixedHeight(height);
    return edit;
}

void setupLayout(QLayout *layout, int left, int top, int right, int bottom, int spacing)
{
    layout->setContentsMargins(left, top, right, bottom);
    layout->setSpacing(spacing);
}

}

}

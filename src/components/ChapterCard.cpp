#include "components/ChapterCard.h"
#include "components/EditorStyles.h"
#include "utils/StatusHelper.h"
#include <QApplication>
#include <QMouseEvent>

ChapterCard::ChapterCard(int chapterNumber, int panelCount, 
                         const QString &status, bool isActive, QWidget *parent)
    : QFrame(parent)
    , m_chapterNumber(chapterNumber)
    , m_panelCount(panelCount)
    , m_status(status)
    , m_isActive(isActive)
{
    setObjectName("chapterCard");
    setCursor(Qt::PointingHandCursor);
    setMinimumHeight(80);
    setMinimumWidth(160);
    setupUI();
}

void ChapterCard::setupUI()
{
    setStyleSheet(m_isActive ? EditorStyles::chapterCardActiveStyle() 
                             : EditorStyles::chapterCardInactiveStyle());
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(16, 12, 16, 12);
    
    mainLayout->addLayout(createHeaderRow());
    mainLayout->addWidget(createInfoRow());
}

QHBoxLayout* ChapterCard::createHeaderRow()
{
    QHBoxLayout *headerRow = new QHBoxLayout();
    headerRow->setSpacing(8);
    headerRow->setContentsMargins(0, 0, 0, 0);
    
    QString chapterText = QString::fromUtf8("\u7b2c%1\u7ae0").arg(m_chapterNumber);
    QLabel *titleLabel = new QLabel(chapterText);
    titleLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #1F2937; background: transparent;");
    headerRow->addWidget(titleLabel);
    
    headerRow->addStretch();
    
    QPushButton *deleteBtn = new QPushButton(QString::fromUtf8("\u00d7"));
    deleteBtn->setFixedSize(24, 24);
    deleteBtn->setCursor(Qt::PointingHandCursor);
    deleteBtn->setStyleSheet(R"(
        QPushButton {
            background: transparent;
            border: none;
            color: #9CA3AF;
            font-size: 18px;
        }
        QPushButton:hover {
            color: #EF4444;
        }
    )");
    connect(deleteBtn, &QPushButton::clicked, this, [this]() {
        emit deleteRequested(m_chapterNumber);
    });
    headerRow->addWidget(deleteBtn);
    
    return headerRow;
}

QWidget* ChapterCard::createInfoRow()
{
    QWidget *row = new QWidget();
    row->setStyleSheet("background: transparent;");
    QHBoxLayout *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(16);
    
    QString panelText = QString::fromUtf8("%1 \u4e2a\u9762\u677f").arg(m_panelCount);
    QLabel *panelLabel = new QLabel(panelText);
    panelLabel->setStyleSheet("font-size: 13px; color: #6B7280; background: transparent;");
    layout->addWidget(panelLabel);
    
    StatusHelper::StatusStyle style = StatusHelper::Chapter::style(m_status);
    QLabel *statusLabel = new QLabel(style.text);
    statusLabel->setStyleSheet(QString("font-size: 13px; color: %1; background: transparent;").arg(style.textColor));
    layout->addWidget(statusLabel);
    
    layout->addStretch();
    
    return row;
}

void ChapterCard::setActive(bool active)
{
    if (m_isActive != active) {
        m_isActive = active;
        setStyleSheet(active ? EditorStyles::chapterCardActiveStyle() 
                             : EditorStyles::chapterCardInactiveStyle());
    }
}

void ChapterCard::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    emit clicked(m_chapterNumber);
}

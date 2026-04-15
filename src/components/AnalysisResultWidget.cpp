#include "components/AnalysisResultWidget.h"
#include "EncodingUtils.h"
#include <QGraphicsOpacityEffect>
#include <QPushButton>

namespace {
    const QString CONTAINER_STYLE = R"(
        QWidget#resultContainer {
            background: #F0FDF4;
            border: 1px solid #86EFAC;
            border-radius: 12px;
            padding: 16px;
        }
    )";
    
    const QString TITLE_STYLE = "font-size: 16px; font-weight: bold; color: #166534; background: transparent;";
    const QString CHAPTER_STYLE = "font-size: 14px; color: #15803D; background: transparent;";
    const QString STAT_LABEL_STYLE = "font-size: 13px; color: #166534; background: transparent;";
    const QString STAT_VALUE_STYLE = "font-size: 18px; font-weight: bold; color: #15803D; background: transparent;";
}

AnalysisResultWidget::AnalysisResultWidget(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_contentWidget(nullptr)
    , m_titleLabel(nullptr)
    , m_chapterLabel(nullptr)
    , m_statsContainer(nullptr)
    , m_panelCountLabel(nullptr)
    , m_characterCountLabel(nullptr)
    , m_sceneCountLabel(nullptr)
    , m_showAnimation(nullptr)
    , m_hideAnimation(nullptr)
{
    setupUI();
}

void AnalysisResultWidget::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    
    m_contentWidget = new QWidget();
    m_contentWidget->setObjectName("resultContainer");
    m_contentWidget->setStyleSheet(CONTAINER_STYLE);
    
    QVBoxLayout *contentLayout = new QVBoxLayout(m_contentWidget);
    contentLayout->setContentsMargins(16, 16, 16, 16);
    contentLayout->setSpacing(12);
    
    QWidget *headerRow = new QWidget();
    headerRow->setStyleSheet("background: transparent;");
    QHBoxLayout *headerLayout = new QHBoxLayout(headerRow);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(8);
    
    m_titleLabel = new QLabel(tr("分析结果"));
    m_titleLabel->setStyleSheet(TITLE_STYLE);
    headerLayout->addWidget(m_titleLabel);
    
    m_chapterLabel = new QLabel();
    m_chapterLabel->setStyleSheet(CHAPTER_STYLE);
    headerLayout->addWidget(m_chapterLabel);
    
    headerLayout->addStretch();
    contentLayout->addWidget(headerRow);
    
    m_statsContainer = new QWidget();
    m_statsContainer->setStyleSheet("background: transparent;");
    QHBoxLayout *statsLayout = new QHBoxLayout(m_statsContainer);
    statsLayout->setContentsMargins(0, 0, 0, 0);
    statsLayout->setSpacing(24);
    
    auto createStatItem = [this, statsLayout](const QString &label, QLabel *&valueLabel) {
        QWidget *item = new QWidget();
        item->setStyleSheet("background: transparent;");
        QVBoxLayout *itemLayout = new QVBoxLayout(item);
        itemLayout->setContentsMargins(0, 0, 0, 0);
        itemLayout->setSpacing(4);
        
        QLabel *labelWidget = new QLabel(label);
        labelWidget->setStyleSheet(STAT_LABEL_STYLE);
        itemLayout->addWidget(labelWidget, 0, Qt::AlignCenter);
        
        valueLabel = new QLabel("0");
        valueLabel->setStyleSheet(STAT_VALUE_STYLE);
        itemLayout->addWidget(valueLabel, 0, Qt::AlignCenter);
        
        statsLayout->addWidget(item);
        return item;
    };
    
    createStatItem(tr("分镜数"), m_panelCountLabel);
    createStatItem(tr("角色数"), m_characterCountLabel);
    createStatItem(tr("场景数"), m_sceneCountLabel);
    
    statsLayout->addStretch();
    contentLayout->addWidget(m_statsContainer);
    
    m_mainLayout->addWidget(m_contentWidget);
    
    QGraphicsOpacityEffect *opacityEffect = new QGraphicsOpacityEffect(this);
    m_contentWidget->setGraphicsEffect(opacityEffect);
    
    m_showAnimation = new QPropertyAnimation(opacityEffect, "opacity", this);
    m_showAnimation->setDuration(300);
    m_showAnimation->setStartValue(0.0);
    m_showAnimation->setEndValue(1.0);
    
    m_hideAnimation = new QPropertyAnimation(opacityEffect, "opacity", this);
    m_hideAnimation->setDuration(200);
    m_hideAnimation->setStartValue(1.0);
    m_hideAnimation->setEndValue(0.0);
    
    connect(m_hideAnimation, &QPropertyAnimation::finished, this, [this]() {
        hide();
    });
    
    setVisible(false);
}

void AnalysisResultWidget::setResult(int chapterNumber, int panelCount, int characterCount, int sceneCount)
{
    m_chapterNumber = chapterNumber;
    m_panelCount = panelCount;
    m_characterCount = characterCount;
    m_sceneCount = sceneCount;
    updateDisplay();
}

void AnalysisResultWidget::setResult(const QJsonObject &result)
{
    m_chapterNumber = result["chapterNumber"].toInt();
    m_panelCount = result["panelCount"].toInt();
    m_characterCount = result["characterCount"].toInt();
    m_sceneCount = result["sceneCount"].toInt();
    updateDisplay();
}

void AnalysisResultWidget::clear()
{
    m_chapterNumber = 0;
    m_panelCount = 0;
    m_characterCount = 0;
    m_sceneCount = 0;
    updateDisplay();
    hide();
}

void AnalysisResultWidget::showAnimated()
{
    show();
    m_showAnimation->start();
}

void AnalysisResultWidget::hideAnimated()
{
    m_hideAnimation->start();
}

void AnalysisResultWidget::updateDisplay()
{
    if (m_chapterNumber > 0) {
        m_chapterLabel->setText(tr("第 %1 章").arg(m_chapterNumber));
    } else {
        m_chapterLabel->clear();
    }
    
    m_panelCountLabel->setText(QString::number(m_panelCount));
    m_characterCountLabel->setText(QString::number(m_characterCount));
    m_sceneCountLabel->setText(QString::number(m_sceneCount));
}

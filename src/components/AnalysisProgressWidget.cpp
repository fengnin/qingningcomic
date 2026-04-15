#include "components/AnalysisProgressWidget.h"
#include "EditorStyles.h"
#include "EncodingUtils.h"
#include "Logger.h"

namespace {
    const QString STATUS_LABEL_STYLE = "font-size: 13px; color: %1; background: transparent;";
    const QString RESULT_LABEL_STYLE = "font-size: 12px; color: #6B7280; background: transparent;";
}

AnalysisProgressWidget::AnalysisProgressWidget(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_progressBar(nullptr)
    , m_statusLabel(nullptr)
    , m_resultLabel(nullptr)
    , m_fadeOutTimer(nullptr)
{
    setupUI();
}

void AnalysisProgressWidget::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(6);
    
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(6);
    m_progressBar->setStyleSheet(EditorStyles::uploadProgressBarStyle());
    m_mainLayout->addWidget(m_progressBar);
    
    m_statusLabel = new QLabel();
    m_statusLabel->setStyleSheet(STATUS_LABEL_STYLE.arg("#3B82F6"));
    m_statusLabel->setWordWrap(true);
    m_mainLayout->addWidget(m_statusLabel);
    
    m_resultLabel = new QLabel();
    m_resultLabel->setStyleSheet(RESULT_LABEL_STYLE);
    m_resultLabel->setWordWrap(true);
    m_resultLabel->setVisible(false);
    m_mainLayout->addWidget(m_resultLabel);
    
    m_fadeOutTimer = new QTimer(this);
    m_fadeOutTimer->setSingleShot(true);
    m_fadeOutTimer->setInterval(1500);
    connect(m_fadeOutTimer, &QTimer::timeout, this, &AnalysisProgressWidget::onFadeOutTimer);
    
    setMinimumHeight(0);
    setMaximumHeight(0);
    hide();
    updateDisplay();
}

void AnalysisProgressWidget::setState(State state)
{
    m_state = state;
    
    if (state == State::Completed || state == State::Failed) {
        m_fadeOutTimer->start();
        m_fadeOutStarted = true;
    } else {
        m_fadeOutTimer->stop();
        m_fadeOutStarted = false;
        m_mainLayout->setContentsMargins(0, 8, 0, 0);
        setMinimumHeight(0);
        setMaximumHeight(QWIDGETSIZE_MAX);
        show();
        updateGeometry();
        if (parentWidget() && parentWidget()->layout()) {
            parentWidget()->layout()->activate();
        }
    }
    
    updateDisplay();
}

void AnalysisProgressWidget::setProgress(int value)
{
    m_progress = qBound(0, value, 100);
    m_progressBar->setValue(m_progress);
}

void AnalysisProgressWidget::setProgressText(const QString &text)
{
    m_progressText = text;
    updateDisplay();
}

void AnalysisProgressWidget::setResult(const QJsonObject &result)
{
    m_result = result;
    
    if (!result.isEmpty()) {
        QStringList resultParts;
        
        if (result.contains("panelCount")) {
            resultParts << QString::fromUtf8(u8"\u5206\u955c: %1").arg(result["panelCount"].toInt());
        }
        if (result.contains("characterCount")) {
            resultParts << QString::fromUtf8(u8"\u89d2\u8272: %1").arg(result["characterCount"].toInt());
        }
        if (result.contains("sceneCount")) {
            resultParts << QString::fromUtf8(u8"\u573a\u666f: %1").arg(result["sceneCount"].toInt());
        }
        
        if (!resultParts.isEmpty()) {
            m_resultLabel->setText(resultParts.join(" / "));
            m_resultLabel->setVisible(true);
        }
    } else {
        m_resultLabel->setVisible(false);
    }
}

void AnalysisProgressWidget::reset()
{
    m_state = State::Idle;
    m_progress = 0;
    m_progressText.clear();
    m_result = QJsonObject();
    m_fadeOutStarted = false;
    
    m_progressBar->setValue(0);
    m_statusLabel->clear();
    m_resultLabel->setVisible(false);
    m_fadeOutTimer->stop();
    
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    setMinimumHeight(0);
    setMaximumHeight(0);
    hide();
    
    updateGeometry();
    if (parentWidget() && parentWidget()->layout()) {
        parentWidget()->layout()->activate();
    }
}

void AnalysisProgressWidget::onFadeOutTimer()
{
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    setMinimumHeight(0);
    setMaximumHeight(0);
    hide();
    
    if (parentWidget() && parentWidget()->layout()) {
        parentWidget()->layout()->update();
    }
    
    emit animationFinished();
}

void AnalysisProgressWidget::updateDisplay()
{
    QString statusText = stateToText(m_state);
    QString statusColor = stateToColor(m_state);
    
    if (!m_progressText.isEmpty() && (m_state == State::Streaming || m_state == State::Processing)) {
        statusText = m_progressText;
    }
    
    m_statusLabel->setText(statusText);
    m_statusLabel->setStyleSheet(STATUS_LABEL_STYLE.arg(statusColor));
    
    if (m_state == State::Completed) {
        m_progressBar->setValue(100);
    }
}

QString AnalysisProgressWidget::stateToText(State state) const
{
    switch (state) {
        case State::Idle:
            return QString::fromUtf8(u8"\u672a\u5f00\u59cb");
        case State::Connecting:
            return QString::fromUtf8(u8"\u8fde\u63a5\u4e2d");
        case State::Streaming:
            return QString::fromUtf8(u8"\u5206\u6790\u4e2d");
        case State::Processing:
            return QString::fromUtf8(u8"\u5904\u7406\u4e2d");
        case State::Completed:
            return QString::fromUtf8(u8"\u5df2\u5b8c\u6210");
        case State::Failed:
            return QString::fromUtf8(u8"\u5931\u8d25");
        default:
            return QString();
    }
}

QString AnalysisProgressWidget::stateToColor(State state) const
{
    switch (state) {
        case State::Idle:
            return "#6B7280";
        case State::Connecting:
        case State::Streaming:
        case State::Processing:
            return "#3B82F6";
        case State::Completed:
            return "#10B981";
        case State::Failed:
            return "#EF4444";
        default:
            return "#6B7280";
    }
}


#include "AnalysisStatusManager.h"
#include "EncodingUtils.h"
#include <QPushButton>
#include <QLabel>

AnalysisStatusManager* AnalysisStatusManager::m_instance = nullptr;

AnalysisStatusManager::AnalysisStatusManager(QObject *parent)
    : QObject(parent)
{
    initStyles();
}

AnalysisStatusManager* AnalysisStatusManager::instance()
{
    if (!m_instance) {
        m_instance = new AnalysisStatusManager();
    }
    return m_instance;
}

void AnalysisStatusManager::initStyles()
{
    m_styles[Status::Ready] = {
        TR("开始分析"),
        TR("分析任务 就绪"),
        "#374151",
        true
    };
    
    m_styles[Status::Processing] = {
        TR("正在生成..."),
        TR("分析任务 进行中"),
        "#3B82F6",
        false
    };
    
    m_styles[Status::Success] = {
        TR("生成完成"),
        TR("分析任务 已完成"),
        "#10B981",
        true
    };
    
    m_styles[Status::Failed] = {
        TR("生成失败"),
        TR("分析任务 失败"),
        "#EF4444",
        true
    };
}

AnalysisStatusManager::StatusStyle AnalysisStatusManager::getStyle(Status status) const
{
    return m_styles.value(status);
}

QString AnalysisStatusManager::statusToString(Status status) const
{
    switch (status) {
        case Status::Ready: return "ready";
        case Status::Processing: return "processing";
        case Status::Success: return "success";
        case Status::Failed: return "failed";
        default: return "unknown";
    }
}

AnalysisStatusManager::Status AnalysisStatusManager::stringToStatus(const QString &str) const
{
    if (str == "ready") return Status::Ready;
    if (str == "processing") return Status::Processing;
    if (str == "success") return Status::Success;
    if (str == "failed") return Status::Failed;
    return Status::Ready;
}

void AnalysisStatusManager::applyButtonStyle(QPushButton *button, QLabel *statusLabel, 
                                              Status status, const QString &extraInfo)
{
    if (!button && !statusLabel) return;
    
    StatusStyle style = getStyle(status);
    
    if (button) {
        button->setEnabled(style.buttonEnabled);
        QString buttonText = style.buttonText;
        if (!extraInfo.isEmpty()) {
            buttonText = extraInfo;
        }
        button->setText(buttonText);
    }
    
    if (statusLabel) {
        statusLabel->setText(style.statusText);
        statusLabel->setStyleSheet(QString("color: %1; font-size: 14px;").arg(style.statusColor));
    }
}

bool AnalysisStatusManager::canStartAnalysis(Status currentStatus) const
{
    return currentStatus != Status::Processing;
}

bool AnalysisStatusManager::isTerminalStatus(Status status) const
{
    return status == Status::Success || status == Status::Failed;
}

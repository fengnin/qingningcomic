#ifndef ANALYSISSTATUSMANAGER_H
#define ANALYSISSTATUSMANAGER_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QJsonObject>
#include <mutex>

class QPushButton;
class QLabel;

class AnalysisStatusManager : public QObject
{
    Q_OBJECT

public:
    enum class Status {
        Ready,
        Processing,
        Success,
        Failed
    };

    struct StatusStyle {
        QString buttonText;
        QString statusText;
        QString statusColor;
        bool buttonEnabled;
    };

    static AnalysisStatusManager* instance();
    
    StatusStyle getStyle(Status status) const;
    QString statusToString(Status status) const;
    Status stringToStatus(const QString &str) const;
    
    void applyButtonStyle(QPushButton *button, QLabel *statusLabel, Status status, 
                          const QString &extraInfo = QString());
    
    bool canStartAnalysis(Status currentStatus) const;
    bool isTerminalStatus(Status status) const;

signals:
    void statusChanged(Status newStatus);

private:
    AnalysisStatusManager(QObject *parent = nullptr);
    AnalysisStatusManager(const AnalysisStatusManager&) = delete;
    AnalysisStatusManager& operator=(const AnalysisStatusManager&) = delete;
    
    void initStyles();
    
    static AnalysisStatusManager *m_instance;
    static std::once_flag m_instanceOnceFlag;
    QMap<Status, StatusStyle> m_styles;
};

#endif // ANALYSISSTATUSMANAGER_H

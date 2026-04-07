#ifndef ANALYSISPROGRESSWIDGET_H
#define ANALYSISPROGRESSWIDGET_H

#include <QWidget>
#include <QProgressBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>
#include <QJsonObject>

class AnalysisProgressWidget : public QWidget
{
    Q_OBJECT

public:
    enum class State {
        Idle,
        Connecting,
        Streaming,
        Processing,
        Completed,
        Failed
    };

    explicit AnalysisProgressWidget(QWidget *parent = nullptr);
    
    void setState(State state);
    void setProgress(int value);
    void setProgressText(const QString &text);
    void setResult(const QJsonObject &result);
    void reset();
    
    State currentState() const { return m_state; }
    int progress() const { return m_progress; }

signals:
    void animationFinished();

private slots:
    void onFadeOutTimer();

private:
    void setupUI();
    void updateDisplay();
    QString stateToText(State state) const;
    QString stateToColor(State state) const;

    QVBoxLayout *m_mainLayout;
    QProgressBar *m_progressBar;
    QLabel *m_statusLabel;
    QLabel *m_resultLabel;
    
    State m_state = State::Idle;
    int m_progress = 0;
    QString m_progressText;
    QJsonObject m_result;
    
    QTimer *m_fadeOutTimer;
    bool m_fadeOutStarted = false;
};

#endif // ANALYSISPROGRESSWIDGET_H

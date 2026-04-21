#ifndef BATCHIMAGEPROCESSOR_H
#define BATCHIMAGEPROCESSOR_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <functional>

class BatchImageProcessor : public QObject
{
    Q_OBJECT

public:
    struct BatchState {
        int totalCount = 0;
        int currentIndex = 0;
        int successCount = 0;
        int failedCount = 0;
        bool cancelled = false;
    };

    explicit BatchImageProcessor(QObject* parent = nullptr);
    virtual ~BatchImageProcessor() = default;

    void cancelGeneration();
    void startBatch(int totalCount);
    virtual void finishBatch();
    bool isGenerating() const { return m_generating; }
    QString lastError() const { return m_lastError; }
    const BatchState& batchState() const { return m_batchState; }

signals:
    void batchProgress(int current, int total, const QString& info);
    void batchCompleted(int successCount, int failedCount);
    void errorOccurred(const QString& operation, const QString& message);

protected:
    void advanceProgress(const QString& info = QString());
    void recordSuccess();
    void recordFailure();
    bool isCancelled() const { return m_batchState.cancelled; }
    bool hasMore() const { return m_batchState.currentIndex < m_batchState.totalCount; }
    bool shouldContinue() const;
    void scheduleNext(std::function<void()> callback, int delayMs = 100);
    
    void setGenerating(bool generating);
    void setError(const QString& message);
    void clearError();
    
    BatchState m_batchState;
    bool m_generating = false;
    QString m_lastError;
};

#endif // BATCHIMAGEPROCESSOR_H

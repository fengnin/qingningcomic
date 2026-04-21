#include "services/BatchImageProcessor.h"

BatchImageProcessor::BatchImageProcessor(QObject* parent)
    : QObject(parent)
{
}

void BatchImageProcessor::cancelGeneration()
{
    m_batchState.cancelled = true;
    setGenerating(false);
}

void BatchImageProcessor::setGenerating(bool generating)
{
    m_generating = generating;
}

void BatchImageProcessor::setError(const QString& message)
{
    m_lastError = message;
}

void BatchImageProcessor::clearError()
{
    m_lastError.clear();
}

void BatchImageProcessor::startBatch(int totalCount)
{
    m_batchState = BatchState();
    m_batchState.totalCount = totalCount;
    m_batchState.cancelled = false;
    setGenerating(true);
    clearError();
}

void BatchImageProcessor::advanceProgress(const QString& info)
{
    m_batchState.currentIndex++;
    emit batchProgress(m_batchState.currentIndex, m_batchState.totalCount, info);
}

void BatchImageProcessor::recordSuccess()
{
    m_batchState.successCount++;
}

void BatchImageProcessor::recordFailure()
{
    m_batchState.failedCount++;
}

void BatchImageProcessor::finishBatch()
{
    setGenerating(false);
    emit batchCompleted(m_batchState.successCount, m_batchState.failedCount);
}

bool BatchImageProcessor::shouldContinue() const
{
    return m_generating && !m_batchState.cancelled && 
           m_batchState.currentIndex < m_batchState.totalCount;
}

void BatchImageProcessor::scheduleNext(std::function<void()> callback, int delayMs)
{
    if (shouldContinue()) {
        QTimer::singleShot(delayMs, callback);
    } else {
        finishBatch();
    }
}

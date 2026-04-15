#include "RetryPolicy.h"
#include "EncodingUtils.h"
#include <QThread>
#include <QtMath>

RetryPolicy::RetryPolicy(QObject* parent)
    : QObject(parent)
{
}

RetryPolicy::RetryPolicy(const RetryConfig& config, QObject* parent)
    : QObject(parent)
    , m_config(config)
{
}

void RetryPolicy::setConfig(const RetryConfig& config)
{
    m_config = config;
}

bool RetryPolicy::shouldRetry(int currentAttempt, const QString& error) const
{
    if (currentAttempt >= m_config.maxAttempts) {
        return false;
    }
    
    return isRetryableError(error);
}

qint64 RetryPolicy::calculateDelayMs(int attempt) const
{
    qint64 delay = static_cast<qint64>(m_config.baseDelayMs * 
                     qPow(m_config.backoffMultiplier, attempt));
    
    return qMin(delay, static_cast<qint64>(m_config.maxDelayMs));
}

bool RetryPolicy::executeWithRetry(Operation operation,
                                    RetryCallback onRetry,
                                    CompleteCallback onComplete)
{
    m_attempts.clear();
    
    for (int i = 0; i < m_config.maxAttempts; ++i) {
        RetryAttempt attempt;
        attempt.attemptNumber = i + 1;
        attempt.timestamp = QDateTime::currentDateTime();
        
        try {
            bool result = operation();
            attempt.success = true;
            m_attempts.append(attempt);
            
            if (onComplete) {
                onComplete(m_attempts);
            }
            
            return result;
        } catch (const std::exception& e) {
            handleException(attempt, "std::exception", QString::fromUtf8(e.what()));
            
            if (!shouldContinueRetry(i, attempt.errorMessage)) {
                if (onComplete) {
                    onComplete(m_attempts);
                }
                return false;
            }
            
            performRetry(i, attempt, onRetry);
        } catch (...) {
            handleException(attempt, "unknown", tr("未知错误"));
            
            if (!shouldContinueRetry(i, attempt.errorMessage)) {
                if (onComplete) {
                    onComplete(m_attempts);
                }
                return false;
            }
            
            performRetry(i, attempt, onRetry);
        }
    }
    
    if (onComplete) {
        onComplete(m_attempts);
    }
    
    return false;
}

bool RetryPolicy::executeOnce(Operation operation,
                               int maxAttempts,
                               int baseDelayMs,
                               RetryCallback onRetry)
{
    RetryConfig config;
    config.maxAttempts = maxAttempts;
    config.baseDelayMs = baseDelayMs;
    
    RetryPolicy policy(config);
    return policy.executeWithRetry(operation, onRetry);
}

int RetryPolicy::successfulAttempts() const
{
    int count = 0;
    for (const auto& attempt : m_attempts) {
        if (attempt.success) {
            ++count;
        }
    }
    return count;
}

int RetryPolicy::failedAttempts() const
{
    int count = 0;
    for (const auto& attempt : m_attempts) {
        if (!attempt.success) {
            ++count;
        }
    }
    return count;
}

QString RetryPolicy::lastError() const
{
    if (m_attempts.isEmpty()) {
        return QString();
    }
    
    return m_attempts.last().errorMessage;
}

bool RetryPolicy::isFinalFailure() const
{
    if (m_attempts.isEmpty()) {
        return false;
    }
    
    const auto& lastAttempt = m_attempts.last();
    return !lastAttempt.success && 
           lastAttempt.attemptNumber >= m_config.maxAttempts;
}

void RetryPolicy::reset()
{
    m_attempts.clear();
}

void RetryPolicy::handleException(RetryAttempt& attempt, const QString& type, const QString& message)
{
    attempt.errorType = type;
    attempt.errorMessage = message;
    attempt.success = false;
    m_attempts.append(attempt);
}

bool RetryPolicy::shouldContinueRetry(int currentAttempt, const QString& error) const
{
    if (currentAttempt >= m_config.maxAttempts - 1) {
        return false;
    }
    
    return isRetryableError(error);
}

void RetryPolicy::performRetry(int currentAttempt, RetryAttempt& attempt, RetryCallback onRetry)
{
    attempt.delayMs = calculateDelayMs(currentAttempt);
    
    if (onRetry) {
        onRetry(currentAttempt + 1, attempt.errorMessage);
    }
    
    QThread::msleep(attempt.delayMs);
}

bool RetryPolicy::isRetryableError(const QString& error) const
{
    QString lowerError = error.toLower();
    
    for (const QString& pattern : m_config.retryableErrors) {
        if (lowerError.contains(pattern.toLower())) {
            return true;
        }
    }
    
    return false;
}

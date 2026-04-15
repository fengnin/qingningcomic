#ifndef RETRYPOLICY_H
#define RETRYPOLICY_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QList>
#include <QThread>
#include <functional>
#include "EncodingUtils.h"

struct RetryAttempt {
    int attemptNumber = 0;
    QDateTime timestamp;
    QString errorType;
    QString errorMessage;
    qint64 delayMs = 0;
    bool success = false;
};

struct RetryConfig {
    int maxAttempts = 3;
    int baseDelayMs = 1000;
    double backoffMultiplier = 2.0;
    int maxDelayMs = 30000;
    
    QStringList retryableErrors;
    
    RetryConfig()
    {
        retryableErrors << "gone away"
                        << "Lost connection"
                        << "Connection was killed"
                        << "timeout"
                        << "network"
                        << "connection refused";
    }
};

class RetryPolicy : public QObject
{
    Q_OBJECT

public:
    using RetryCallback = std::function<void(int, const QString&)>;
    using CompleteCallback = std::function<void(const QList<RetryAttempt>&)>;
    using Operation = std::function<bool()>;

    explicit RetryPolicy(QObject* parent = nullptr);
    explicit RetryPolicy(const RetryConfig& config, QObject* parent = nullptr);
    
    void setConfig(const RetryConfig& config);
    const RetryConfig& config() const { return m_config; }
    
    bool shouldRetry(int currentAttempt, const QString& error) const;
    qint64 calculateDelayMs(int attempt) const;
    
    bool executeWithRetry(Operation operation,
                          RetryCallback onRetry = nullptr,
                          CompleteCallback onComplete = nullptr);
    
    template<typename T>
    T executeWithRetryTyped(std::function<T()> operation,
                            T defaultValue = T(),
                            RetryCallback onRetry = nullptr,
                            CompleteCallback onComplete = nullptr)
    {
        m_attempts.clear();
        
        for (int i = 0; i < m_config.maxAttempts; ++i) {
            RetryAttempt attempt;
            attempt.attemptNumber = i + 1;
            attempt.timestamp = QDateTime::currentDateTime();
            
            try {
                T result = operation();
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
                    return defaultValue;
                }
                
                performRetry(i, attempt, onRetry);
            } catch (...) {
                handleException(attempt, "unknown", TR("未知异常"));
                
                if (!shouldContinueRetry(i, attempt.errorMessage)) {
                    if (onComplete) {
                        onComplete(m_attempts);
                    }
                    return defaultValue;
                }
                
                performRetry(i, attempt, onRetry);
            }
        }
        
        if (onComplete) {
            onComplete(m_attempts);
        }
        
        return defaultValue;
    }
    
    static bool executeOnce(Operation operation,
                            int maxAttempts = 3,
                            int baseDelayMs = 1000,
                            RetryCallback onRetry = nullptr);

    const QList<RetryAttempt>& attempts() const { return m_attempts; }
    int totalAttempts() const { return m_attempts.size(); }
    int successfulAttempts() const;
    int failedAttempts() const;
    QString lastError() const;
    bool isFinalFailure() const;
    void reset();

private:
    void handleException(RetryAttempt& attempt, const QString& type, const QString& message);
    bool shouldContinueRetry(int currentAttempt, const QString& error) const;
    void performRetry(int currentAttempt, RetryAttempt& attempt, RetryCallback onRetry);
    bool isRetryableError(const QString& error) const;
    
    RetryConfig m_config;
    QList<RetryAttempt> m_attempts;
};

#endif // RETRYPOLICY_H

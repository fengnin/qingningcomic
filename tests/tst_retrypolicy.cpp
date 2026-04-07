#include <QtTest>
#include "RetryPolicy.h"

class TestRetryPolicy : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testRetrySuccess();
    void testRetryFailure();
    void testExponentialBackoff();
    void testMaxDelay();
    void testNonRetryableError();
    void testRetryableErrorDetection();
    void testDefaultRetryableErrors();
    void testReset();
    void testExecuteOnce();
};

void TestRetryPolicy::initTestCase()
{
}

void TestRetryPolicy::cleanupTestCase()
{
}

void TestRetryPolicy::testRetrySuccess()
{
    RetryConfig config;
    config.maxAttempts = 3;
    config.baseDelayMs = 100;
    
    RetryPolicy policy(config);
    
    int callCount = 0;
    bool result = policy.executeWithRetry([&]() -> bool {
        callCount++;
        if (callCount < 2) {
            throw std::runtime_error("MySQL server has gone away");
        }
        return true;
    });
    
    QVERIFY(result);
    QCOMPARE(callCount, 2);
    QCOMPARE(policy.totalAttempts(), 2);
}

void TestRetryPolicy::testRetryFailure()
{
    RetryConfig config;
    config.maxAttempts = 3;
    config.baseDelayMs = 100;
    
    RetryPolicy policy(config);
    
    int callCount = 0;
    bool result = policy.executeWithRetry([&]() -> bool {
        callCount++;
        throw std::runtime_error("MySQL server has gone away");
    });
    
    QVERIFY(!result);
    QCOMPARE(callCount, 3);
    QVERIFY(policy.isFinalFailure());
    QVERIFY(!policy.lastError().isEmpty());
}

void TestRetryPolicy::testExponentialBackoff()
{
    RetryConfig config;
    config.maxAttempts = 5;
    config.baseDelayMs = 1000;
    config.backoffMultiplier = 2.0;
    config.maxDelayMs = 30000;
    
    RetryPolicy policy(config);
    
    QCOMPARE(policy.calculateDelayMs(0), 1000);
    QCOMPARE(policy.calculateDelayMs(1), 2000);
    QCOMPARE(policy.calculateDelayMs(2), 4000);
    QCOMPARE(policy.calculateDelayMs(3), 8000);
    QCOMPARE(policy.calculateDelayMs(4), 16000);
}

void TestRetryPolicy::testMaxDelay()
{
    RetryConfig config;
    config.maxAttempts = 10;
    config.baseDelayMs = 1000;
    config.backoffMultiplier = 2.0;
    config.maxDelayMs = 5000;
    
    RetryPolicy policy(config);
    
    QVERIFY(policy.calculateDelayMs(0) <= 5000);
    QVERIFY(policy.calculateDelayMs(3) <= 5000);
    QVERIFY(policy.calculateDelayMs(5) <= 5000);
    QCOMPARE(policy.calculateDelayMs(10), 5000);
}

void TestRetryPolicy::testRetryableErrorDetection()
{
    RetryConfig config;
    config.retryableErrors << "gone away" << "timeout" << "connection";
    
    RetryPolicy policy(config);
    
    QVERIFY(policy.shouldRetry(1, "MySQL server has gone away"));
    QVERIFY(policy.shouldRetry(1, "Connection timeout"));
    QVERIFY(policy.shouldRetry(1, "Lost connection to server"));
    QVERIFY(!policy.shouldRetry(1, "Syntax error"));
    QVERIFY(!policy.shouldRetry(1, "Invalid parameter"));
    QVERIFY(!policy.shouldRetry(3, "MySQL server has gone away"));
}

void TestRetryPolicy::testNonRetryableError()
{
    RetryConfig config;
    config.maxAttempts = 3;
    config.baseDelayMs = 100;
    config.retryableErrors << "gone away" << "timeout";
    
    RetryPolicy policy(config);
    
    int callCount = 0;
    bool result = policy.executeWithRetry([&]() -> bool {
        callCount++;
        throw std::runtime_error("Syntax error in SQL");
    });
    
    QVERIFY(!result);
    QCOMPARE(callCount, 1);
}

void TestRetryPolicy::testDefaultRetryableErrors()
{
    RetryConfig config;
    
    QVERIFY(config.retryableErrors.contains("gone away"));
    QVERIFY(config.retryableErrors.contains("Lost connection"));
    QVERIFY(config.retryableErrors.contains("timeout"));
    QVERIFY(config.retryableErrors.contains("network"));
}

void TestRetryPolicy::testReset()
{
    RetryConfig config;
    config.maxAttempts = 3;
    config.baseDelayMs = 100;
    
    RetryPolicy policy(config);
    
    policy.executeWithRetry([&]() -> bool {
        throw std::runtime_error("MySQL server has gone away");
    });
    
    QVERIFY(policy.totalAttempts() > 0);
    
    policy.reset();
    
    QCOMPARE(policy.totalAttempts(), 0);
    QVERIFY(policy.lastError().isEmpty());
}

void TestRetryPolicy::testExecuteOnce()
{
    int callCount = 0;
    
    bool result = RetryPolicy::executeOnce([&]() -> bool {
        callCount++;
        if (callCount < 2) {
            throw std::runtime_error("MySQL server has gone away");
        }
        return true;
    }, 3, 100);
    
    QVERIFY(result);
    QCOMPARE(callCount, 2);
}

QTEST_MAIN(TestRetryPolicy)
#include "tst_retrypolicy.moc"

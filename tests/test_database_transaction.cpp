#include <QCoreApplication>
#include <QTest>
#include <QFuture>
#include <QtConcurrent>
#include "DatabaseManager.h"
#include "Logger.h"

class TestDatabaseTransaction : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    void testBasicTransaction();
    void testTransactionRollback();
    void testTransactionBlock();
    void testConcurrentAccess();
    void testMutexLock();
    void testTryLock();

private:
    bool connectToDatabase();
    void createTestTable();
    void dropTestTable();
};

void TestDatabaseTransaction::initTestCase()
{
    Logger::instance()->setMinLevel(Logger::Level::Debug);
    
    if (!connectToDatabase()) {
        QSKIP("无法连接数据库，跳过测试");
    }
    
    createTestTable();
}

void TestDatabaseTransaction::cleanupTestCase()
{
    dropTestTable();
}

bool TestDatabaseTransaction::connectToDatabase()
{
    DatabaseConfig config;
    config.host = qEnvironmentVariable("DB_HOST", "localhost");
    config.port = qEnvironmentVariable("DB_PORT", "3306").toInt();
    config.database = qEnvironmentVariable("DB_NAME", "qingning_comic");
    config.username = qEnvironmentVariable("DB_USER", "qingning");
    config.password = qEnvironmentVariable("DB_PASS", "123456");
    
    return DatabaseManager::instance()->connect(config);
}

void TestDatabaseTransaction::createTestTable()
{
    QString createSql = R"(
        CREATE TABLE IF NOT EXISTS test_transaction (
            id INT AUTO_INCREMENT PRIMARY KEY,
            name VARCHAR(100),
            value INT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        ) ENGINE=InnoDB
    )";
    
    DatabaseManager::instance()->executeSql(createSql);
    DatabaseManager::instance()->executeSql("TRUNCATE TABLE test_transaction");
}

void TestDatabaseTransaction::dropTestTable()
{
    DatabaseManager::instance()->executeSql("DROP TABLE IF EXISTS test_transaction");
}

void TestDatabaseTransaction::testBasicTransaction()
{
    DatabaseManager* db = DatabaseManager::instance();
    
    QVERIFY(db->beginTransaction());
    QVERIFY(db->inTransaction());
    
    db->insert("test_transaction", {{"name", "test1"}, {"value", 100}});
    db->insert("test_transaction", {{"name", "test2"}, {"value", 200}});
    
    QVERIFY(db->commit());
    QVERIFY(!db->inTransaction());
    
    QVariantMap countResult = db->selectOne("test_transaction", "1=1");
    int count = db->count("test_transaction");
    QCOMPARE(count, 2);
    
    db->executeSql("TRUNCATE TABLE test_transaction");
}

void TestDatabaseTransaction::testTransactionRollback()
{
    DatabaseManager* db = DatabaseManager::instance();
    
    QVERIFY(db->beginTransaction());
    
    db->insert("test_transaction", {{"name", "rollback_test"}, {"value", 999}});
    
    int countBeforeRollback = db->count("test_transaction", "name = ?", {"rollback_test"});
    QCOMPARE(countBeforeRollback, 1);
    
    QVERIFY(db->rollback());
    QVERIFY(!db->inTransaction());
    
    int countAfterRollback = db->count("test_transaction", "name = ?", {"rollback_test"});
    QCOMPARE(countAfterRollback, 0);
}

void TestDatabaseTransaction::testTransactionBlock()
{
    DatabaseManager* db = DatabaseManager::instance();
    
    bool success = db->transaction([&]() -> bool {
        db->insert("test_transaction", {{"name", "block1"}, {"value", 10}});
        db->insert("test_transaction", {{"name", "block2"}, {"value", 20}});
        db->insert("test_transaction", {{"name", "block3"}, {"value", 30}});
        return true;
    });
    
    QVERIFY(success);
    
    int count = db->count("test_transaction", "name LIKE ?", {"block%"});
    QCOMPARE(count, 3);
    
    db->executeSql("TRUNCATE TABLE test_transaction");
    
    success = db->transaction([&]() -> bool {
        db->insert("test_transaction", {{"name", "will_rollback"}, {"value", 1}});
        return false;
    });
    
    QVERIFY(!success);
    
    int countAfterRollback = db->count("test_transaction");
    QCOMPARE(countAfterRollback, 0);
}

void TestDatabaseTransaction::testConcurrentAccess()
{
    DatabaseManager* db = DatabaseManager::instance();
    
    db->executeSql("TRUNCATE TABLE test_transaction");
    
    QList<QFuture<void>> futures;
    
    for (int i = 0; i < 10; ++i) {
        futures.append(QtConcurrent::run([db, i]() {
            for (int j = 0; j < 10; ++j) {
                db->transaction([&]() -> bool {
                    db->insert("test_transaction", {
                        {"name", QString("concurrent_%1_%2").arg(i).arg(j)},
                        {"value", i * 10 + j}
                    });
                    QThread::msleep(1);
                    return true;
                });
            }
        }));
    }
    
    for (auto& future : futures) {
        future.waitForFinished();
    }
    
    int finalCount = db->count("test_transaction");
    QCOMPARE(finalCount, 100);
}

void TestDatabaseTransaction::testMutexLock()
{
    DatabaseManager* db = DatabaseManager::instance();
    
    db->lock();
    
    QVERIFY(db->tryLock(0) == true);
    
    db->unlock();
    db->unlock();
    
    QVERIFY(db->tryLock(100));
    db->unlock();
}

void TestDatabaseTransaction::testTryLock()
{
    DatabaseManager* db = DatabaseManager::instance();
    
    QMutex externalMutex;
    
    QVERIFY(db->tryLock(1000));
    db->unlock();
    
    externalMutex.lock();
    
    QtConcurrent::run([&externalMutex]() {
        QThread::msleep(500);
        externalMutex.unlock();
    });
    
    QVERIFY(db->tryLock(2000));
    db->unlock();
}

QTEST_MAIN(TestDatabaseTransaction)
#include "test_database_transaction.moc"

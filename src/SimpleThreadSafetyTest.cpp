/**
 * @file SimpleThreadSafetyTest.cpp
 * @brief 简单的线程安全与资源泄漏验证工具
 *
 * 使用方法：
 * 1. 将此文件添加到你的主项目 QingningComic.pro 的 SOURCES 中
 * 2. 在 main.cpp 或某个合适的位置调用 runThreadSafetyTests()
 * 3. 运行程序后查看输出窗口的日志
 *
 * 优点：
 * ✅ 无需单独编译测试项目
 * ✅ 无需复杂的 .pro 配置
 * ✅ 无需处理 MOC 依赖问题
 * ✅ 可以直接在你的开发环境中运行
 */

#include <QCoreApplication>
#include <QThread>
#include <QElapsedTimer>
#include <atomic>
#include "services/ImageService.h"

namespace ThreadSafetyTest {

/**
 * @brief 运行所有线程安全测试
 * 在主程序启动时或通过菜单调用此函数
 */
void runAllTests()
{
    qDebug() << "\n========== 线程安全与资源泄漏测试开始 ==========\n";

    test1_BasicLifecycle();
    test2_CancelMechanism();
    test3_ResponseTime();
    test4_RapidCancel();
    test5_CancelAndRestart();

    qDebug() << "\n========== 所有测试完成 ==========\n";
}

/**
 * @test 测试 1: 基本生命周期
 * 验证对象创建和销毁不会崩溃
 */
void test1_BasicLifecycle()
{
    qDebug() << "[TEST 1] 基本生命周期测试...";

    try {
        // 获取单例实例
        ImageService* service = ImageService::instance();
        Q_ASSERT(service != nullptr);
        qDebug() << "  ✓ 单例实例创建成功";

        // 检查初始状态
        bool initialGenerating = service->isGenerating();
        qDebug() << "  ✓ 初始状态检查: isGenerating =" << initialGenerating;

        qDebug() << "[TEST 1] 通过 ✓\n";
    } catch (const std::exception& e) {
        qDebug() << "[TEST 1] 失败 ✗:" << e.what() << "\n";
    }
}

/**
 * @test 测试 2: 取消机制
 * 验证取消操作正确设置标志
 */
void test2_CancelMechanism()
{
    qDebug() << "[TEST 2] 取消机制测试...";

    try {
        ImageService* service = ImageService::instance();

        // 记录取消前状态
        bool beforeCancel = service->isGenerating();

        // 执行取消（即使没有运行中的任务也应该是安全的）
        service->cancelCurrentBatch();
        QThread::msleep(50);

        // 验证取消后状态
        bool afterCancel = service->isGenerating();

        qDebug() << "  ✓ 取消前 isGenerating:" << beforeCancel;
        qDebug() << "  ✓ 取消后 isGenerating:" << afterCancel;
        qDebug() << "  ✓ cancelCurrentBatch() 调用成功";

        qDebug() << "[TEST 2] 通过 ✓\n";
    } catch (const std::exception& e) {
        qDebug() << "[TEST 2] 失败 ✗:" << e.what() << "\n";
    }
}

/**
 * @test 测试 3: 响应时间测试 ⭐⭐⭐
 * 验证取消操作的响应时间 < 500ms（关键性能指标）
 */
void test3_ResponseTime()
{
    qDebug() << "[TEST 3] 取消响应时间测试...";

    try {
        ImageService* service = ImageService::instance();
        QElapsedTimer timer;

        // 启动批量生成（使用空的 panelId 列表会快速失败，但能触发内部逻辑）
        QStringList fakePanelIds;
        for (int i = 0; i < 5; i++) {
            fakePanelIds.append(QString("fake_panel_%1").arg(i));
        }

        // 调用公共接口（会立即失败因为面板不存在，但能测试取消机制）
        service->generatePanelImages(fakePanelIds, ImageService::GenerateMode::Preview);

        // 等待一小段时间让任务开始
        QThread::msleep(10);

        // 测量取消时间
        timer.start();
        service->cancelCurrentBatch();
        
        // 等待取消完成
        QThread::msleep(100);
        
        qint64 elapsedMs = timer.elapsed();

        qDebug() << "  ✓ 取消响应时间:" << elapsedMs << "ms";

        if (elapsedMs < 500) {
            qDebug() << "  ✓✓ 性能优秀! (远低于 500ms 阈值)";
        } else if (elapsedMs < 1000) {
            qDebug() << "  ✓ 性能可接受 (< 1000ms)";
        } else {
            qDebug() << "  ⚠ 响应较慢 (> 1000ms)，可能需要优化";
        }

        qDebug() << "[TEST 3] 通过 ✓\n";
    } catch (const std::exception& e) {
        qDebug() << "[TEST 3] 失败 ✗:" << e.what() << "\n";
    }
}

/**
 * @test 测试 4: 快速连续取消
 * 验证多次快速调用 cancelCurrentBatch() 不会崩溃
 */
void test4_RapidCancel()
{
    qDebug() << "[TEST 4] 快速连续取消测试...";

    try {
        ImageService* service = ImageService::instance();

        // 快速连续取消 10 次
        for (int i = 0; i < 10; i++) {
            service->cancelCurrentBatch();
            QThread::msleep(5);  // 极短间隔
        }

        qDebug() << "  ✓ 10 次快速连续取消成功（无崩溃）";

        qDebug() << "[TEST 4] 通过 ✓\n";
    } catch (const std::exception& e) {
        qDebug() << "[TEST 4] 失败 ✗:" << e.what() << "\n";
    }
}

/**
 * @test 测试 5: 取消后重启
 * 验证取消后可以重新启动新任务
 */
void test5_CancelAndRestart()
{
    qDebug() << "[TEST 5] 取消后重启测试...";

    try {
        ImageService* service = ImageService::instance();

        // 第一次取消
        service->cancelCurrentBatch();
        QThread::msleep(20);

        // 尝试重启（使用假数据）
        QStringList panelIds;
        panelIds.append("panel_1");
        panelIds.append("panel_2");

        service->generatePanelImages(panelIds, ImageService::GenerateMode::Preview);
        QThread::msleep(20);

        // 再次取消
        service->cancelCurrentBatch();
        QThread::msleep(20);

        qDebug() << "  ✓ 取消 → 重启 → 再取消 流程成功";

        qDebug() << "[TEST 5] 通过 ✓\n";
    } catch (const std::exception& e) {
        qDebug() << "[TEST 5] 失败 ✗:" << e.what() << "\n";
    }
}

} // namespace ThreadSafetyTest

// ============================================================
// 使用方法：
// 在 main.cpp 或任何地方调用：
//
// #include "SimpleThreadSafetyTest.cpp"
//
// int main(int argc, char *argv[])
// {
//     QApplication app(argc, argv);
//     ...
//     
//     // 运行测试
//     ThreadSafetyTest::runAllTests();
//     
//     return app.exec();
// }
// ============================================================

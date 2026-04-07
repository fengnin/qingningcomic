#include <QApplication>
#include <QMessageBox>
#include "MainWindow.h"
#include "AppInitializer.h"
#include "Logger.h"
#include "EncodingUtils.h"

int main(int argc, char *argv[])
{
    EncodingUtils::initSystemEncoding();
    
    QApplication app(argc, argv);
    
    Logger::instance()->init();
    LOG_INFO("Application", "QingningComic starting...");
    
    AppInitializer::InitResult initResult = AppInitializer::initialize();
    
    if (!initResult.databaseSuccess) {
        LOG_ERROR("Application", QString("Database initialization failed: %1").arg(initResult.errorMessage));
        QMessageBox::critical(nullptr, QObject::tr("启动失败"),
            QObject::tr("数据库初始化失败：\n%1\n\n请检查 config.ini 配置后重试。").arg(initResult.errorMessage));
        return 1;
    }
    
    if (!initResult.qwenSuccess) {
        LOG_WARNING("Application", QString("QwenClient initialization failed: %1").arg(initResult.errorMessage));
        QMessageBox::warning(nullptr, QObject::tr("警告"),
            QObject::tr("AI 服务初始化失败，部分功能将不可用。\n\n错误信息：%1").arg(initResult.errorMessage));
    }

    MainWindow window;
    window.show();

    int result = app.exec();
    
    LOG_INFO("Application", "QingningComic exiting...");
    return result;
}

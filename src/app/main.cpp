#include <QApplication>
#include <QMessageBox>
#include "app/MainWindow.h"
#include "app/AppInitializer.h"
#include "utils/Logger.h"
#include "utils/EncodingUtils.h"

int main(int argc, char *argv[])
{
    EncodingUtils::initSystemEncoding();

    QApplication app(argc, argv);

    Logger::instance()->init();
    LOG_INFO("Application", "QingningComic starting...");

    AppInitializer::InitResult initResult = AppInitializer::initialize();

    if (!initResult.databaseSuccess) {
        LOG_ERROR("Application", QString("Database initialization failed: %1").arg(initResult.errorMessage));
        QMessageBox::critical(nullptr, QObject::tr("初始化失败"),
            QObject::tr("数据库初始化失败：%1").arg(initResult.errorMessage));
        return 1;
    }

    if (!initResult.qwenSuccess) {
        LOG_WARNING("Application", QString("QwenClient initialization failed: %1").arg(initResult.errorMessage));
        QMessageBox::warning(nullptr, QObject::tr("初始化警告"),
            QObject::tr("QwenClient 初始化失败：%1").arg(initResult.errorMessage));
    }

    MainWindow window;
    window.show();

    int result = app.exec();

    LOG_INFO("Application", "QingningComic exiting...");
    return result;
}

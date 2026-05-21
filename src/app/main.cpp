#include <QApplication>
#include <QMessageBox>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include "app/MainWindow.h"
#include "app/AppInitializer.h"
#include "utils/Logger.h"
#include "utils/EncodingUtils.h"

namespace {
void bootMark(const char* stage)
{
    const QString line = QString("[%1] %2\n")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss.zzz"))
        .arg(QString::fromUtf8(stage));
    QFile f(QDir(QCoreApplication::applicationDirPath().isEmpty()
                    ? QDir::currentPath()
                    : QCoreApplication::applicationDirPath())
            .filePath("boot_trace.log"));
    if (f.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream ts(&f);
        ts << line;
        f.flush();
        f.close();
    }
    fprintf(stderr, "%s", line.toLocal8Bit().constData());
    fflush(stderr);
}
} // namespace

int main(int argc, char *argv[])
{
    bootMark("enter main");

    EncodingUtils::initSystemEncoding();
    bootMark("after initSystemEncoding");

    QApplication app(argc, argv);
    bootMark("after QApplication ctor");

    Logger::instance()->init();
    bootMark("after Logger init");
    LOG_INFO("Application", "QingningComic starting...");

    AppInitializer::InitResult initResult = AppInitializer::initialize();
    bootMark("after AppInitializer::initialize");

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
    bootMark("after MainWindow ctor");
    window.show();
    bootMark("after window.show");

    int result = app.exec();

    LOG_INFO("Application", "QingningComic exiting...");
    return result;
}

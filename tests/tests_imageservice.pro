# ImageService 单元测试项目配置
# 使用 Qt Test 框架

TARGET = tst_imageservice
TEMPLATE = app

CONFIG += c++11 console
CONFIG -= app_bundle

QT += testlib core network concurrent sql

# 测试源文件
SOURCES += \
    tst_imageservice.cpp

# 头文件（需要 MOC 处理 Q_OBJECT 宏）
HEADERS += \
    $$PWD/../include/ImageService.h \
    $$PWD/../include/QwenImageClient.h \
    $$PWD/../include/StorageClient.h \
    $$PWD/../include/StoryboardService.h \
    $$PWD/../include/TaskQueue.h \
    $$PWD/../include/Task.h \
    $$PWD/../include/Panel.h \
    $$PWD/../include/Storyboard.h \
    $$PWD/../include/DatabaseManager.h \
    $$PWD/../include/PromptBuilder.h \
    $$PWD/../include/Logger.h \
    $$PWD/../include/EncodingUtils.h

# 包含目录
INCLUDEPATH += \
    $$PWD/../include \
    $$PWD/../src/utils \
    $$PWD/../src/services \
    $$PWD/../src/api \
    $$PWD/../src/models

# 链接必要的源文件
SOURCES += \
    $$PWD/../src/services/ImageService.cpp \
    $$PWD/../src/api/QwenImageClient.cpp \
    $$PWD/../src/api/StorageClient.cpp \
    $$PWD/../src/services/StoryboardService.cpp \
    $$PWD/../src/services/TaskQueue.cpp \
    $$PWD/../src/models/Panel.cpp \
    $$PWD/../src/models/Storyboard.cpp \
    $$PWD/../src/models/Task.cpp \
    $$PWD/../src/data/DatabaseManager.cpp \
    $$PWD/../src/utils/PromptBuilder.cpp \
    $$PWD/../src/utils/Logger.cpp \
    $$PWD/../src/utils/EncodingUtils.cpp

# 输出目录
DESTDIR = $$PWD/bin
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR = $$PWD/build/moc

# Windows 编码设置
win32-g++ {
    QMAKE_CXXFLAGS += -fexec-charset=UTF-8
    QMAKE_CXXFLAGS += -finput-charset=UTF-8
}

CODECFORSRC = UTF-8

message("========================================")
message("  ImageService Unit Tests")
message("========================================")

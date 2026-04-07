# 青柠漫画 - StoryboardService 失败路径测试项目
# 使用 Qt Test 框架

TARGET = tst_storyboardservice_failure
TEMPLATE = app

CONFIG += c++11 console
CONFIG -= app_bundle

QT += testlib core sql network concurrent

# 测试源文件
SOURCES += \
    tst_storyboardservice_failure.cpp

# 头文件
HEADERS += \
    $$PWD/../include/StoryboardService.h \
    $$PWD/../include/DatabaseManager.h \
    $$PWD/../include/Logger.h \
    $$PWD/../include/EncodingUtils.h \
    $$PWD/../include/TransactionScope.h \
    $$PWD/../include/ServiceContainer.h \
    $$PWD/../include/services/BaseService.h \
    $$PWD/../include/Storyboard.h \
    $$PWD/../include/Panel.h

# 包含目录
INCLUDEPATH += \
    $$PWD/../include \
    $$PWD/../src/services \
    $$PWD/../src/data \
    $$PWD/../src/utils \
    $$PWD/../src/models

# 链接必要的源文件
SOURCES += \
    $$PWD/../src/services/StoryboardService.cpp \
    $$PWD/../src/services/BaseService.cpp \
    $$PWD/../src/data/DatabaseManager.cpp \
    $$PWD/../src/utils/Logger.cpp \
    $$PWD/../src/utils/EncodingUtils.cpp \
    $$PWD/../src/ServiceContainer.cpp \
    $$PWD/../src/models/Storyboard.cpp \
    $$PWD/../src/models/Panel.cpp

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
message("  StoryboardService 失败路径测试")
message("========================================")

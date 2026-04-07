# 青柠漫画 - NovelService 失败路径测试项目
# 使用 Qt Test 框架

TARGET = tst_novelservice_failure
TEMPLATE = app

CONFIG += c++11 console
CONFIG -= app_bundle

QT += testlib core sql network concurrent

# 测试源文件
SOURCES += \
    tst_novelservice_failure.cpp

# 头文件
HEADERS += \
    $$PWD/../include/NovelService.h \
    $$PWD/../include/DatabaseManager.h \
    $$PWD/../include/Logger.h \
    $$PWD/../include/EncodingUtils.h \
    $$PWD/../include/TransactionScope.h \
    $$PWD/../include/ServiceContainer.h \
    $$PWD/../include/services/BaseService.h \
    $$PWD/../include/Novel.h

# 包含目录
INCLUDEPATH += \
    $$PWD/../include \
    $$PWD/../src/services \
    $$PWD/../src/data \
    $$PWD/../src/utils \
    $$PWD/../src/models

# 链接必要的源文件
SOURCES += \
    $$PWD/../src/services/NovelService.cpp \
    $$PWD/../src/services/BaseService.cpp \
    $$PWD/../src/data/DatabaseManager.cpp \
    $$PWD/../src/utils/Logger.cpp \
    $$PWD/../src/utils/EncodingUtils.cpp \
    $$PWD/../src/ServiceContainer.cpp \
    $$PWD/../src/models/Novel.cpp

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
message("  NovelService 失败路径测试")
message("========================================")

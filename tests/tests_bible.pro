# Bible 模型单元测试项目配置
# 使用 Qt Test 框架

TARGET = tst_bible
TEMPLATE = app

CONFIG += c++11 console
CONFIG -= app_bundle

QT += testlib core

# 测试源文件
SOURCES += \
    tst_bible.cpp

# 头文件
HEADERS += \
    $$PWD/../include/Bible.h \
    $$PWD/../include/Character.h \
    $$PWD/../include/Scene.h \
    $$PWD/../include/Storyboard.h \
    $$PWD/../include/Panel.h \
    $$PWD/../include/Job.h \
    $$PWD/../include/Logger.h \
    $$PWD/../include/utils/JsonUtils.h

# 包含目录
INCLUDEPATH += \
    $$PWD/../include \
    $$PWD/../src/utils

# 链接必要的源文件
SOURCES += \
    $$PWD/../src/models/Bible.cpp \
    $$PWD/../src/models/Character.cpp \
    $$PWD/../src/models/Storyboard.cpp \
    $$PWD/../src/models/Panel.cpp \
    $$PWD/../src/models/Job.cpp \
    $$PWD/../src/utils/JsonUtils.cpp \
    $$PWD/../src/utils/Logger.cpp

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
message("  Bible Model Unit Tests")
message("========================================")

# Task 模型单元测试项目配置
# 使用 Qt Test 框架

TARGET = tst_task
TEMPLATE = app

CONFIG += c++11 console
CONFIG -= app_bundle

QT += testlib core

# 测试源文件
SOURCES += \
    tst_task.cpp

# 头文件
HEADERS += \
    $$PWD/../include/Task.h

# 包含目录
INCLUDEPATH += \
    $$PWD/../include

# 链接必要的源文件
SOURCES += \
    $$PWD/../src/models/Task.cpp

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
message("  Task Model Unit Tests")
message("========================================")

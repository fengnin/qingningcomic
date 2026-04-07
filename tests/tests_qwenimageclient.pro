# QwenImageClient 单元测试项目配置
# 使用 Qt Test 框架

TARGET = tst_qwenimageclient
TEMPLATE = app

CONFIG += c++11 console
CONFIG -= app_bundle

QT += testlib core network concurrent

# 测试源文件
SOURCES += \
    tst_qwenimageclient.cpp

# 头文件（需要 MOC 处理 Q_OBJECT 宏）
HEADERS += \
    $$PWD/../include/QwenImageClient.h \
    $$PWD/../include/Logger.h \
    $$PWD/../include/EncodingUtils.h

# 包含目录
INCLUDEPATH += \
    $$PWD/../include \
    $$PWD/../src/utils \
    $$PWD/../src/api

# 链接必要的源文件
SOURCES += \
    $$PWD/../src/api/QwenImageClient.cpp \
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
message("  QwenImageClient Unit Tests")
message("========================================")

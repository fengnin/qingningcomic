# RetryPolicy 单元测试项目配置
TARGET = tst_retrypolicy
TEMPLATE = app

CONFIG += c++11 console
CONFIG += testcase

QT += testlib

# 头文件
HEADERS += \
    $$PWD/../include/RetryPolicy.h

# 源文件
SOURCES += \
    $$PWD/../src/utils/RetryPolicy.cpp \
    tst_retrypolicy.cpp

# 包含目录
INCLUDEPATH += \
    $$PWD/../include \
    $$PWD/../src/utils

# 输出目录
DESTDIR = $$PWD/bin
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR = $$PWD/build/moc

# Windows 编码设置
win32-g++ {
    QMAKE_CXXFLAGS += -fexec-charset=UTF-8
    QMAKE_CXXFLAGS += -finput-charset=UTF-8
}


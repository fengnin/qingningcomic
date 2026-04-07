# PanelCard 单元测试项目配置
# 使用 Qt Test 框架

TARGET = tst_panelcard
TEMPLATE = app

CONFIG += c++11 console
CONFIG -= app_bundle

QT += testlib core widgets

# 测试源文件
SOURCES += \
    tst_panelcard.cpp

# 头文件
HEADERS += \
    $$PWD/../include/components/PanelCard.h \
    $$PWD/../include/components/EditorCardBase.h \
    $$PWD/../include/components/EditorStyles.h \
    $$PWD/../include/Logger.h \
    $$PWD/../include/EncodingUtils.h

# 包含目录
INCLUDEPATH += \
    $$PWD/../include \
    $$PWD/../src/utils \
    $$PWD/../src/components

# 链接必要的源文件
SOURCES += \
    $$PWD/../src/components/PanelCard.cpp \
    $$PWD/../src/components/EditorCardBase.cpp \
    $$PWD/../src/components/EditorStyles.cpp \
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
message("  PanelCard Unit Tests")
message("========================================")

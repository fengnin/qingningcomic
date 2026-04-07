# 青柠漫画 - QwenClient 失败路径测试项目
# 使用 Qt Test 框架

TARGET = tst_qwenclient_failure
TEMPLATE = app

CONFIG += c++11 console
CONFIG -= app_bundle

QT += testlib core network concurrent

# 测试源文件
SOURCES += \
    tst_qwenclient_failure.cpp

# 头文件
HEADERS += \
    $$PWD/../include/QwenClient.h \
    $$PWD/../include/api/QwenJsonRepair.h \
    $$PWD/../include/api/QwenPromptBuilder.h \
    $$PWD/../include/api/QwenStoryboardMerger.h \
    $$PWD/../include/api/QwenStreamHandler.h \
    $$PWD/../include/Logger.h \
    $$PWD/../include/EncodingUtils.h \
    $$PWD/../include/ServiceContainer.h

# 包含目录
INCLUDEPATH += \
    $$PWD/../include \
    $$PWD/../include/api \
    $$PWD/../src/api \
    $$PWD/../src/utils \
    $$PWD/../src/services

# 链接必要的源文件
SOURCES += \
    $$PWD/../src/api/QwenClient.cpp \
    $$PWD/../src/api/QwenJsonRepair.cpp \
    $$PWD/../src/api/QwenPromptBuilder.cpp \
    $$PWD/../src/api/QwenStoryboardMerger.cpp \
    $$PWD/../src/api/QwenStreamHandler.cpp \
    $$PWD/../src/utils/Logger.cpp \
    $$PWD/../src/utils/EncodingUtils.cpp \
    $$PWD/../src/ServiceContainer.cpp

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
message("  QwenClient 失败路径测试")
message("========================================")

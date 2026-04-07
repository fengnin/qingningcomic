# SceneExtractor 单元测试项目配置
# 使用 Qt Test 框架

TARGET = tst_sceneextractor
TEMPLATE = app

CONFIG += c++11 console
CONFIG -= app_bundle

QT += testlib core sql

# 测试源文件
SOURCES += \
    tst_sceneextractor.cpp

# 头文件
HEADERS += \
    $$PWD/../include/SceneExtractor.h \
    $$PWD/../include/Scene.h \
    $$PWD/../include/Logger.h \
    $$PWD/../include/DatabaseManager.h \
    $$PWD/../include/utils/JsonUtils.h

# 包含目录
INCLUDEPATH += \
    $$PWD/../include \
    $$PWD/../src/utils \
    $$PWD/../src/models \
    $$PWD/../src/services

# 链接必要的源文件
SOURCES += \
    $$PWD/../src/services/SceneExtractor.cpp \
    $$PWD/../src/models/Scene.cpp \
    $$PWD/../src/utils/JsonUtils.cpp \
    $$PWD/../src/utils/Logger.cpp \
    $$PWD/../src/data/DatabaseManager.cpp

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
message("  SceneExtractor Unit Tests")
message("========================================")

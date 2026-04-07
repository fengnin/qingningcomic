# 图片生成集成测试项目配置
# 使用 Qt Test 框架

TARGET = tst_imagegeneration
TEMPLATE = app

CONFIG += c++11 console
CONFIG -= app_bundle

QT += testlib core network concurrent sql

# 测试源文件
SOURCES += \
    tst_imagegeneration.cpp

# 头文件（需要 MOC 处理 Q_OBJECT 宏）
HEADERS += \
    $$PWD/../include/ImageService.h \
    $$PWD/../include/BibleImageService.h \
    $$PWD/../include/FileStorage.h \
    $$PWD/../include/Character.h \
    $$PWD/../include/Scene.h \
    $$PWD/../include/Panel.h \
    $$PWD/../include/Storyboard.h \
    $$PWD/../include/StoryboardService.h \
    $$PWD/../include/CharacterExtractor.h \
    $$PWD/../include/SceneExtractor.h \
    $$PWD/../include/DatabaseManager.h \
    $$PWD/../include/Logger.h \
    $$PWD/../include/QwenImageClient.h \
    $$PWD/../include/StorageClient.h \
    $$PWD/../include/TaskQueue.h \
    $$PWD/../include/Task.h

# 包含目录
INCLUDEPATH += \
    $$PWD/../include \
    $$PWD/../src/utils \
    $$PWD/../src/services \
    $$PWD/../src/api \
    $$PWD/../src/data \
    $$PWD/../src/models

# 链接必要的源文件
# 注意：Scene 类在 Scene.h 中内联实现，没有 Scene.cpp
SOURCES += \
    $$PWD/../src/services/ImageService.cpp \
    $$PWD/../src/services/BibleImageService.cpp \
    $$PWD/../src/data/FileStorage.cpp \
    $$PWD/../src/models/Character.cpp \
    $$PWD/../src/models/Panel.cpp \
    $$PWD/../src/models/Storyboard.cpp \
    $$PWD/../src/services/StoryboardService.cpp \
    $$PWD/../src/services/CharacterExtractor.cpp \
    $$PWD/../src/services/SceneExtractor.cpp \
    $$PWD/../src/data/DatabaseManager.cpp \
    $$PWD/../src/utils/Logger.cpp \
    $$PWD/../src/utils/FileManager.cpp \
    $$PWD/../src/api/QwenImageClient.cpp \
    $$PWD/../src/api/StorageClient.cpp \
    $$PWD/../src/services/TaskQueue.cpp \
    $$PWD/../src/models/Task.cpp \
    $$PWD/../src/utils/PromptBuilder.cpp \
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
message("  图片生成集成测试")
message("========================================")

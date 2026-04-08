
# 青柠漫画 - C++ 版本
# qmake 项目配置文件
# 适用于：Qt 5.15.2 + MinGW 64-bit

# 项目名称
TARGET = QingningComic

# 项目类型：应用程序
TEMPLATE = app

# C++11 标准
CONFIG += c++11

# 启用 Qt 模块
QT += core network sql widgets concurrent

# 源文件列表
SOURCES += \
    src/app/main.cpp \
    src/app/AppInitializer.cpp \
    src/app/MainWindow.cpp \
    src/viewmodels/BaseViewModel.cpp \
    src/viewmodels/NovelViewModel.cpp \
    src/viewmodels/StoryboardViewModel.cpp \
    src/pages/DashboardPage.cpp \
    src/pages/CharacterDetailPage.cpp \
    src/pages/ExportPage.cpp \
    src/pages/NovelUploadPage.cpp \
    src/pages/NovelPage.cpp \
    src/pages/NovelDetailPage.cpp \
    src/data/DatabaseManager.cpp \
    src/data/FileStorage.cpp \
    src/data/SqlBuilder.cpp \
    src/utils/BibleCache.cpp \
    src/models/Novel.cpp \
    src/models/Character.cpp \
    src/models/Storyboard.cpp \
    src/models/Panel.cpp \
    src/models/Job.cpp \
    src/models/Task.cpp \
    src/models/Bible.cpp \
    src/services/NovelService.cpp \
    src/services/StoryboardService.cpp \
    src/services/AnalysisService.cpp \
    src/services/BaseService.cpp \
    src/services/ExportService.cpp \
    src/services/TaskQueue.cpp \
    src/services/CharacterExtractor.cpp \
    src/services/SceneExtractor.cpp \
    src/services/BibleGenerator.cpp \
    src/api/QwenClient.cpp \
    src/api/StorageClient.cpp \
    src/api/QwenImageClient.cpp \
    src/api/VolcEngineImageClient.cpp \
    src/api/VolcEngineSignature.cpp \
    src/api/VolcEngineRequest.cpp \
    src/api/VolcEngineResponse.cpp \
    src/api/QwenJsonRepair.cpp \
    src/api/QwenPromptBuilder.cpp \
    src/api/QwenStoryboardMerger.cpp \
    src/api/QwenStreamHandler.cpp \
    src/utils/FileManager.cpp \
    src/utils/JsonUtils.cpp \
    src/utils/StyleManager.cpp \
    src/utils/Logger.cpp \
    src/utils/EncodingUtils.cpp \
    src/utils/AppConfig.cpp \
    src/utils/UserSession.cpp \
    src/utils/JsonRepairAdapter.cpp \
    src/utils/PromptBuilder.cpp \
    src/utils/AsyncImageLoader.cpp \
    src/utils/RetryPolicy.cpp \
    src/components/SuccessDialog.cpp \
    src/components/ConfirmDialog.cpp \
    src/utils/SchemaToPrompt.cpp \
    src/components/ChapterSpinBox.cpp \
    src/components/ChapterSelectDialog.cpp \
    src/components/ImageViewerDialog.cpp \
    src/components/ModeComboBox.cpp \
    src/components/ChapterCard.cpp \
    src/components/StoryboardItem.cpp \
    src/components/BibleItem.cpp \
    src/components/PanelCard.cpp \
    src/components/PanelEditorWidget.cpp \
    src/components/EditorCardBase.cpp \
    src/components/EditorStyles.cpp \
    src/components/AnalysisProgressWidget.cpp \
    src/components/AnalysisResultWidget.cpp \
    src/services/AnalysisStatusManager.cpp \
    src/services/ImageService.cpp \
    src/services/BibleImageService.cpp \
    src/services/BatchImageProcessor.cpp \
    src/services/TaskRegistry.cpp \
    src/services/ServiceContainer.cpp \
    src/api/SharedNetworkManager.cpp \
    src/widgets/FortuneCookieWidget.cpp \
    src/widgets/SidebarWidget.cpp

HEADERS += \
    include/AppInitializer.h \
    include/MainWindow.h \
    include/DashboardPage.h \
    include/NovelUploadPage.h \
    include/ExportPage.h \
    include/NovelPage.h \
    include/NovelDetailPage.h \
    include/CharacterDetailPage.h \
    include/DatabaseManager.h \
    include/FileStorage.h \
    include/SqlBuilder.h \
    include/Novel.h \
    include/Character.h \
    include/Storyboard.h \
    include/Panel.h \
    include/Job.h \
    include/Task.h \
    include/Bible.h \
    include/BibleCache.h \
    include/viewmodels/BaseViewModel.h \
    include/viewmodels/NovelViewModel.h \
    include/viewmodels/StoryboardViewModel.h \
    include/NovelService.h \
    include/StoryboardService.h \
    include/AnalysisService.h \
    include/services/BaseService.h \
    include/services/ExportService.h \
    include/TaskQueue.h \
    include/CharacterExtractor.h \
    include/SceneExtractor.h \
    include/Scene.h \
    include/BibleGenerator.h \
    include/BibleImageService.h \
    include/utils/JsonUtils.h \
    include/QwenClient.h \
    include/StorageClient.h \
    include/QwenImageClient.h \
    include/VolcEngineImageClient.h \
    include/api/VolcEngineSignature.h \
    include/api/VolcEngineRequest.h \
    include/api/VolcEngineResponse.h \
    include/api/QwenJsonRepair.h \
    include/api/QwenPromptBuilder.h \
    include/api/QwenStoryboardMerger.h \
    include/api/QwenStreamHandler.h \
    include/FileManager.h \
    include/StyleManager.h \
    include/Logger.h \
    include/EncodingUtils.h \
    include/utils/AsyncImageLoader.h \
    include/AppConfig.h \
    include/UserSession.h \
    include/SuccessDialog.h \
    include/ConfirmDialog.h \
    include/SchemaToPrompt.h \
    include/JsonRepairAdapter.h \
    include/PromptBuilder.h \
    include/RetryPolicy.h \
    include/components/ChapterSpinBox.h \
    include/components/ChapterSelectDialog.h \
    include/components/ImageViewerDialog.h \
    include/components/ModeComboBox.h \
    include/components/ChapterCard.h \
    include/components/StoryboardItem.h \
    include/components/BibleItem.h \
    include/components/PanelCard.h \
    include/components/PanelEditorWidget.h \
    include/components/EditorCardBase.h \
    include/components/EditorStyles.h \
    include/components/AnalysisProgressWidget.h \
    include/components/AnalysisResultWidget.h \
    include/AnalysisStatusManager.h \
    include/utils/StatusHelper.h \
    include/utils/ShotTypeHelper.h \
    include/utils/SingletonUtils.h \
    include/ServiceContainer.h \
    include/TaskRegistry.h \
    include/ImageService.h \
    include/BatchImageProcessor.h \
    include/SharedNetworkManager.h \
    include/FortuneCookieWidget.h \
    include/SidebarWidget.h

# 包含目录
INCLUDEPATH += \
    include \
    src \
    src/data \
    src/models \
    src/services \
    src/api \
    src/utils \
    src/components \
    include/components

# 目标文件存放目录
DESTDIR = bin

# 中间文件存放目录
MOC_DIR = build/moc
RCC_DIR = build/rcc
UI_DIR = build/ui
OBJECTS_DIR = build/obj

# Windows 特定配置
win32 {
    QMAKE_CXXFLAGS += -Wall
    
    win32-g++ {
        QMAKE_CXXFLAGS += -fexec-charset=UTF-8
        QMAKE_CXXFLAGS += -finput-charset=UTF-8
        QMAKE_CXXFLAGS += -fwide-exec-charset=UTF-8
        QMAKE_CFLAGS += -fexec-charset=UTF-8
        QMAKE_CFLAGS += -finput-charset=UTF-8
    }
    
    msvc {
        QMAKE_CXXFLAGS += /utf-8
        QMAKE_CFLAGS += /utf-8
        QMAKE_CXXFLAGS += /source-charset:utf-8
        QMAKE_CXXFLAGS += /execution-charset:utf-8
    }
    
    LIBS += -lkernel32
}

CODECFORSRC = UTF-8
CODECFORTR = UTF-8
QMAKE_DEFAULT_ENCODING = UTF-8

# Debug 和 Release 配置
CONFIG(debug, debug|release) {
    # Debug 模式
    DEFINES += QT_DEBUG
    message(Debug build)
} else {
    # Release 模式
    DEFINES += QT_NO_DEBUG_OUTPUT
    message(Release build)
}

# 部署 schemas 目录
schemas.files = schemas
schemas.path = $$DESTDIR
INSTALLS += schemas

# 自动复制 schemas 到 build 目录
win32 {
    copy_schemas.commands = $(COPY_DIR) $$shell_path($$PWD/schemas) $$shell_path($$OUT_PWD/bin/schemas)
    first.depends = $(first) copy_schemas
    QMAKE_EXTRA_TARGETS += copy_schemas
}

# 打印配置信息
message("========================================")
message("  Qingning Comic - C++ Version")
message("  Qt Version: $$[QT_VERSION]")
message("  Compiler: $$QMAKE_COMPILER")
message("========================================")

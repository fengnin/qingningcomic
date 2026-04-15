# 青柠漫画项目进度文档

## 项目概述
青柠漫画 - AI 小说转漫画平台 (C++ 版本)
- 技术栈：C++11 + Qt 5.15.2
- 编译器：MinGW 64-bit
- 构建系统：qmake

---

## 一、原仓库与 C++ 版本功能对比

### 1. 原仓库 (Node.js/AWS Lambda) 功能模块

| 模块 | Lambda 函数 | 功能描述 | API 端点 |
|------|-------------|----------|----------|
| **小说管理** | novels | 小说的 CRUD 操作 | `GET/POST /novels`, `GET/DELETE /novels/{id}` |
| | analyze-novel | 创建小说分析任务 | `POST /novels/{id}/analyze` |
| | analyze-worker | SQS Worker，执行小说分析 | SQS 触发器 |
| **Bible管理** | bible | 角色/场景圣经管理 | `GET /novels/{id}/bible`, `PATCH /novels/{id}/bible/...` |
| | reference-worker | 自动生成参考图片 | SQS 触发器 |
| **角色管理** | characters | 角色配置管理 | `GET/PUT /characters/{charId}` |
| | generate-portrait | 生成四视角肖像图 | `POST /characters/{charId}/configurations/{configId}/portrait` |
| **分镜脚本** | storyboards | 查询分镜脚本 | `GET /novels/{id}/storyboards`, `GET /storyboards/{id}` |
| | panels | 面板查询和更新 | `GET/PATCH /panels/{panelId}` |
| | generate-panels | 创建面板图片生成任务 | `POST /storyboards/{id}/panels/generate` |
| | panel-worker | 执行面板图片生成 | DynamoDB Stream 触发器 |
| | edit-panel | 面板图片编辑 (inpaint/bg_swap/outpaint) | `POST /panels/{panelId}/edit` |
| **改稿模块** | change-request | 接收自然语言改稿请求 | `POST /change-requests` |
| | change-request-worker | 执行改稿操作 | SQS 触发器 |
| **导出模块** | export | 创建导出任务 (PDF/Webtoon/Resources) | `POST /exports`, `GET /exports/{id}` |
| | export-worker | 执行导出文件生成 | SQS 触发器 |
| **任务管理** | jobs | 查询任务状态和进度 | `GET /jobs`, `GET /jobs/{id}` |
| | retry-handler | 重试失败任务 | EventBridge 触发器 |

### 2. 原仓库工具类

| 工具类 | 功能描述 |
|--------|----------|
| qwen-adapter | Qwen API 适配器，长文本分块、JSON Schema 验证、自动重试 |
| imagen-adapter | Google Imagen/Gemini Images API 适配器，图片生成和编辑 |
| bible-manager | 角色/场景圣经管理器，版本化存储、S3 混合存储 |
| prompt-builder | Prompt 构建工具，视角/姿态/风格/表情映射表 |
| s3-utils | S3 工具类，上传、下载、预签名 URL |
| s3-image-utils | S3 图片工具类，S3 URI 转 Base64 |
| ai-secrets | AI 密钥管理，从 AWS Secrets Manager 获取凭证 |
| auth | 认证工具类（网页版使用 Supabase Auth） |
| response | 统一响应格式工具 |
| schema-to-prompt | JSON Schema 转 Prompt 工具 |

---

## 二、C++ 版本实现状态

### ✅ 已完成的功能

#### 2.1 数据模型层 (100% 完成)

| 文件 | 功能描述 | 对应原仓库 |
|------|----------|------------|
| [Novel.h](include/Novel.h) | 小说数据模型，包含状态枚举 | novels 函数 |
| [Character.h](include/Character.h) | 角色数据模型，包含外观配置 | characters 函数 |
| [Storyboard.h](include/Storyboard.h) | 分镜数据模型 | storyboards 函数 |
| [Panel.h](include/Panel.h) | 面板数据模型，包含对白、角色姿态 | panels 函数 |
| [Job.h](include/Job.h) | 任务数据模型，包含状态枚举 | jobs 函数 |
| [Bible.h](include/Bible.h) | 圣经数据模型，角色/场景描述 | bible 函数 |
| [Task.h](include/Task.h) | 任务模型，异步任务管理 | - |
| [Scene.h](include/Scene.h) | 场景数据模型 | bible 函数 |
| **[ChangeRequest.h](include/ChangeRequest.h)** | **改稿请求数据模型** | **change-request** |

#### 2.2 工具类层 (100% 完成)

| 文件 | 功能描述 | 对应原仓库 |
|------|----------|------------|
| [FileManager.h](include/FileManager.h) | 文件读写、目录操作 | s3-utils (本地版) |
| [JsonUtils.h](include/utils/JsonUtils.h) | JSON 序列化/反序列化 | 原仓库内置 JSON |
| [StyleManager.h](include/StyleManager.h) | UI 样式管理 | 前端 CSS |
| [Logger.h](include/Logger.h) | 日志系统 | - |
| [EncodingUtils.h](include/EncodingUtils.h) | 编码工具，解决中文乱码 | - |
| [AppConfig.h](include/AppConfig.h) | 应用配置管理 | Secrets Manager |
| [AsyncImageLoader.h](include/utils/AsyncImageLoader.h) | 异步图片加载器 | - |
| [BibleCache.h](include/BibleCache.h) | 圣经缓存管理 | - |
| [JsonRepairAdapter.h](include/JsonRepairAdapter.h) | JSON 修复适配器 | jsonrepair |
| [PromptBuilder.h](include/PromptBuilder.h) | Prompt 构建工具 | prompt-builder |
| [RetryPolicy.h](include/RetryPolicy.h) | 重试策略 | - |
| [SchemaToPrompt.h](include/SchemaToPrompt.h) | JSON Schema 转 Prompt | schema-to-prompt |
| [SingletonUtils.h](include/utils/SingletonUtils.h) | 单例模式工具 | - |
| [StatusHelper.h](include/utils/StatusHelper.h) | 状态辅助工具 | - |
| [ShotTypeHelper.h](include/utils/ShotTypeHelper.h) | 镜头类型辅助工具 | - |
| [UserSession.h](include/UserSession.h) | 用户会话管理 | auth |

#### 2.3 数据访问层 (100% 完成)

| 文件 | 功能描述 | 状态 |
|------|----------|------|
| [DatabaseManager.h](include/DatabaseManager.h) | 数据库连接、CRUD 操作封装 | ✅ 已实现 |
| [FileStorage.h](include/FileStorage.h) | 文件存储管理 | ✅ 已实现 |
| [SqlBuilder.h](include/SqlBuilder.h) | SQL 语句构建器 | ✅ 已实现 |

#### 2.4 服务层 (100% 完成)

| 服务 | 功能描述 | 对应原仓库 | 状态 |
|------|----------|------------|------|
| NovelService | 小说 CRUD 服务 | novels 函数 | ✅ 已实现 |
| StoryboardService | 分镜 CRUD 服务，保存/读取分镜数据 | storyboards 函数 | ✅ 已实现 |
| AnalysisService | 分析服务，调用 Qwen 生成分镜 | analyze-worker | ✅ 已实现 |
| ExportService | 导出服务 | export 函数 | ✅ 已实现 |
| BaseService | 服务基类 | - | ✅ 已实现 |
| BibleGenerator | 圣经生成服务 | bible 函数 | ✅ 已实现 |
| BibleImageService | 圣经图片服务 | reference-worker | ✅ 已实现 |
| CharacterExtractor | 角色提取服务 | - | ✅ 已实现 |
| SceneExtractor | 场景提取服务 | - | ✅ 已实现 |
| ImageService | 图片服务 | - | ✅ 已实现 |
| BatchImageProcessor | 批量图片处理器 | panel-worker | ✅ 已实现 |
| AnalysisStatusManager | 分析状态管理器 | - | ✅ 已实现 |
| ServiceContainer | 服务容器，依赖注入 | - | ✅ 已实现 |
| TaskQueue | 任务队列 | SQS | ✅ 已实现 |
| TaskRegistry | 任务注册表 | - | ✅ 已实现 |
| **ChangeRequestService** | **改稿服务** | **change-request + change-request-worker** | ✅ 已实现 |

#### 2.5 API 客户端层 (100% 完成)

| 客户端 | 功能描述 | 对应原仓库 | 状态 |
|--------|----------|------------|------|
| QwenClient | Qwen API 客户端，文本分析、分镜生成 | qwen-adapter.js | ✅ 已实现 |
| QwenImageClient | 通义万相图像生成客户端 | imagen-adapter.js | ✅ 已实现 |
| VolcEngineImageClient | 火山引擎图像生成客户端（Seedream 3.0） | - | ✅ 已实现 |
| StorageClient | S3 兼容存储客户端，文件上传/下载 | s3-utils.js | ✅ 已实现 |
| SharedNetworkManager | 共享网络管理器 | - | ✅ 已实现 |
| QwenJsonRepair | Qwen JSON 修复工具 | jsonrepair | ✅ 已实现 |
| QwenPromptBuilder | Qwen Prompt 构建器 | prompt-builder | ✅ 已实现 |
| QwenStoryboardMerger | Qwen 分镜合并器 | - | ✅ 已实现 |
| QwenStreamHandler | Qwen 流式响应处理器 | - | ✅ 已实现 |
| VolcEngineRequest | 火山引擎请求构建 | - | ✅ 已实现 |
| VolcEngineResponse | 火山引擎响应解析 | - | ✅ 已实现 |
| VolcEngineSignature | 火山引擎签名计算 | - | ✅ 已实现 |

#### 2.6 UI 页面层 (100% 完成)

| 页面 | 功能描述 | 对应原仓库前端 |
|------|----------|----------------|
| [MainWindow](include/MainWindow.h) | 主窗口，侧边栏导航 | DashboardLayout.tsx |
| [DashboardPage](include/DashboardPage.h) | 总览页面，任务统计 | DashboardPage.tsx |
| [NovelPage](include/NovelPage.h) | 项目空间，作品列表 | NovelsIndexPage.tsx |
| [NovelUploadPage](include/NovelUploadPage.h) | 上传作品表单 | NovelUploadPage.tsx |
| [NovelDetailPage](include/NovelDetailPage.h) | 作品详情，章节/分镜/圣经管理 | NovelDetailPage.tsx |
| [CharacterDetailPage](include/CharacterDetailPage.h) | 角色详情，配置管理 | CharacterDetailPage.tsx |
| [ExportPage](include/ExportPage.h) | 导出中心 | ExportsPage.tsx |

#### 2.7 自定义 UI 组件 (100% 完成)

| 组件 | 功能描述 |
|------|----------|
| ChapterSpinBox | 自定义数字输入框 |
| ModeComboBox | 自定义下拉框 |
| ChapterCard | 章节卡片组件 |
| StoryboardItem | 分镜条目组件 |
| PanelCard | 面板卡片组件 |
| BibleItem | 圣经条目组件 |
| AnalysisProgressWidget | 分析进度组件 |
| AnalysisResultWidget | 分析结果组件 |
| ChapterSelectDialog | 章节选择对话框 |
| ConfirmDialog | 确认对话框 |
| EditorCardBase | 编辑器卡片基类 |
| EditorStyles | 编辑器样式 |
| ImageViewerDialog | 图片查看对话框 |
| PanelEditorWidget | 面板编辑器组件 |
| SuccessDialog | 成功对话框 |
| FortuneCookieWidget | 签语饼组件 |
| SidebarWidget | 侧边栏组件 |

#### 2.8 ViewModel 层 (100% 完成)

| ViewModel | 功能描述 |
|-----------|----------|
| BaseViewModel | ViewModel 基类 |
| NovelViewModel | 小说视图模型 |
| StoryboardViewModel | 分镜视图模型 |

#### 2.9 其他模块 (100% 完成)

| 模块 | 功能描述 |
|------|----------|
| AppInitializer | 应用初始化器 |
| TransactionScope | 事务作用域 |

---

## 三、实现进度统计

### 总体进度

| 分类 | 已完成 | 完成率 |
|------|--------|--------|
| 数据模型 | 9 | 100% |
| 工具类 | 16 | 100% |
| 数据访问层 | 3 | 100% |
| 服务层 | 16 | 100% |
| API 客户端 | 12 | 100% |
| UI 页面 | 7 | 100% |
| UI 组件 | 17 | 100% |
| ViewModel | 3 | 100% |
| 其他模块 | 2 | 100% |

### 功能对比图

```
原仓库功能                    C++ 版本状态
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
前端 UI 层                    ████████████████████ 100%
数据模型                      ████████████████████ 100%
工具类                        ████████████████████ 100%
API 客户端                    ████████████████████ 100%
服务层                        ████████████████████ 100%
数据持久化                    ████████████████████ 100%
ViewModel                     ████████████████████ 100%
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
总体进度                      ████████████████████ 100%
```

---

## 四、项目文件结构

```
comic/
├── QingningComic.pro           # qmake 项目配置
├── PROJECT_STATUS.md            # 本文档 - 项目进度记录
├── DATABASE_DESIGN.md           # 数据库设计文档
├── README.md                    # 项目说明文档
├── config.ini.example           # 配置文件模板（提交到 Git）
├── config.ini                   # 配置文件（不提交，包含敏感信息）
├── include/                     # 头文件目录
│   ├── api/                    # API 客户端
│   │   ├── QwenJsonRepair.h
│   │   ├── QwenPromptBuilder.h
│   │   ├── QwenStoryboardMerger.h
│   │   ├── QwenStreamHandler.h
│   │   ├── VolcEngineRequest.h
│   │   ├── VolcEngineResponse.h
│   │   └── VolcEngineSignature.h
│   ├── components/             # UI 组件
│   │   ├── AnalysisProgressWidget.h
│   │   ├── AnalysisResultWidget.h
│   │   ├── BibleItem.h
│   │   ├── ChapterCard.h
│   │   ├── ChapterSelectDialog.h
│   │   ├── ChapterSpinBox.h
│   │   ├── ConfirmDialog.h
│   │   ├── EditorCardBase.h
│   │   ├── EditorStyles.h
│   │   ├── ImageViewerDialog.h
│   │   ├── ModeComboBox.h
│   │   ├── PanelCard.h
│   │   ├── PanelEditorWidget.h
│   │   ├── StoryboardItem.h
│   │   └── SuccessDialog.h
│   ├── services/               # 服务层
│   │   ├── BaseService.h
│   │   └── ExportService.h
│   ├── utils/                  # 工具类
│   │   ├── AsyncImageLoader.h
│   │   ├── JsonUtils.h
│   │   ├── ShotTypeHelper.h
│   │   ├── SingletonUtils.h
│   │   └── StatusHelper.h
│   ├── viewmodels/             # ViewModel
│   │   ├── BaseViewModel.h
│   │   ├── NovelViewModel.h
│   │   └── StoryboardViewModel.h
│   ├── Novel.h                 # 小说模型 ✅
│   ├── Character.h             # 角色模型 ✅
│   ├── Storyboard.h            # 分镜模型 ✅
│   ├── Panel.h                 # 面板模型 ✅
│   ├── Job.h                   # 任务模型 ✅
│   ├── Bible.h                 # 圣经模型 ✅
│   ├── Task.h                  # 任务模型 ✅
│   ├── Scene.h                 # 场景模型 ✅
│   ├── ChangeRequest.h         # 改稿请求模型 ✅
│   ├── FileManager.h           # 文件管理器 ✅
│   ├── StyleManager.h          # 样式管理器 ✅
│   ├── Logger.h                # 日志系统 ✅
│   ├── EncodingUtils.h         # 编码工具 ✅
│   ├── AppConfig.h             # 配置管理 ✅
│   ├── DatabaseManager.h       # 数据库管理 ✅
│   ├── FileStorage.h           # 文件存储 ✅
│   ├── SqlBuilder.h            # SQL 构建器 ✅
│   ├── QwenClient.h            # Qwen API 客户端 ✅
│   ├── QwenImageClient.h       # 通义万相客户端 ✅
│   ├── VolcEngineImageClient.h # 火山引擎客户端 ✅
│   ├── StorageClient.h         # S3 存储客户端 ✅
│   ├── SharedNetworkManager.h  # 共享网络管理器 ✅
│   ├── NovelService.h          # 小说服务 ✅
│   ├── StoryboardService.h     # 分镜服务 ✅
│   ├── AnalysisService.h       # 分析服务 ✅
│   ├── BibleGenerator.h        # 圣经生成服务 ✅
│   ├── BibleImageService.h     # 圣经图片服务 ✅
│   ├── CharacterExtractor.h    # 角色提取服务 ✅
│   ├── SceneExtractor.h        # 场景提取服务 ✅
│   ├── ImageService.h          # 图片服务 ✅
│   ├── BatchImageProcessor.h   # 批量图片处理器 ✅
│   ├── AnalysisStatusManager.h # 分析状态管理 ✅
│   ├── ServiceContainer.h      # 服务容器 ✅
│   ├── TaskQueue.h             # 任务队列 ✅
│   ├── TaskRegistry.h          # 任务注册表 ✅
│   ├── ChangeRequestService.h  # 改稿服务 ✅
│   ├── PromptBuilder.h         # Prompt 构建器 ✅
│   ├── RetryPolicy.h           # 重试策略 ✅
│   ├── SchemaToPrompt.h        # Schema 转 Prompt ✅
│   ├── JsonRepairAdapter.h     # JSON 修复适配器 ✅
│   ├── BibleCache.h            # 圣经缓存 ✅
│   ├── UserSession.h           # 用户会话 ✅
│   ├── TransactionScope.h      # 事务作用域 ✅
│   ├── AppInitializer.h        # 应用初始化器 ✅
│   ├── MainWindow.h            # 主窗口 ✅
│   ├── DashboardPage.h         # 总览页面 ✅
│   ├── NovelPage.h             # 项目空间 ✅
│   ├── NovelUploadPage.h       # 上传作品 ✅
│   ├── NovelDetailPage.h       # 作品详情 ✅
│   ├── CharacterDetailPage.h   # 角色详情 ✅
│   ├── ExportPage.h            # 导出中心 ✅
│   ├── SidebarWidget.h         # 侧边栏 ✅
│   └── FortuneCookieWidget.h   # 签语饼 ✅
├── src/                         # 源文件目录
│   ├── api/                    # API 客户端实现
│   ├── components/             # UI 组件实现
│   ├── data/                   # 数据层实现
│   ├── models/                 # 模型实现
│   ├── pages/                  # 页面实现
│   ├── services/               # 服务层实现
│   ├── utils/                  # 工具类实现
│   ├── viewmodels/             # ViewModel 实现
│   ├── widgets/                # 小部件实现
│   └── app/                    # 应用入口
│       ├── main.cpp            # 主程序入口 ✅
│       ├── MainWindow.cpp      # 主窗口实现 ✅
│       └── AppInitializer.cpp  # 应用初始化实现 ✅
└── build/                       # 构建输出目录
```

---

## 五、开发规范

### 代码规范
1. 使用 C++11 标准
2. 使用 Qt 5.15.2 框架
3. 关键地方添加中文注释，说明代码作用
4. 代码不能嵌套，保持结构清晰、层级分明
5. 采用模块化设计，提高代码复用性和可维护性

### 提交规范
- 每次提交一个功能模块
- 提交信息清晰描述变更内容
- 确保代码可以正常编译后再提交

---

## 六、快速开始

### 编译项目
1. 使用 Qt Creator 打开 `QingningComic.pro`
2. 选择 "Desktop Qt 5.15.2 MinGW 64-bit" 套件
3. 按 `Ctrl+B` 编译项目
4. 按 `Ctrl+R` 运行项目

### 查看进度
- 查看本文档了解已完成和待完成的工作
- 查看 `README.md` 了解项目总体介绍

---

*最后更新时间：2026-04-08*

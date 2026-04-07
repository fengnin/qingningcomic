# 青柠漫画项目 - 模块分析文档

> **文档版本**: 1.0  
> **创建日期**: 2026-02-26  
> **项目**: 青柠漫画 (C++ 版本)

---

## 目录

1. [原仓库完整模块清单](#1-原仓库完整模块清单)
2. [原仓库模块树状图](#2-原仓库模块树状图)
3. [C++ 实现版本完成情况对照表](#3-c-实现版本完成情况对照表)
4. [已实现模块功能对比分析](#4-已实现模块功能对比分析)
5. [未实现模块优先级建议](#5-未实现模块优先级建议)
6. [总结与建议](#6-总结与建议)

---

## 1. 原仓库完整模块清单

### 1.1 前端模块 (Frontend)

| 模块名称 | 核心功能 | 技术实现 | 职责说明 |
|---------|---------|---------|---------|
| **认证模块** | 用户登录/登出 | React Context + Cognito OIDC | 处理用户认证状态管理，PKCE 流程 |
| **作品管理模块** | 小说列表/上传/删除 | React + TypeScript | 管理用户的小说作品 |
| **角色管理模块** | 角色圣经管理 | React + TypeScript | 角色配置、参考图上传、标准像生成 |
| **分镜编辑模块** | 分镜查看/编辑 | React + TypeScript | 分镜面板的查看和编辑 |
| **漫画画布模块** | 图像编辑/对白编辑 | Canvas/Konva.js | 漫画的最终编辑和导出 |
| **任务监控模块** | Job 进度跟踪 | React Hooks + Polling | 实时显示 AI 生成任务进度 |
| **导出模块** | PDF/Webtoon/资源包导出 | React + TypeScript | 成品漫画的多种格式导出 |
| **API 客户端** | 自动生成的 API 调用 | OpenAPI Generator | 类型安全的后端 API 调用 |

### 1.2 后端模块 (Backend)

#### 1.2.1 API 网关层

| 模块名称 | 核心功能 | 技术实现 | 职责说明 |
|---------|---------|---------|---------|
| **API Gateway** | 请求路由/认证 | AWS API Gateway | Edge-Optimized + CloudFront CDN |
| **Cognito Authorizer** | JWT 验证 | AWS Cognito | 用户身份验证和授权 |

#### 1.2.2 Lambda 函数层

| 模块名称 | 核心功能 | 技术实现 | 职责说明 |
|---------|---------|---------|---------|
| **NovelsFunction** | 小说 CRUD | Node.js + DynamoDB | 小说的创建、查询、更新、删除 |
| **AnalyzeNovelFunction** | 小说分析 + 分镜生成 | Node.js + Qwen API | 调用 Qwen 分析小说文本，生成分镜和角色圣经 |
| **AnalyzeWorkerFunction** | 分析任务 Worker | Node.js + DynamoDB Streams | 异步处理分析任务 |
| **CharactersFunction** | 角色 CRUD | Node.js + DynamoDB | 角色圣经的管理 |
| **GeneratePortraitFunction** | 角色标准像生成 | Node.js + Imagen API | 为角色生成多视角标准像 |
| **StoryboardsFunction** | 分镜 CRUD | Node.js + DynamoDB | 分镜数据的管理 |
| **PanelsFunction** | 面板 CRUD | Node.js + DynamoDB | 分镜面板的管理 |
| **GeneratePanelsFunction** | 面板图像生成调度 | Node.js + DynamoDB | 创建面板生成任务，调度 Worker |
| **PanelWorkerFunction** | 面板图像生成 Worker | Node.js + Imagen API + DynamoDB Streams | 调用 Imagen 生成单个面板图像 |
| **ReferenceWorkerFunction** | 参考图处理 Worker | Node.js + Imagen API | 处理角色参考图，生成标准像 |
| **EditPanelFunction** | 面板编辑 | Node.js + Imagen API | 支持局部重绘、背景替换等编辑功能 |
| **ChangeRequestFunction** | 修改请求处理 | Node.js + Qwen/Imagen API | 自然语言指令解析为 CR-DSL 并执行 |
| **ChangeRequestWorker** | 修改请求 Worker | Node.js + EventBridge | 异步处理修改请求 |
| **JobsFunction** | 任务查询 | Node.js + DynamoDB | 查询 Job 状态和进度 |
| **ExportFunction** | 导出功能 | Node.js + S3 + PDFKit | 打包导出成品漫画 |
| **ExportWorkerFunction** | 导出 Worker | Node.js + DynamoDB Streams | 异步处理导出任务 |
| **RetryHandlerFunction** | 失败重试处理 | Node.js + EventBridge | 处理失败任务的延迟重试 |

#### 1.2.3 AI 适配器层

| 模块名称 | 核心功能 | 技术实现 | 职责说明 |
|---------|---------|---------|---------|
| **QwenAdapter** | Qwen API 封装 | Node.js + OpenAI SDK | 文本分析、分镜生成、JSON 修正、对白重写 |
| **ImagenAdapter** | Imagen 3 API 封装 | Node.js + Google Cloud SDK | 图像生成、图像编辑、参考图上传 |
| **PromptBuilder** | Prompt 构建 | Node.js | 构建 AI 所需的提示词 |
| **BibleManager** | 圣经管理 | Node.js | 角色圣经和场景圣经的提取和管理 |

#### 1.2.4 工具层

| 模块名称 | 核心功能 | 职责说明 |
|---------|---------|---------|
| **S3Utils** | S3 操作 | 文件上传下载、预签名 URL 生成 |
| **S3ImageUtils** | S3 图像操作 | 图像相关的 S3 操作 |
| **Response** | HTTP 响应 | 统一的响应格式、CORS 头 |
| **Auth** | 认证工具 | 用户信息提取、多租户隔离 |
| **AISecrets** | AI 密钥管理 | 从 Secrets Manager 获取 API 密钥 |

### 1.3 数据存储模块

| 模块名称 | 核心功能 | 技术实现 | 职责说明 |
|---------|---------|---------|---------|
| **DynamoDB 单表** | 结构化数据存储 | DynamoDB Single Table Design | 存储小说、角色、分镜、面板、任务等 |
| **S3 存储** | 文件存储 | AWS S3 + CloudFront | 存储小说原文、参考图、生成的图像、导出文件 |

### 1.4 基础设施模块

| 模块名称 | 核心功能 | 技术实现 | 职责说明 |
|---------|---------|---------|---------|
| **CI/CD 流水线** | 自动化部署 | GitHub Actions | 自动部署后端和前端 |
| **CloudWatch** | 监控和告警 | AWS CloudWatch | Lambda、API Gateway、DynamoDB 指标监控 |
| **EventBridge** | 事件调度 | AWS EventBridge | 延迟任务调度、重试机制 |

---

## 2. 原仓库模块树状图

```
青柠漫画 (qnyproj)
├── 前端 (Frontend)
│   ├── 认证模块
│   │   ├── AuthContext
│   │   └── Cognito 集成
│   ├── 作品管理模块
│   │   ├── NovelsIndexPage
│   │   ├── NovelDetailPage
│   │   └── NovelUploadPage
│   ├── 角色管理模块
│   │   ├── CharacterDetailPage
│   │   ├── ReferenceImageUploader
│   │   └── PortraitGallery
│   ├── 分镜编辑模块
│   │   ├── StoryboardPanel
│   │   └── PanelGrid
│   ├── 漫画画布模块
│   │   ├── Canvas (Konva.js)
│   │   ├── Toolbar
│   │   └── PropertyPanel
│   ├── 任务监控模块
│   │   └── useJobMonitor Hook
│   ├── 导出模块
│   │   └── ExportsPage
│   └── API 客户端
│       └── OpenAPI 生成客户端
│
├── 后端 (Backend)
│   ├── API 网关
│   │   └── API Gateway + Cognito Authorizer
│   ├── Lambda 函数
│   │   ├── 小说管理
│   │   │   └── NovelsFunction
│   │   ├── 分析模块
│   │   │   ├── AnalyzeNovelFunction
│   │   │   └── AnalyzeWorkerFunction
│   │   ├── 角色管理
│   │   │   ├── CharactersFunction
│   │   │   └── GeneratePortraitFunction
│   │   ├── 分镜管理
│   │   │   └── StoryboardsFunction
│   │   ├── 面板管理
│   │   │   ├── PanelsFunction
│   │   │   ├── GeneratePanelsFunction
│   │   │   └── PanelWorkerFunction
│   │   ├── 编辑模块
│   │   │   ├── EditPanelFunction
│   │   │   ├── ChangeRequestFunction
│   │   │   └── ChangeRequestWorker
│   │   ├── 参考图处理
│   │   │   └── ReferenceWorkerFunction
│   │   ├── 任务管理
│   │   │   └── JobsFunction
│   │   ├── 导出模块
│   │   │   ├── ExportFunction
│   │   │   └── ExportWorkerFunction
│   │   └── 重试处理
│   │       └── RetryHandlerFunction
│   ├── AI 适配器
│   │   ├── QwenAdapter
│   │   ├── ImagenAdapter
│   │   ├── PromptBuilder
│   │   └── BibleManager
│   └── 工具库
│       ├── S3Utils
│       ├── S3ImageUtils
│       ├── Response
│       ├── Auth
│       └── AISecrets
│
├── 数据存储
│   ├── DynamoDB 单表
│   │   ├── NOVEL 项
│   │   ├── CHARACTER 项
│   │   ├── STORYBOARD 项
│   │   ├── PANEL 项
│   │   ├── JOB 项
│   │   └── CHANGE_REQUEST 项
│   └── S3 存储
│       ├── novels/ (小说原文)
│       ├── characters/ (角色参考图/标准像)
│       ├── panels/ (生成的面板图像)
│       ├── edits/ (编辑版本)
│       └── exports/ (导出文件)
│
└── 基础设施
    ├── CI/CD (GitHub Actions)
    ├── CloudWatch (监控)
    └── EventBridge (事件调度)
```

---

## 3. C++ 实现版本完成情况对照表

### 3.1 数据模型层

| 原仓库模块 | C++ 实现文件 | 完成度 | 备注 |
|-----------|-------------|--------|------|
| **Novel (小说模型)** | `include/Novel.h` `src/models/Novel.cpp` | ✅ 100% | 完整实现，包含所有必要字段和 Q_GADGET 属性系统 |
| **Character (角色模型)** | `include/Character.h` `src/models/Character.cpp` | ✅ 100% | 完整实现，包含 CharacterAppearance 结构体 |
| **Storyboard (分镜模型)** | `include/Storyboard.h` `src/models/Storyboard.cpp` | ✅ 100% | 完整实现 |
| **Panel (面板模型)** | `include/Panel.h` `src/models/Panel.cpp` | ✅ 100% | 完整实现 |
| **Job (任务模型)** | `include/Job.h` `src/models/Job.cpp` | ✅ 100% | 完整实现 |

### 3.2 工具层

| 原仓库模块 | C++ 实现文件 | 完成度 | 备注 |
|-----------|-------------|--------|------|
| **JSON 序列化/反序列化** | `include/JsonHelper.h` `src/utils/JsonHelper.cpp` | ✅ 100% | 完整实现所有模型的 JSON 转换 |
| **文件管理** | `include/FileManager.h` `src/utils/FileManager.cpp` | ✅ 100% | 支持文本/二进制文件读写、目录操作 |
| **S3Utils (S3 工具)** | - | ❌ 0% | 未实现（需要 Qt Network + AWS SDK） |
| **AI 密钥管理** | - | ❌ 0% | 未实现 |

### 3.3 GUI 界面层

| 原仓库模块 | C++ 实现文件 | 完成度 | 备注 |
|-----------|-------------|--------|------|
| **主窗口/导航** | `include/MainWindow.h` `src/MainWindow.cpp` | ✅ 100% | 完整实现，支持多页面切换 |
| **小说管理界面** | `include/NovelPage.h` `src/NovelPage.cpp` | ⚠️ 70% | 列表和详情展示完成，CRUD 功能为占位符 |
| **角色管理界面** | `include/CharacterPage.h` `src/CharacterPage.cpp` | ⚠️ 70% | 列表和详情展示完成，CRUD 功能为占位符 |
| **分镜编辑界面** | - | ❌ 0% | 未实现 |
| **漫画画布界面** | - | ❌ 0% | 未实现 |
| **任务监控界面** | - | ❌ 0% | 未实现 |
| **导出界面** | - | ❌ 0% | 未实现 |

### 3.4 服务层

| 原仓库模块 | C++ 实现文件 | 完成度 | 备注 |
|-----------|-------------|--------|------|
| **NovelService (小说服务)** | - | ❌ 0% | 未实现（曾实现但已移除） |
| **CharacterService (角色服务)** | - | ❌ 0% | 未实现 |
| **StoryboardService (分镜服务)** | - | ❌ 0% | 未实现 |
| **PanelService (面板服务)** | - | ❌ 0% | 未实现 |
| **JobService (任务服务)** | - | ❌ 0% | 未实现 |

### 3.5 AI 集成层

| 原仓库模块 | C++ 实现文件 | 完成度 | 备注 |
|-----------|-------------|--------|------|
| **QwenAdapter (Qwen 客户端)** | - | ❌ 0% | 未实现 |
| **ImagenAdapter (Imagen 客户端)** | - | ❌ 0% | 未实现 |
| **PromptBuilder (Prompt 构建)** | - | ❌ 0% | 未实现 |
| **BibleManager (圣经管理)** | - | ❌ 0% | 未实现 |

### 3.6 项目配置

| 配置项 | 状态 | 备注 |
|-------|------|------|
| **Qt Widgets 模块** | ✅ 已启用 | GUI 支持已就绪 |
| **Qt Network 模块** | ✅ 已启用 | 网络请求支持已就绪 |
| **Qt Concurrent 模块** | ✅ 已启用 | 并发处理支持已就绪 |
| **Qt SQL 模块** | ✅ 已启用 | 数据库支持已就绪 |
| **UTF-8 编码配置** | ✅ 已修复 | 中文显示问题已解决 |

---

## 4. 已实现模块功能对比分析

### 4.1 数据模型层对比

#### Novel (小说模型)

| 功能点 | 原仓库 | C++ 实现 | 差异说明 |
|-------|--------|---------|---------|
| **基本字段** | id, title, userId, status, content, storyboardId, createdAt, updatedAt | id, title, author, description, userId, status, content, storyboardId, createdAt, updatedAt | C++ 版本增加了 author 和 description 字段，更适合本地桌面应用 |
| **状态枚举** | Uploaded, Analyzing, Analyzed, Generating, Completed, Error | Planning, Writing, Completed, Paused | C++ 版本简化了状态，更适合本地编辑场景 |
| **序列化** | DynamoDB 格式 | JSON 格式 | 存储方式不同，功能等价 |
| **属性系统** | TypeScript interface | Q_GADGET + Q_PROPERTY | 都支持元对象系统 |

**结论**：C++ 版本的数据模型在功能上等价，甚至有所增强，更适合桌面应用场景。

#### Character (角色模型)

| 功能点 | 原仓库 | C++ 实现 | 差异说明 |
|-------|--------|---------|---------|
| **基本字段** | id, novelId, name, role, appearance, personality | id, novelId, name, role, appearance, personality | ✅ 完全一致 |
| **Appearance 结构体** | gender, age, hairColor, hairStyle, eyeColor, height, build, clothing, distinctiveFeatures | gender, age, hairColor, hairStyle, eyeColor, height, build, clothing, distinctiveFeatures | ✅ 完全一致 |
| **多配置支持** | ✅ 支持 | ❌ 不支持 | 原仓库支持同一角色多个配置（日常/战斗等） |

**结论**：核心功能完整，多配置支持是后续可以增强的点。

### 4.2 工具层对比

#### JSON 序列化

| 功能点 | 原仓库 | C++ 实现 | 差异说明 |
|-------|--------|---------|---------|
| **Novel 序列化** | ✅ DynamoDB 格式 | ✅ JSON 格式 | 格式不同，功能等价 |
| **Character 序列化** | ✅ DynamoDB 格式 | ✅ JSON 格式 | 格式不同，功能等价 |
| **Storyboard 序列化** | ✅ DynamoDB 格式 | ✅ JSON 格式 | 格式不同，功能等价 |
| **Panel 序列化** | ✅ DynamoDB 格式 | ✅ JSON 格式 | 格式不同，功能等价 |
| **Job 序列化** | ✅ DynamoDB 格式 | ✅ JSON 格式 | 格式不同，功能等价 |
| **Schema 验证** | ✅ AJV/Zod | ❌ 无 | C++ 版本缺少运行时 Schema 验证 |

**结论**：JSON 序列化功能完整，Schema 验证可以后续用 Qt 的验证框架补充。

### 4.3 GUI 层对比

#### 主窗口

| 功能点 | 原仓库 | C++ 实现 | 差异说明 |
|-------|--------|---------|---------|
| **导航栏** | ✅ React Router | ✅ QStackedWidget + 按钮导航 | 实现方式不同，功能等价 |
| **多页面切换** | ✅ 支持 | ✅ 支持 | ✅ 完全一致 |
| **用户认证** | ✅ Cognito 集成 | ❌ 无 | 桌面版本暂不需要 |
| **响应式布局** | ✅ CSS Flexbox/Grid | ✅ Qt Layouts | 实现方式不同，功能等价 |

**结论**：主窗口功能完整，满足桌面应用需求。

#### 小说管理页面

| 功能点 | 原仓库 | C++ 实现 | 差异说明 |
|-------|--------|---------|---------|
| **小说列表展示** | ✅ 支持 | ✅ 支持 | ✅ 完全一致 |
| **小说详情展示** | ✅ 支持 | ✅ 支持 | ✅ 完全一致 |
| **上传小说** | ✅ 支持 | ❌ 占位符 | 需要实现文件选择和读取 |
| **删除小说** | ✅ 支持 | ❌ 占位符 | 需要实现文件删除 |
| **开始分析** | ✅ 支持 | ❌ 占位符 | 需要集成 Qwen API |

**结论**：展示功能完整，编辑功能需要补充。

---

## 5. 未实现模块优先级建议

### 5.1 优先级 P0 - 核心功能 (必须优先实现)

| 模块名称 | 预计工时 | 依赖项 | 优先级理由 |
|---------|---------|-------|-----------|
| **NovelService** | 2 小时 | 数据模型 | 核心业务服务，所有功能的基础 |
| **CharacterService** | 2 小时 | 数据模型 | 角色圣经管理是 AI 生成的基础 |
| **StoryboardService** | 2 小时 | 数据模型 | 分镜数据管理 |
| **PanelService** | 2 小时 | 数据模型 | 面板数据管理 |
| **JobService** | 2 小时 | 数据模型 | 任务状态跟踪 |
| **QwenClient** | 4-6 小时 | Qt Network | 文本分析和分镜生成的核心 |
| **小说页面 CRUD** | 2 小时 | NovelService | 完善小说管理功能 |
| **角色页面 CRUD** | 2 小时 | CharacterService | 完善角色管理功能 |

**P0 小计**: 18-20 小时

### 5.2 优先级 P1 - 重要功能 (建议尽快实现)

| 模块名称 | 预计工时 | 依赖项 | 优先级理由 |
|---------|---------|-------|-----------|
| **ImagenClient** | 4-6 小时 | Qt Network | 图像生成核心 |
| **分镜管理页面** | 4-6 小时 | StoryboardService, PanelService | 分镜查看和编辑 |
| **任务监控页面** | 3-4 小时 | JobService | 实时显示生成进度 |
| **PromptBuilder** | 3-4 小时 | QwenClient | 构建 AI 提示词 |
| **BibleManager** | 3-4 小时 | CharacterService | 角色圣经提取和管理 |

**P1 小计**: 17-24 小时

### 5.3 优先级 P2 - 增强功能 (可以延后实现)

| 模块名称 | 预计工时 | 依赖项 | 优先级理由 |
|---------|---------|-------|-----------|
| **面板编辑功能** | 6-8 小时 | ImagenClient | 局部重绘、背景替换等 |
| **修改请求 (CR-DSL)** | 8-10 小时 | QwenClient, ImagenClient | 自然语言编辑 |
| **导出功能** | 4-6 小时 | PanelService | PDF/Webtoon 导出 |
| **数据库集成** | 4-6 小时 | Qt SQL | 用 SQLite 替代文件存储 |
| **参考图上传** | 2-3 小时 | CharacterService | 角色参考图管理 |
| **标准像生成** | 3-4 小时 | ImagenClient | 角色多视角标准像 |

**P2 小计**: 27-37 小时

### 5.4 优先级 P3 - 优化功能 (可选实现)

| 模块名称 | 预计工时 | 依赖项 | 优先级理由 |
|---------|---------|-------|-----------|
| **角色多配置支持** | 3-4 小时 | CharacterService | 同一角色不同服装/状态 |
| **圣经智能压缩** | 4-6 小时 | QwenClient | 突破 Token 限制 |
| **多阶段分析** | 6-8 小时 | QwenClient | 提升分镜质量 |
| **单元测试** | 6-8 小时 | Qt Test | 保证代码质量 |
| **国际化 (i18n)** | 3-4 小时 | Qt Linguist | 多语言支持 |

**P3 小计**: 22-30 小时

---

## 6. 总结与建议

### 6.1 当前状态总结

**已完成**:
- ✅ 完整的 5 个核心数据模型
- ✅ JSON 序列化/反序列化工具
- ✅ 文件管理工具
- ✅ 主窗口 GUI 框架
- ✅ 小说管理页面（展示功能）
- ✅ 角色管理页面（展示功能）
- ✅ 项目配置和编码修复

**总体完成度**: 约 **35-40%** (核心数据模型和 GUI 框架完成)

### 6.2 实施路线图建议

#### 第一阶段（1-2 周）- 核心功能可用
1. 实现所有 Service 层（NovelService, CharacterService 等）
2. 完善 GUI 页面的 CRUD 功能
3. 实现 QwenClient，支持文本分析和分镜生成

**目标**: 可以上传小说、生成分镜、查看结果

#### 第二阶段（2-3 周）- AI 集成完成
1. 实现 ImagenClient，支持图像生成
2. 实现分镜管理页面
3. 实现任务监控页面
4. 实现 PromptBuilder 和 BibleManager

**目标**: 完整的 AI 生成流程可用

#### 第三阶段（2-4 周）- 功能完善
1. 实现面板编辑功能
2. 实现导出功能
3. 数据库集成（可选）
4. 参考图和标准像生成

**目标**: 产品级功能完整性

#### 第四阶段（2-3 周）- 优化增强
1. 角色多配置支持
2. 圣经智能压缩
3. 单元测试
4. 国际化

**目标**: 生产级质量和用户体验

### 6.3 技术建议

1. **保持非嵌套式架构**: 继续遵循当前的模块化设计，避免过度嵌套
2. **使用 Qt 的信号槽机制**: 实现模块间松耦合通信
3. **优先使用 JSON 文件存储**: 在实现数据库前，先用 JSON 文件持久化数据
4. **充分利用 Qt Concurrent**: 处理 AI 调用等耗时操作时使用多线程
5. **渐进式实现**: 按照优先级逐步实现，每个阶段都有可用的功能

### 6.4 风险提示

1. **AI API 调用**: Qwen 和 Imagen 的 API 集成需要仔细处理错误和重试
2. **异步处理**: AI 生成耗时较长，需要良好的异步处理和进度反馈
3. **数据一致性**: 本地文件存储需要注意并发访问和数据一致性问题
4. **测试覆盖**: GUI 和 AI 集成部分的测试需要特别注意

---

**文档结束**

*最后更新: 2026-02-26*

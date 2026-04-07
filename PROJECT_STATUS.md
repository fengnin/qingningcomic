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

#### 2.2 工具类层 (100% 完成)

| 文件 | 功能描述 | 对应原仓库 |
|------|----------|------------|
| [FileManager.h](include/FileManager.h) | 文件读写、目录操作 | s3-utils (本地版) |
| [JsonHelper.h](include/JsonHelper.h) | JSON 序列化/反序列化 | 原仓库内置 JSON |
| [StyleManager.h](include/StyleManager.h) | UI 样式管理 | 前端 CSS |
| [Logger.h](include/Logger.h) | 日志系统 | - |
| [EncodingUtils.h](include/EncodingUtils.h) | 编码工具，解决中文乱码 | - |
| [AppConfig.h](include/AppConfig.h) | 应用配置管理 | Secrets Manager |

#### 2.3 数据访问层 (100% 完成)

| 文件 | 功能描述 | 状态 |
|------|----------|------|
| [DatabaseManager.h](include/DatabaseManager.h) | 数据库连接、CRUD 操作封装 | ✅ 已实现 |

#### 2.4 服务层 (43% 完成)

| 服务 | 功能描述 | 对应原仓库 | 状态 |
|------|----------|------------|------|
| NovelService | 小说 CRUD 服务 | novels 函数 | ✅ 已实现 |
| StoryboardService | 分镜 CRUD 服务，保存/读取分镜数据 | storyboards 函数 | ✅ 已实现 |
| AnalysisService | 分析服务，调用 Qwen 生成分镜 | analyze-worker | ✅ 已实现 |
| CharacterService | 角色 CRUD 服务 | characters 函数 | ❌ 待实现 |
| PanelService | 面板 CRUD 服务 | panels 函数 | ❌ 待实现 |
| JobService | 任务管理服务 | jobs 函数 | ❌ 待实现 |
| BibleService | 圣经管理服务 | bible 函数 | ❌ 待实现 |
| ExportService | 导出服务 | export 函数 | ❌ 待实现 |

#### 2.5 API 客户端层 (100% 完成)

| 客户端 | 功能描述 | 对应原仓库 | 状态 |
|--------|----------|------------|------|
| QwenClient | Qwen API 客户端，文本分析、分镜生成 | qwen-adapter.js | ✅ 已实现 |
| ImagenClient | Gemini Images API 客户端，图片生成、编辑 | imagen-adapter.js | ✅ 已实现 |
| StorageClient | S3 兼容存储客户端，文件上传/下载 | s3-utils.js | ✅ 已实现 |

#### 2.6 UI 页面层 (100% 完成)

| 页面 | 功能描述 | 对应原仓库前端 |
|------|----------|----------------|
| [MainWindow](include/MainWindow.h) | 主窗口，侧边栏导航 | DashboardLayout.tsx |
| [DashboardPage](include/DashboardPage.h) | 总览页面，任务统计 | DashboardPage.tsx |
| [NovelPage](include/NovelPage.h) | 项目空间，作品列表 | NovelsIndexPage.tsx |
| [NovelUploadPage](include/NovelUploadPage.h) | 上传作品表单 | NovelUploadPage.tsx |
| [NovelDetailPage](include/NovelDetailPage.h) | 作品详情，章节/分镜/圣经管理 | NovelDetailPage.tsx |
| [CharacterPage](include/CharacterPage.h) | 角色管理页面 | CharacterDetailPage.tsx |
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

---

### ❌ 未实现的功能

#### 3.1 服务层 (5/8 待实现)

| 服务 | 功能描述 | 对应原仓库 | 优先级 |
|------|----------|------------|--------|
| ~~NovelService~~ | ~~小说 CRUD 服务~~ | ~~novels 函数~~ | ~~高~~ ✅ 已实现 |
| ~~StoryboardService~~ | ~~分镜 CRUD 服务~~ | ~~storyboards 函数~~ | ~~中~~ ✅ 已实现 |
| ~~AnalysisService~~ | ~~分析服务，调用 Qwen 生成分镜~~ | ~~analyze-worker~~ | ~~高~~ ✅ 已实现 |
| CharacterService | 角色 CRUD 服务 | characters 函数 | 高 |
| PanelService | 面板 CRUD 服务 | panels 函数 | 中 |
| JobService | 任务管理服务 | jobs 函数 | 中 |
| BibleService | 圣经管理服务 | bible 函数 | 中 |
| ExportService | 导出服务 | export 函数 | 低 |

#### 3.2 API 客户端 (33% 完成)

| 客户端 | 功能描述 | 对应原仓库 | 优先级 |
|--------|----------|------------|--------|
| ~~QwenClient~~ | ~~Qwen API 客户端，文本分析、分镜生成~~ | ~~qwen-adapter.js~~ | ~~高~~ ✅ 已实现 |
| ImagenClient | Imagen API 客户端，图片生成、编辑 | imagen-adapter.js | 高 |
| ApiClient | 通用 HTTP 客户端 | - | 高 |

#### 3.3 核心业务功能 (0% 完成)

| 功能 | 功能描述 | 对应原仓库 | 优先级 |
|------|----------|------------|--------|
| 小说分析 | 调用 Qwen API 分析小说生成分镜 | analyze-novel + analyze-worker | 高 |
| 面板生成 | 调用 Imagen API 生成面板图片 | generate-panels + panel-worker | 高 |
| 角色肖像生成 | 生成四视角角色肖像 | generate-portrait | 中 |
| 面板编辑 | inpaint/bg_swap/outpaint | edit-panel | 中 |
| 改稿功能 | 自然语言改稿 | change-request + change-request-worker | 低 |
| 导出功能 | PDF/Webtoon/Resources 导出 | export + export-worker | 低 |

#### 3.4 数据持久化 (0% 完成)

| 功能 | 功能描述 | 对应原仓库 | 优先级 |
|------|----------|------------|--------|
| 数据库管理 | Docker MySQL 数据库 CRUD | DynamoDB | 中 |
| 文件存储 | 本地文件存储（可扩展滨纷云） | S3 | 中 |
| 配置管理 | 应用配置持久化 | AWS Secrets Manager | 低 |

---

## 四、实现进度统计

### 总体进度

| 分类 | 已完成 | 未实现 | 完成率 |
|------|--------|--------|--------|
| 数据模型 | 5 | 0 | 100% |
| 工具类 | 6 | 0 | 100% |
| UI 页面 | 8 | 0 | 100% |
| 自定义组件 | 6 | 0 | 100% |
| **服务层** | 3 | 5 | **43%** |
| **API 客户端** | 3 | 0 | **100%** |
| **核心业务** | 1 | 5 | **17%** |
| **数据持久化** | 2 | 1 | **67%** |

### 功能对比图

```
原仓库功能                    C++ 版本状态
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
前端 UI 层                    ████████████████████ 100%
数据模型                      ████████████████████ 100%
工具类                        ████████████████████ 100%
API 客户端                    ████████████████████ 100%
数据持久化                    █████████████░░░░░░░  67%
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
服务层                        ████████░░░░░░░░░░░░  43%
核心业务功能                  ███░░░░░░░░░░░░░░░░░  17%
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
总体进度                      ███████████████░░░░░  75%
```

---

## 五、接下来需要完成的工作

### 优先级 1 - 高优先级（核心功能）

#### 任务 1.1：QwenClient（阿里云 Qwen API 客户端）✅ 已完成
- **优先级**：高
- **对应原仓库**：lib/qwen-adapter.js
- **实施步骤**：
  1. ✅ 创建 `include/QwenClient.h` 头文件
  2. ✅ 创建 `src/api/QwenClient.cpp` 实现文件
  3. ✅ 实现 HTTP 请求功能（使用 Qt Network）
  4. ✅ 实现文本分析功能
  5. ✅ 实现分镜生成功能
  6. ✅ 添加 JSON 严格模式支持
- **已实现功能**：
  - `splitTextIntelligently()` - 智能文本分块（与原仓库完全一致）
  - `mergeStoryboards()` - 分镜合并（PANELS_PER_PAGE = 6）
  - `generateStoryboard()` - 分镜生成主流程
  - `callQwen()` - API 调用（含指数退避重试）
  - JSON 修复和解析工具
- **所需资源**：
  - Qt Network 模块 ✅
  - 阿里云 Qwen API Key（用于测试）
- **验收标准**：
  - ✅ 可以发送 HTTP 请求到 Qwen API
  - ✅ 可以解析 JSON 响应
  - ✅ 可以处理速率限制和重试
  - ✅ 代码有清晰的中文注释

#### 任务 1.2：ImagenClient（Google Gemini Images API 客户端）✅ 已完成
- **优先级**：高
- **对应原仓库**：lib/imagen-adapter.js
- **实施步骤**：
  1. ✅ 创建 `include/ImagenClient.h` 头文件
  2. ✅ 创建 `src/api/ImagenClient.cpp` 实现文件
  3. ✅ 实现图像生成功能（generate）
  4. ✅ 实现图像编辑功能（edit）
  5. ✅ 支持预览和高清两种模式
  6. ✅ 支持参考图输入
  7. ✅ 实现占位图生成（测试用）
- **已实现功能**：
  - `generate()` - 图像生成（支持参考图、宽高比、模式设置）
  - `edit()` - 图像编辑（支持语义掩码编辑）
  - `generateAsync()` / `editAsync()` - 异步生成/编辑
  - `generatePlaceholder()` - 占位图生成（API 不可用时）
  - 指数退避重试机制
  - 完整的错误处理和日志记录
- **所需资源**：
  - Qt Network 模块 ✅
  - Gemini API Key（用于测试）
- **验收标准**：
  - ✅ 可以调用 Gemini API 生成图像
  - ✅ 支持参考图输入（最多3张）
  - ✅ 可以获取生成的图像数据
  - ✅ 代码有清晰的中文注释

#### 任务 1.3：服务层实现
- **优先级**：高
- **对应原仓库**：functions/ 目录下的 Lambda 函数
- **实施步骤**：
  1. 创建 `src/services/` 目录
  2. 实现 NovelService（小说管理）
  3. 实现 CharacterService（角色管理）
  4. 实现 StoryboardService（分镜管理）
  5. 实现 PanelService（面板管理）
  6. 实现 JobService（任务管理）

### 优先级 2 - 中优先级（数据持久化）

#### 任务 2.1：数据库集成
- **优先级**：中
- **对应原仓库**：DynamoDB
- **实施步骤**：
  1. 在 CentOS 虚拟机中安装 MySQL 8.0
  2. 配置 VMware 端口转发
  3. 创建数据库表结构（10 张表）
  4. 实现数据库 CRUD 操作
- **所需资源**：
  - Qt SQL 模块
  - CentOS 虚拟机 + VMware Workstation
  - MySQL 8.0
- **CentOS 安装 MySQL 命令**：
  ```bash
  # 安装 MySQL
  sudo yum install -y https://dev.mysql.com/get/mysql80-community-release-el7-7.noarch.rpm
  sudo yum install -y mysql-community-server
  sudo systemctl start mysqld
  sudo systemctl enable mysqld
  
  # 配置数据库
  mysql -u root -p
  ALTER USER 'root'@'localhost' IDENTIFIED BY 'Qingning123!';
  CREATE DATABASE qingning_comic CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
  CREATE USER 'qingning'@'%' IDENTIFIED BY 'qingning123';
  GRANT ALL PRIVILEGES ON qingning_comic.* TO 'qingning'@'%';
  FLUSH PRIVILEGES;
  EXIT;
  
  # 允许远程连接
  sudo sed -i '/^\[mysqld\]$/a bind-address = 0.0.0.0' /etc/my.cnf
  sudo systemctl restart mysqld
  ```
- **详细配置**：查看 [DATABASE_DESIGN.md](DATABASE_DESIGN.md)

#### 任务 2.2：文件存储管理
- **优先级**：中
- **对应原仓库**：S3 + s3-utils.js
- **实施步骤**：
  1. 当前使用本地文件存储（FileStorage）
  2. 后续可扩展支持滨纷云（S3 兼容）
  3. 实现图片上传/下载功能
  4. 实现图片缓存机制

### 优先级 3 - 低优先级（增强功能）

#### 任务 3.1：单元测试
- **优先级**：低
- **实施步骤**：
  1. 配置 Qt Test 框架
  2. 为每个模型编写单元测试
  3. 为每个服务编写单元测试
  4. 为工具类编写单元测试
- **验收标准**：
  - 测试覆盖率达到 80% 以上
  - 所有测试用例通过

---

## 六、项目文件结构

```
comic/
├── QingningComic.pro           # qmake 项目配置
├── PROJECT_STATUS.md            # 本文档 - 项目进度记录
├── DATABASE_DESIGN.md           # 数据库设计文档
├── README.md                    # 项目说明文档
├── config.ini.example           # 配置文件模板（提交到 Git）
├── config.ini                   # 配置文件（不提交，包含敏感信息）
├── include/                     # 头文件目录
│   ├── Novel.h                 # 小说模型 ✅
│   ├── Character.h             # 角色模型 ✅
│   ├── Storyboard.h            # 分镜模型 ✅
│   ├── Panel.h                 # 面板模型 ✅
│   ├── Job.h                   # 任务模型 ✅
│   ├── JsonHelper.h            # JSON 工具类 ✅
│   ├── FileManager.h           # 文件管理器 ✅
│   ├── StyleManager.h          # 样式管理器 ✅
│   ├── Logger.h                # 日志系统 ✅
│   ├── EncodingUtils.h         # 编码工具 ✅
│   ├── AppConfig.h             # 配置管理 ✅
│   ├── MainWindow.h            # 主窗口 ✅
│   ├── DashboardPage.h         # 总览页面 ✅
│   ├── NovelPage.h             # 项目空间 ✅
│   ├── NovelUploadPage.h       # 上传作品 ✅
│   ├── NovelDetailPage.h       # 作品详情 ✅
│   ├── CharacterPage.h         # 角色管理 ✅
│   ├── CharacterDetailPage.h   # 角色详情 ✅
│   ├── ExportPage.h            # 导出中心 ✅
│   ├── QwenClient.h            # Qwen API 客户端 ✅
│   ├── StoryboardService.h     # 分镜服务 ✅
│   ├── AnalysisService.h       # 分析服务 ✅
│   └── ImagenClient.h          # Imagen API 客户端 ❌ 待实现
├── src/                         # 源文件目录
│   ├── main.cpp                # 主程序入口 ✅
│   ├── models/                 # 模型实现 ✅
│   │   ├── Novel.cpp
│   │   ├── Character.cpp
│   │   ├── Storyboard.cpp
│   │   ├── Panel.cpp
│   │   └── Job.cpp
│   ├── utils/                  # 工具类实现 ✅
│   │   ├── FileManager.cpp
│   │   ├── JsonHelper.cpp
│   │   ├── StyleManager.cpp
│   │   ├── Logger.cpp
│   │   ├── EncodingUtils.cpp
│   │   └── AppConfig.cpp
│   ├── services/               # 服务层
│   │   ├── NovelService.cpp    # 小说服务 ✅
│   │   ├── StoryboardService.cpp # 分镜服务 ✅
│   │   └── AnalysisService.cpp # 分析服务 ✅
│   ├── api/                    # API 客户端
│   │   ├── QwenClient.cpp      # Qwen API 实现 ✅
│   │   └── ImagenClient.cpp    # Imagen API 实现 ❌ 待实现
│   ├── MainWindow.cpp          # 主窗口实现 ✅
│   ├── DashboardPage.cpp       # 总览页面实现 ✅
│   ├── NovelPage.cpp           # 项目空间实现 ✅
│   ├── NovelUploadPage.cpp     # 上传作品实现 ✅
│   ├── NovelDetailPage.cpp     # 作品详情实现 ✅
│   ├── CharacterPage.cpp       # 角色管理实现 ✅
│   ├── CharacterDetailPage.cpp # 角色详情实现 ✅
│   └── ExportPage.cpp          # 导出中心实现 ✅
└── build/                       # 构建输出目录
```

---

## 七、开发规范

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

## 八、快速开始

### 编译项目
1. 使用 Qt Creator 打开 `QingningComic.pro`
2. 选择 "Desktop Qt 5.15.2 MinGW 64-bit" 套件
3. 按 `Ctrl+B` 编译项目
4. 按 `Ctrl+R` 运行项目

### 查看进度
- 查看本文档了解已完成和待完成的工作
- 查看 `README.md` 了解项目总体介绍

---

*最后更新时间：2026-03-10*

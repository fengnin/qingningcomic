# AI Agent 开发指南 - 青柠漫画 C++ 版本

本文档为 AI Agent 提供完整的开发指导，用于将 `E:\QingningComic\qingning` 下的原始青柠漫画项目完整复刻为 C++11 + Qt 5.15.2 桌面版本。
当前仓库已完成核心功能与基础设施实现，后续工作以修正、维护、补充文档和同步面试材料为主。

---

## 目录

1. [项目概述](#1-项目概述)
2. [技术架构](#2-技术架构)
3. [开发环境配置](#3-开发环境配置)
4. [核心模块实现指南](#4-核心模块实现指南)
5. [API 客户端实现](#5-api-客户端实现)
6. [数据模型与存储](#6-数据模型与存储)
7. [AI 服务集成](#7-ai-服务集成)
8. [代码规范与最佳实践](#8-代码规范与最佳实践)
9. [任务追踪与进度管理](#9-任务追踪与进度管理)
10. [软件设计文档与文档规范](#10-软件设计文档与文档规范)

---

## 1. 项目概述

### 1.1 项目目标

本仓库是对 `E:\QingningComic\qingning` 下原始青柠漫画项目的 C++ 桌面复刻版。
目标不是重新设计一套新产品，而是尽量保持原项目的功能边界、数据结构、交互流程和文档表达一致。

- **实现方式**：C++11 + Qt 5.15.2 + Qt Widgets
- **项目定位**：桌面端完整复刻原项目
- **当前状态**：核心模块、UI、数据层、服务层和 API 客户端已完成

### 1.2 原项目功能清单

| 功能模块 | 原项目实现 | 当前状态 |
|---------|------------|----------|
| 小说管理 | `novels` / `analyze-novel` / `analyze-worker` | 已完成 |
| Bible 管理 | `bible` / `reference-worker` | 已完成 |
| 角色管理 | `characters` / `generate-portrait` | 已完成 |
| 分镜脚本 | `storyboards` / `panels` / `generate-panels` / `panel-worker` | 已完成 |
| 图像编辑 | `edit-panel` | 已完成 |
| 改稿模块 | `change-request` / `change-request-worker` | 已完成 |
| 导出模块 | `export` / `export-worker` | 已完成 |
| 任务管理 | `jobs` / `retry-handler` | 已完成 |

### 1.3 原项目工具链

| 工具类 | 功能描述 |
|--------|----------|
| `qwen-adapter` | Qwen API 适配器，长文本分块、JSON Schema 验证、自动重试 |
| `imagen-adapter` | Imagen / Gemini Images API 适配器，图片生成和编辑 |
| `bible-manager` | 角色/场景圣经管理器，版本化存储 |
| `prompt-builder` | Prompt 构建工具，视角/姿态/风格/表情映射表 |
| `s3-utils` | S3 工具类，上传、下载、预签名 URL |
| `s3-image-utils` | S3 图片工具类，S3 URI 转 Base64 |
| `ai-secrets` | AI 密钥管理 |
| `auth` | 认证工具类 |
| `response` | 统一响应格式工具 |
| `schema-to-prompt` | JSON Schema 转 Prompt 工具 |

### 1.4 C++ 版本实现状态

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
| **总体** | **所有核心模块** | **100%** |

### 1.5 复刻原则

1. 先对齐原项目功能，再对齐交互细节和命名习惯
2. 能保持一致的接口、状态、字段和数据结构尽量保持一致
3. 技术实现以可维护和可演示为优先，但不偏离原项目逻辑
4. 文档、注释和面试材料要同步反映真实实现

---

## 2. 技术架构

### 2.1 整体架构图

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           青柠漫画 C++ 桌面应用                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ UI 层 (Qt Widgets)                                                 │   │
│  │ MainWindow / DashboardPage / NovelPage / NovelDetailPage / ...     │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                      │                                       │
│                                      ▼                                       │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ ViewModel 层                                                        │   │
│  │ NovelViewModel / StoryboardViewModel / BaseViewModel                │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                      │                                       │
│                                      ▼                                       │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ 服务层                                                               │   │
│  │ NovelService / StoryboardService / AnalysisService / ExportService  │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                      │                                       │
│                     ┌────────────────┼────────────────┐                    │
│                     ▼                ▼                ▼                    │
│        ┌──────────────────┐  ┌─────────────────┐  ┌──────────────────┐    │
│        │ API 客户端层      │  │ 数据访问层      │  │ 本地文件与配置层 │    │
│        │ QwenClient        │  │ DatabaseManager │  │ FileManager      │    │
│        │ QwenImageClient   │  │ FileStorage     │  │ AppConfig        │    │
│        │ VolcEngineClient  │  │ SqlBuilder      │  │ Logger 等        │    │
│        └──────────────────┘  └─────────────────┘  └──────────────────┘    │
│                     │                │                │                    │
│                     └────────────────┼────────────────┘                    │
│                                      ▼                                       │
│                          外部服务与数据源                                     │
│                     Qwen / 通义万相 / 火山引擎 / MySQL 8.0                   │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 数据流向

```
用户操作
  → Qt UI
  → ViewModel
  → Service
  → API 客户端 / 数据访问层
  → 外部 AI 服务 / MySQL / 本地文件
  → 返回结果刷新 UI
```

### 2.3 模块依赖关系

```
MainWindow
    ├── NovelViewModel
    │       ├── NovelService
    │       └── BaseViewModel
    ├── StoryboardViewModel
    │       ├── StoryboardService
    │       └── BaseViewModel
    ├── AnalysisService
    │       ├── QwenClient
    │       └── PromptBuilder
    ├── ExportService
    │       ├── FileStorage
    │       └── DatabaseManager
    └── AppInitializer
            ├── ServiceContainer
            └── DatabaseManager / FileStorage / Config
```

---

## 3. 开发环境配置

### 3.1 必需工具

| 工具 | 版本 | 用途 |
|------|------|------|
| Qt | 5.15.2 (LTS) | GUI 框架 |
| Qt Creator | 4.14+ | IDE |
| MinGW | 64-bit | 编译器 (Windows) |
| CMake | 3.16+ | 构建系统 (可选) |
| Git | 2.30+ | 版本控制 |

### 3.2 Qt 模块依赖

```qmake
# QingningComic.pro
QT += core gui widgets network sql concurrent
```

### 3.3 项目结构

```
comic/
├── include/                    # 头文件
│   ├── models/                 # 数据模型
│   │   ├── Novel.h
│   │   ├── Character.h
│   │   ├── Storyboard.h
│   │   ├── Panel.h
│   │   └── Job.h
│   ├── services/               # 服务层
│   │   ├── NovelService.h
│   │   ├── CharacterService.h
│   │   ├── StoryboardService.h
│   │   └── PanelService.h
│   ├── api/                    # API 客户端
│   │   ├── ApiClient.h
│   │   ├── QwenAdapter.h
│   │   └── ImagenAdapter.h
│   ├── storage/                # 存储层
│   │   ├── DataStorage.h
│   │   └── FileManager.h
│   ├── utils/                  # 工具类
│   │   ├── TaskQueue.h
│   │   ├── JsonHelper.h
│   │   └── Settings.h
│   └── ui/                     # UI 组件
│       ├── MainWindow.h
│       ├── pages/              # 页面组件
│       └── widgets/            # 自定义控件
├── src/                        # 源文件
│   ├── models/
│   ├── services/
│   ├── api/
│   ├── storage/
│   ├── utils/
│   └── ui/
├── resources/                  # 资源文件
│   ├── icons/
│   ├── styles/
│   └── resources.qrc
├── tests/                      # 测试
├── docs/                       # 文档
├── QingningComic.pro           # qmake 配置
└── CMakeLists.txt              # CMake 配置 (可选)
```

---

## 4. 核心模块实现指南

### 4.1 数据模型 (Models)

所有数据模型应遵循以下规范：

```cpp
// include/models/Novel.h
#pragma once

#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QMetaType>

// 小说状态枚举
enum class NovelStatus {
    Created,      // 已创建
    Analyzing,    // 分析中
    Analyzed,     // 已分析
    Generating,   // 生成中
    Completed,    // 已完成
    Error         // 错误
};

// 小说数据模型
class Novel
{
    Q_GADGET
    Q_PROPERTY(QString id READ id WRITE setId)
    Q_PROPERTY(QString title READ title WRITE setTitle)
    Q_PROPERTY(NovelStatus status READ status WRITE setStatus)
    Q_PROPERTY(QString userId READ userId WRITE setUserId)
    Q_PROPERTY(QString storyboardId READ storyboardId WRITE setStoryboardId)
    Q_PROPERTY(qint64 createdAt READ createdAt WRITE setCreatedAt)
    Q_PROPERTY(qint64 updatedAt READ updatedAt WRITE setUpdatedAt)

public:
    Novel() = default;
    
    // JSON 序列化/反序列化
    static Novel fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
    // 属性访问器
    QString id() const { return m_id; }
    void setId(const QString& id) { m_id = id; }
    
    QString title() const { return m_title; }
    void setTitle(const QString& title) { m_title = title; }
    
    NovelStatus status() const { return m_status; }
    void setStatus(NovelStatus status) { m_status = status; }
    
    QString statusString() const;
    static NovelStatus statusFromString(const QString& str);
    
    // ... 其他属性访问器

private:
    QString m_id;
    QString m_title;
    QString m_originalText;
    QString m_originalTextS3;
    NovelStatus m_status = NovelStatus::Created;
    QString m_userId;
    QString m_storyboardId;
    QVariantMap m_metadata;
    qint64 m_createdAt = 0;
    qint64 m_updatedAt = 0;
};

// 注册为 Qt 元类型，支持信号槽传递
Q_DECLARE_METATYPE(Novel)
```

### 4.2 服务层 (Services)

服务层负责业务逻辑，使用信号槽进行异步通信：

```cpp
// include/services/NovelService.h
#pragma once

#include <QObject>
#include <QSharedPointer>
#include "models/Novel.h"
#include "api/ApiClient.h"
#include "storage/DataStorage.h"

class NovelService : public QObject
{
    Q_OBJECT

public:
    explicit NovelService(ApiClient* apiClient, DataStorage* storage, QObject* parent = nullptr);
    
    // 异步操作
    void fetchNovels();
    void fetchNovel(const QString& id);
    void createNovel(const QString& title, const QString& text);
    void deleteNovel(const QString& id);
    void analyzeNovel(const QString& id);
    
    // 同步操作（本地缓存）
    QList<Novel> getCachedNovels() const;
    Novel getCachedNovel(const QString& id) const;

signals:
    void novelsFetched(const QList<Novel>& novels);
    void novelFetched(const Novel& novel);
    void novelCreated(const Novel& novel);
    void novelDeleted(const QString& id);
    void analysisStarted(const QString& jobId);
    void error(const QString& message);

private slots:
    void onFetchNovelsFinished(const QJsonObject& response);
    void onFetchNovelFinished(const QJsonObject& response);
    void onCreateNovelFinished(const QJsonObject& response);
    void onApiError(const QString& error);

private:
    ApiClient* m_apiClient;
    DataStorage* m_storage;
    QList<Novel> m_cachedNovels;
};
```

### 4.3 API 客户端 (ApiClient)

```cpp
// include/api/ApiClient.h
#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>
#include <functional>

class ApiClient : public QObject
{
    Q_OBJECT

public:
    explicit ApiClient(QObject* parent = nullptr);
    
    // 配置
    void setBaseUrl(const QString& url);
    void setAuthToken(const QString& token);
    
    // HTTP 方法
    void get(const QString& endpoint);
    void post(const QString& endpoint, const QJsonObject& data);
    void put(const QString& endpoint, const QJsonObject& data);
    void del(const QString& endpoint);
    
```

## 10. 软件设计文档与文档规范

### 10.1 文档目标
- 项目实现前优先形成软件设计文档（SDD）
- SDD 需要描述架构、模块、接口、数据设计、错误处理、测试与部署
- 文档应当与代码同步更新，作为实现和面试展示的依据

### 10.2 必须覆盖的内容
- 文档概述：目的、范围、读者、术语、参考资料
- 系统架构：整体分层、技术选型、部署架构
- 模块设计：模块职责、类设计、核心流程、关键逻辑
- 接口设计：API 路径、请求参数、响应格式、错误码
- 数据设计：数据库表、索引、核心数据结构、数据流
- 安全设计：认证、授权、敏感数据处理、传输安全
- 性能设计：性能指标、瓶颈分析、优化策略
- 错误处理：错误码体系、异常处理、重试与降级
- 测试策略：单元测试、集成测试、接口测试、验收测试
- 部署方案：环境划分、部署流程、监控与回滚

### 10.3 编写标准
- 使用 Markdown 编写，标题层级清晰
- 技术说明以可实现为准，避免空泛口号
- 关键代码、复杂流程、协议和状态转换必须说明原因
- 能用图表达的内容优先用 Mermaid 图
- 接口示例、数据结构示例、错误码表尽量标准化
- 文档命名建议采用 `项目名_SDD_v版本号.md`

### 10.4 输出偏好
- 解释设计时优先说明“为什么这么选”
- 实现说明优先覆盖“怎么做”和“如何迭代优化”
- 涉及新功能时，要补充后续规划和风险点
- 如果内容适合面试展示，要同步整理进面试文档

### 10.5 项目约束
- 默认技术栈以 C++11 + Qt 5.15.2 为主
- 开发环境优先按 `Desktop_Qt_5_15_2_MinGW_64_bit`
- 数据库设计优先按 MySQL 8.0 思路考虑
- 不要主动帮忙编译，除非用户明确要求
- 调用外部 API 时，不公开密钥、私有参数和敏感实现细节

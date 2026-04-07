# AI Agent 开发指南 - 青柠漫画 C++ 版本

本文档为 AI Agent 提供完整的开发指导，用于使用 C++11 + Qt 5.15.2 完全复刻青柠漫画项目。

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

---

## 1. 项目概述

### 1.1 项目目标

将现有的 AWS Serverless 架构的青柠漫画项目，使用 C++11 + Qt 5.15.2 完全复刻，实现：

- **桌面应用**：Windows/Linux/macOS 桌面客户端
- **云端服务**：数据库、文件存储全部使用云服务
- **面试展示**：给面试官在线体验

### 1.2 核心功能清单

| 功能模块 | 原实现 | C++ 实现状态 |
|---------|--------|-------------|
| 小说管理 | Lambda + DynamoDB | 待实现 |
| 文本分析 | Qwen API | 待实现 |
| 角色圣经 | DynamoDB + S3 | 待实现 |
| 分镜生成 | Qwen API | 待实现 |
| 面板生成 | Imagen 3 API | 待实现 |
| 图像编辑 | Imagen edit API | 待实现 |
| 改稿功能 | CR-DSL | 待实现 |
| 导出功能 | Lambda | 待实现 |
| 用户认证 | Cognito | 待实现 |

### 1.3 最终技术栈选型 ✅

| 层级 | 原项目 (AWS) | C++ 版本 (最终确定) | 说明 |
|------|-------------|-------------------|------|
| **前端** | React 19 + TypeScript | **Qt Widgets** | 桌面应用 |
| **后端 API** | API Gateway + Lambda | **Cloudflare Workers** | 边缘计算，免费额度充足 |
| **数据库** | DynamoDB | **Supabase PostgreSQL** | 免费套餐，REST API 支持 |
| **文件存储** | S3 | **Supabase Storage / Cloudflare R2** | 免费套餐，S3 兼容 API |
| **用户认证** | Cognito | **Supabase Auth** | 免费套餐，支持 OAuth |
| **AI 服务** | Qwen + Imagen | **Qwen + Imagen (相同)** | HTTP API 调用 |

### 1.4 云服务免费额度

| 服务 | 免费额度 | 预估月成本 |
|------|----------|-----------|
| Cloudflare Workers | 100k 请求/天 | **$0** |
| Supabase (PostgreSQL) | 500MB 数据库 + 1GB 存储 | **$0** |
| Supabase Storage | 1GB 存储 + 2GB 带宽 | **$0** |
| Supabase Auth | 50k 月活用户 | **$0** |
| **总计** | - | **$0** |

### 1.5 架构优势

1. **零成本起步**：所有服务都有免费额度，足够演示使用
2. **全球加速**：Cloudflare CDN 边缘节点，访问速度快
3. **易于分享**：一个 URL 即可让面试官体验
4. **技术亮点**：C++ 桌面应用 + Serverless 架构

---

## 2. 技术架构

### 2.1 整体架构图

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           用户访问层                                          │
│  ┌───────────────────────────────────────────────────────────────────────┐  │
│  │  面试官 → 下载桌面应用 → 运行 QingNingComic.exe                        │  │
│  └───────────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                        前端层 (Qt Widgets)                                   │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    Qt Widgets 桌面应用                               │   │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐   │   │
│  │  │ MainWindow  │ │ NovelPage   │ │ CharacterPg │ │ ExportPage  │   │   │
│  │  └──────┬──────┘ └──────┬──────┘ └──────┬──────┘ └─────┬───────┘   │   │
│  └─────────┼───────────────┼───────────────┼───────────────┼───────────┘   │
└────────────┼───────────────┼───────────────┼───────────────┼───────────────┘
             │               │               │               │
             └───────────────┼───────────────┼───────────────┘
                             │               │
┌────────────────────────────▼───────────────▼───────────────────────────────┐
│                        服务层 (C++ Classes)                                  │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐          │
│  │NovelService │ │CharacterSvc │ │StoryboardSvc│ │ PanelService│          │
│  └──────┬──────┘ └──────┬──────┘ └──────┬──────┘ └──────┬──────┘          │
└─────────┼───────────────┼───────────────┼───────────────┼──────────────────┘
          │               │               │               │
┌─────────▼───────────────▼───────────────▼───────────────▼──────────────────┐
│                        API 客户端层                                          │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐                          │
│  │ ApiClient   │ │ QwenAdapter │ │ImagenAdapter│                          │
│  │ (HTTP 请求) │ │ (文本分析)  │ │ (图像生成)  │                          │
│  └──────┬──────┘ └──────┬──────┘ └──────┬──────┘                          │
└─────────┼───────────────┼───────────────┼──────────────────────────────────┘
          │               │               │
┌─────────▼───────────────▼───────────────▼──────────────────────────────────┐
│                        云服务层 (外部服务)                                    │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    Cloudflare Workers (API 网关)                     │   │
│  │                    - 请求转发、认证校验、速率限制                      │   │
│  └──────────────────────────────┬──────────────────────────────────────┘   │
│                                 │                                           │
│  ┌──────────────────────────────▼──────────────────────────────────────┐   │
│  │                      Supabase (BaaS 平台)                            │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                  │   │
│  │  │ PostgreSQL  │  │   Storage   │  │    Auth     │                  │   │
│  │  │  数据库     │  │  文件存储   │  │  用户认证   │                  │   │
│  │  │ novels      │  │ images/     │  │ OAuth/JWT   │                  │   │
│  │  │ characters  │  │ exports/    │  │             │                  │   │
│  │  │ panels      │  │ portraits/  │  │             │                  │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘                  │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                        AI 服务 (HTTP API)                            │   │
│  │  ┌─────────────────────┐  ┌─────────────────────┐                   │   │
│  │  │ Qwen API (阿里云)   │  │ Imagen 3 (GCP)      │                   │   │
│  │  │ - 文本分析          │  │ - 图像生成          │                   │   │
│  │  │ - 分镜生成          │  │ - 图像编辑          │                   │   │
│  │  │ - 改稿解析          │  │ - 肖像生成          │                   │   │
│  │  └─────────────────────┘  └─────────────────────┘                   │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 数据流向

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                              用户操作流程                                     │
│                                                                              │
│  1. 上传小说                                                                 │
│     用户 → Qt 桌面应用 → Cloudflare Workers → Supabase Storage (存储文本)    │
│                                    ↓                                         │
│                           Supabase DB (创建记录)                             │
│                                                                              │
│  2. 分析小说                                                                 │
│     用户点击分析 → Qt 桌面应用 → Cloudflare Workers → Qwen API (生成分镜)    │
│                                                    ↓                         │
│                                           Supabase DB (存储分镜数据)         │
│                                                                              │
│  3. 生成面板图像                                                             │
│     用户点击生成 → Qt 桌面应用 → Cloudflare Workers → Imagen API (生成图像)  │
│                                                    ↓                         │
│                                           Supabase Storage (存储图像)        │
│                                           Supabase DB (更新面板状态)         │
│                                                                              │
│  4. 导出漫画                                                                 │
│     用户点击导出 → Qt 桌面应用 → 本地处理 → 保存 PDF/Webtoon                  │
│                                                                              │
└──────────────────────────────────────────────────────────────────────────────┘
```

### 2.3 模块依赖关系

```
MainWindow
    ├── NovelService
    │       ├── ApiClient (HTTP 通信)
    │       └── SupabaseClient (数据库操作)
    ├── CharacterService
    │       ├── ApiClient
    │       └── StorageClient (文件上传)
    ├── StoryboardService
    │       ├── ApiClient
    │       └── QwenAdapter (AI 文本分析)
    └── PanelService
            ├── ApiClient
            ├── ImagenAdapter (AI 图像生成)
            └── StorageClient (图像存储)
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
    

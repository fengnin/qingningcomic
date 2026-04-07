# QingNing 漫画分镜系统 - 面试技术文档

## 项目概述

**项目名称**: QingNing (青柠漫画分镜系统)  
**技术栈**: C++11 + Qt 5.15.2 + MySQL 8.0  
**代码规模**: ~21,338 行代码，95 个源文件（47 个 .cpp + 48 个 .h）  
**开发周期**: 2024年至今

---

## 一、项目架构

### 1.1 整体架构图

```
┌─────────────────────────────────────────────────────────────────┐
│                        QingNing 应用层                          │
├─────────────────────────────────────────────────────────────────┤
│  MainWindow                                                      │
│  ├── DashboardPage (总览)                                        │
│  ├── NovelUploadPage (上传)                                      │
│  ├── NovelPage (项目空间)                                        │
│  ├── NovelDetailPage (作品详情)                                  │
│  ├── CharacterPage (角色管理)                                    │
│  └── ExportPage (导出中心)                                       │
├─────────────────────────────────────────────────────────────────┤
│                      ViewModel 层                                │
├─────────────────────────────────────────────────────────────────┤
│  NovelViewModel       小说数据管理                               │
│  StoryboardViewModel  分镜数据管理                               │
│  BaseViewModel        通用状态管理 (busy/error)                  │
├─────────────────────────────────────────────────────────────────┤
│                        业务服务层                                │
├─────────────────────────────────────────────────────────────────┤
│  NovelService        小说管理服务                                │
│  StoryboardService   分镜生成服务                                │
│  AnalysisService     数据分析服务                                │
├─────────────────────────────────────────────────────────────────┤
│                        数据访问层                                │
├─────────────────────────────────────────────────────────────────┤
│  DatabaseManager     MySQL 数据库管理                            │
│  FileStorage         本地文件存储                                │
│  StorageClient       S3 云存储客户端                             │
├─────────────────────────────────────────────────────────────────┤
│                        外部服务层                                │
├─────────────────────────────────────────────────────────────────┤
│  QwenClient          通义千问 API 客户端                         │
│  QwenImageClient     Qwen 图像生成 API 客户端                    │
│  StorageClient       S3 云存储客户端                             │
│  JsonRepair          JSON 修复库                                 │
│  SchemaToPrompt      Schema 转 Prompt 工具                       │
└─────────────────────────────────────────────────────────────────┘
```

### 1.2 目录结构

```
qingning/
├── include/              # 头文件
│   ├── components/       # UI 组件
│   ├── models/           # 数据模型
│   ├── services/         # 业务服务
│   └── utils/            # 工具类
├── src/
│   ├── api/              # API 客户端
│   ├── components/       # UI 组件实现
│   ├── data/             # 数据访问层
│   ├── models/           # 数据模型实现
│   ├── services/         # 业务服务实现
│   └── utils/            # 工具类实现
├── schemas/              # JSON Schema 定义
└── config.ini            # 配置文件
```

---

## 二、核心功能模块

### 2.1 AI 分镜生成流程

#### 整体流程图

```
小说文本 → 文本分割 → 多次调用AI API → 结果合并 → 保存数据库
```

#### 详细步骤

```
用户输入小说文本
        │
        ▼
┌───────────────────────────────────┐
│  1. 入口：AnalysisService          │
│  .analyzeNovel()                   │
│  - 估算段落数                       │
│  - 创建任务推入队列                 │
└───────────────────────────────────┘
        │
        ▼
┌───────────────────────────────────┐
│  2. 文本分割                       │
│  splitTextIntelligently()          │
│  - 以 8000 字符为基准切分           │
│  - 按 \n\n+ 分割段落               │
│  - 超长段落按句子切分               │
└───────────────────────────────────┘
        │
        ▼
┌───────────────────────────────────┐
│  3. 构建提示词                     │
│  buildUserMessageWithBible()       │
│  - 附带现有角色/场景圣经            │
│  - 标记段落结构                     │
└───────────────────────────────────┘
        │
        ▼
┌───────────────────────────────────┐
│  4. 调用 Qwen API                  │
│  callStoryboardApi()               │
│  - 使用 storyboard.json Schema     │
│  - 第一个 chunk 带圣经上下文        │
│  - 后续 chunk 只带空数组            │
│  - (带重试机制)                    │
└───────────────────────────────────┘
        │
        ▼
┌───────────────────────────────────┐
│  5. 结果规范化                     │
│  ensureStoryboardShape()           │
│  - 补充缺失字段                     │
│  - 规范化面板结构                   │
│  - 提取角色和场景信息               │
└───────────────────────────────────┘
        │
        ▼
┌───────────────────────────────────┐
│  6. 合并结果                       │
│  mergeStoryboards()                │
│  - 合并所有 chunk 的面板            │
│  - 去重角色和场景                   │
│  - 计算总页数                       │
└───────────────────────────────────┘
        │
        ▼
┌───────────────────────────────────┐
│  7. 保存结果                       │
│  AnalysisService.processTask()     │
│  - 保存到数据库                     │
│  - 返回统计信息                     │
└───────────────────────────────────┘
        │
        ▼
    输出分镜数据
```

#### 面板数量决定因素

**面板数量由 AI 决定**，不是由代码固定计算。代码只负责：
1. 分割文本
2. 发送给 AI
3. 接收 AI 返回的面板数组
4. 合并和规范化

AI 根据以下因素决定面板数量：
- 文本内容
- 场景变化
- 对话密度
- System Prompt 中的指导规则

#### 关键配置

| 参数 | 值 | 说明 |
|------|-----|------|
| `maxChunkLength` | 8000 | 每块最大字符数 |
| `PANELS_PER_PAGE` | 6 | 每页面板数 |
| Schema | storyboard.json | 输出结构定义 |

### 2.2 文本分割算法

#### Chunk 概念

**Chunk** = **文本块**，是将长文本分割成的小片段，每个片段单独发送给 AI API 处理。

```
问题：AI API 有 token 限制
    │
    ├── Qwen API 单次请求限制：约 8000 字符
    │
    └── 小说文本可能很长：几万字甚至更多
            │
            ▼
    解决方案：将长文本分割成多个 chunk，逐个处理
```

#### 核心代码

```cpp
QStringList QwenClient::splitTextIntelligently(const QString& text, int maxLength)
{
    // 1. 按双换行符分割段落
    QStringList paragraphs = text.split(QRegularExpression("\n\n+"));
    QStringList chunks;
    QString currentChunk;
    
    // 2. 遍历每个段落
    for (const QString& para : paragraphs) {
        // 情况A: 段落超长 → 按句子拆分
        if (para.length() > maxLength) {
            if (!currentChunk.isEmpty()) {
                chunks.append(currentChunk.trimmed());
                currentChunk.clear();
            }
            
            QStringList sentences = extractSentences(para);
            for (const QString& sentence : sentences) {
                if (currentChunk.length() + sentence.length() > maxLength) {
                    if (!currentChunk.isEmpty()) {
                        chunks.append(currentChunk.trimmed());
                    }
                    currentChunk = sentence;
                } else {
                    currentChunk += sentence;
                }
            }
            continue;
        }
        
        // 情况B: 段落不长 → 动态合并
        if (currentChunk.length() + para.length() + 2 > maxLength) {
            if (!currentChunk.isEmpty()) {
                chunks.append(currentChunk.trimmed());
            }
            currentChunk = para;
        } else {
            if (!currentChunk.isEmpty()) {
                currentChunk += "\n\n";
            }
            currentChunk += para;
        }
    }
    
    // 3. 添加最后一个块
    if (!currentChunk.isEmpty()) {
        chunks.append(currentChunk.trimmed());
    }
    
    return chunks.isEmpty() ? QStringList{ text } : chunks;
}
```

#### 设计要点

| 要点 | 说明 |
|------|------|
| **优先保持段落完整性** | 尽量不拆分段落 |
| **超长段落降级到句子级别** | 段落超长才按句子拆分 |
| **动态合并小段落** | 减少不必要的 API 调用 |
| **maxLength = 8000** | 适配 Qwen API 的 token 限制 |

---

## 三、JsonRepair 库设计

### 3.1 支持的修复功能

| 问题类型 | 示例 | 修复后 |
|----------|------|--------|
| 缺失引号 | `{name: "test"}` | `{"name": "test"}` |
| 单引号字符串 | `{'key': 'value'}` | `{"key": "value"}` |
| 尾部逗号 | `{a: 1, b: 2,}` | `{"a": 1, "b": 2}` |
| 多余逗号 | `{a: 1,, b: 2}` | `{"a": 1, "b": 2}` |
| JavaScript 注释 | `/* comment */ {a:1}` | `{"a": 1}` |
| 中文引号 | `{名字: "测试"}` | `{"名字": "测试"}` |
| 布尔常量 | `{active: true}` | `{"active": true}` |
| undefined | `{val: undefined}` | `{"val": null}` |

### 3.2 修复流程图

```
原始 JSON 字符串
        │
        ▼
┌─────────────────────────────────┐
│  1. 预处理                      │
│  - 替换中文引号 " " → " "       │
│  - 移除 Markdown 代码块         │
└─────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────┐
│  2. 标准解析                    │
│  QJsonDocument::fromJson()      │
│  成功 → 返回                    │
└─────────────────────────────────┘
        │ 失败
        ▼
┌─────────────────────────────────┐
│  3. JsonRepair 库修复           │
│  - 词法分析识别 Token           │
│  - 语法解析自动修复错误          │
│  成功 → 返回                    │
└─────────────────────────────────┘
        │ 失败
        ▼
┌─────────────────────────────────┐
│  4. 手动修复常见问题            │
│  - 移除尾部逗号                 │
│  - 修复缺失引号                 │
└─────────────────────────────────┘
```

---

## 四、数据库设计

### 4.1 核心表结构

```sql
-- 小说表 (MySQL 8.0)
CREATE TABLE novels (
    id VARCHAR(36) PRIMARY KEY,
    title VARCHAR(255) NOT NULL,
    author VARCHAR(100),
    content LONGTEXT,
    status ENUM('draft', 'processing', 'completed'),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_status (status),
    INDEX idx_created (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 分镜表
CREATE TABLE panels (
    id VARCHAR(36) PRIMARY KEY,
    novel_id VARCHAR(36),
    page INT,
    index INT,
    scene TEXT,
    background JSON,          -- MySQL 8.0 原生 JSON 类型
    characters JSON,
    dialogue JSON,
    visual_prompt TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (novel_id) REFERENCES novels(id) ON DELETE CASCADE,
    INDEX idx_novel_page (novel_id, page, index)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 角色表
CREATE TABLE characters (
    id VARCHAR(36) PRIMARY KEY,
    novel_id VARCHAR(36),
    name VARCHAR(100),
    role ENUM('protagonist', 'antagonist', 'supporting'),
    appearance JSON,
    personality JSON,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (novel_id) REFERENCES novels(id) ON DELETE CASCADE,
    INDEX idx_novel (novel_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
```

### 4.2 DatabaseManager 设计模式

```cpp
class DatabaseManager {
public:
    static DatabaseManager* instance();  // 单例模式
    
    // CRUD 操作
    QVariantMap selectOne(const QString& table, 
                          const QString& where, 
                          const QVariantList& values);
    QList<QVariantMap> selectAll(...);
    qint64 insert(const QString& table, const QVariantMap& data);
    bool update(const QString& table, const QVariantMap& data, ...);
    bool remove(const QString& table, const QString& where, ...);
    
private:
    bool checkConnection(const QString& operation);
    bool setValidationError(const QString& message);
    QSqlQuery executeQuery(const QString& sql, const QString& op);
};
```

---

## 五、技术亮点

### 5.1 Schema 驱动的 Prompt 生成

```cpp
QString SchemaToPrompt::buildSystemPrompt(const QJsonObject& schema, 
                                           const Options& options)
{
    QString prompt;
    
    // 1. 角色定义
    prompt += "你是一个专业的漫画分镜师...\n\n";
    
    // 2. 输出格式说明
    prompt += "## 输出格式\n";
    prompt += "严格按照以下 JSON Schema 输出:\n";
    prompt += formatSchema(schema);
    
    // 3. 字段说明
    prompt += "\n## 字段说明\n";
    prompt += generateFieldDescriptions(schema);
    
    return prompt;
}
```

### 5.2 指数退避重试机制

```cpp
void QwenClient::exponentialBackoff(int attempt)
{
    int baseDelay = 1000;  // 1 秒
    int maxDelay = 30000;  // 30 秒
    int delay = qMin(baseDelay * qPow(2, attempt), maxDelay);
    
    // 添加随机抖动
    delay += QRandomGenerator::global()->bounded(1000);
    
    QThread::msleep(delay);
}
```

### 5.3 状态机设计 (分镜状态)

```cpp
enum class StoryboardStatus {
    Draft,        // 草稿
    Processing,   // 处理中
    Completed,    // 已完成
    Error         // 错误
};

// 状态转换表
static const QHash<StoryboardStatus, QSet<StoryboardStatus>> validTransitions = {
    {StoryboardStatus::Draft,      {StoryboardStatus::Processing}},
    {StoryboardStatus::Processing, {StoryboardStatus::Completed, StoryboardStatus::Error}},
    {StoryboardStatus::Error,      {StoryboardStatus::Processing}},
    {StoryboardStatus::Completed,  {}}
};
```

### 5.4 QwenImageClient 设计思路

#### 技术选型

| 方案 | 优点 | 缺点 | 选择 |
|------|------|------|------|
| Google Imagen 3 API | 专用于图像生成 | 需要 Vertex AI，费用高 | ❌ |
| Gemini Images API | 统一接口，支持多模态 | 免费额度少（2张/天） | ❌ |
| 通义万相 (Wan) | 阿里云生态，免费额度多 | 不支持文字渲染 | ❌ |
| **Qwen-Image-Plus** | 支持文字渲染，免费100张 | - | ✅ 选择 |
| Stable Diffusion | 开源可部署 | 需要本地 GPU | ❌ |
| DALL-E | 效果好 | 需要代理，费用高 | ❌ |

---

## 六、性能优化

### 6.1 数据库查询优化

| 优化项 | 实现方式 |
|--------|----------|
| 连接池 | 单例 + 长连接 |
| 批量插入 | 事务包装 |
| 索引优化 | novel_id, status 字段索引 |
| JSON 字段 | MySQL 8.0 原生 JSON 类型 |
| 窗口函数 | MySQL 8.0 窗口函数支持 |
| CTE | 公用表表达式优化复杂查询 |

### 6.2 内存管理

```cpp
// Qt 父子对象模型自动内存管理
class NovelPage : public QWidget {
    QWidget* m_content = new QWidget(this);  // 自动释放
};

// 智能指针用于非 QObject 对象
QScopedPointer<QNetworkReply> reply(manager->get(request));
```

### 6.3 图片缓存键优化

#### 问题背景

面板预览组件 (PanelCard) 使用异步图片加载器 (AsyncImageLoader) 缓存图片。原缓存键仅基于面板位置（如 `panel_1_2`），导致同一位置刷新新图片后仍显示旧缓存。

#### 问题分析

```
原缓存键: panel_1_2 (仅位置)
    │
    ├── 第一次加载: URL_A → 缓存到 panel_1_2
    │
    └── 刷新后: URL_B → 查找 panel_1_2 → 返回 URL_A 的图片 ❌
```

#### 解决方案

在缓存键中包含 URL 的哈希值，确保每个 URL 有唯一的缓存：

```cpp
void PanelCard::setPreviewUrl(const QString &url)
{
    m_previewUrl = url;
    m_currentImagePath = url;
    
    if (url.isEmpty()) {
        setPreviewState(PreviewState::Empty);
        return;
    }
    
    // 使用 URL 的 MD5 哈希值作为缓存键的一部分
    QString urlHash = QString(QCryptographicHash::hash(
        url.toUtf8(), QCryptographicHash::Md5).toHex());
    
    // 缓存键 = 位置 + URL哈希
    QString id = QString("panel_%1_%2_%3")
        .arg(m_chapterNumber).arg(m_panelNumber).arg(urlHash.left(8));
    
    QSize targetSize(Size::PREVIEW_WIDTH, Size::PREVIEW_HEIGHT);
    QString cacheKey = AsyncImageLoader::makeCacheKey(id, targetSize);
    
    // ... 加载逻辑 ...
}
```

#### 优化效果

| 指标 | 优化前 | 优化后 |
|------|--------|--------|
| 缓存准确性 | 同位置图片可能错乱 | 每个 URL 独立缓存 |
| 内存占用 | 正常 | 略增（不同 URL 分别缓存） |
| 用户体验 | 图片显示错误 | 图片显示正确 |

#### 设计要点

| 要点 | 说明 |
|------|------|
| **唯一性** | URL 哈希确保不同图片不同缓存键 |
| **性能** | MD5 计算快速，取前8位足够区分 |
| **兼容性** | 保留位置信息便于调试 |

---

## 七、错误处理策略

### 7.1 分层错误处理

```
┌─────────────────────────────────┐
│  UI 层                          │
│  - 显示用户友好错误信息         │
│  - 提供重试选项                 │
└─────────────────────────────────┘
            ↓
┌─────────────────────────────────┐
│  Service 层                     │
│  - 业务逻辑错误处理             │
│  - 日志记录                     │
└─────────────────────────────────┘
            ↓
┌─────────────────────────────────┐
│  Data 层                        │
│  - 数据库连接错误               │
│  - API 调用错误                 │
└─────────────────────────────────┘
```

### 7.2 错误恢复机制

```cpp
// API 调用重试
StoryboardResult QwenClient::generateStoryboard(...) {
    for (int attempt = 0; attempt < m_config.maxRetries; ++attempt) {
        try {
            return callApi(...);
        } catch (const ApiError& e) {
            if (attempt < m_config.maxRetries - 1) {
                exponentialBackoff(attempt);
                continue;
            }
            throw;
        }
    }
}
```

---

## 八、任务队列系统设计

### 8.1 问题分析

**核心问题**: AI 分镜生成耗时 30-90 秒，导致 UI 阻塞

```
无队列时的问题：
┌─────────────────────────────────────────────────────────────┐
│ 用户点击"生成分镜"                                          │
│         │                                                    │
│         ▼                                                    │
│ ┌───────────────────────────────────────────────────────┐   │
│ │ UI 线程阻塞等待 API 响应 (30-90秒)                     │   │
│ │ - 界面卡死，无法操作                                   │   │
│ │ - 用户不知道进度                                       │   │
│ │ - 无法取消操作                                         │   │
│ └───────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### 8.2 技术选型

| 方案 | 优点 | 缺点 | 选择 |
|------|------|------|------|
| QThread + 信号槽 | Qt 原生，简单 | 需要自己管理 | ✅ 选择 |
| QtConcurrent | 简单并行 | 不适合长任务 | ❌ |
| QThreadPool | 灵活 | 复杂度高 | ❌ |
| 外部队列 (Redis) | 功能强大 | 依赖外部服务 | ❌ |

### 8.3 架构设计

#### 生产者-消费者模式

```
┌─────────────────────────────────────────────────────────────────┐
│                      任务队列架构                                │
├─────────────────────────────────────────────────────────────────┤
│   生产者 (UI 线程)                                              │
│   └── TaskQueue::enqueue(task) ─────────────┐                  │
│                                              ▼                  │
│   ┌─────────────────────────────────────────────────────┐     │
│   │              TaskQueue (单例)                        │     │
│   │  ┌─────────┐  ┌─────────┐  ┌─────────┐              │     │
│   │  │ Queue   │  │ Tasks   │  │Handlers │              │     │
│   │  │ (缓冲区)│  │ (存储)  │  │(处理器) │              │     │
│   │  └─────────┘  └─────────┘  └─────────┘              │     │
│   └─────────────────────────────────────────────────────┘     │
│                          │                                     │
│              ┌───────────┼───────────┐                        │
│              ▼           ▼           ▼                        │
│   ┌─────────────┐ ┌─────────────┐ ┌─────────────┐            │
│   │  Worker 1   │ │  Worker 2   │ │  Worker N   │  消费者    │
│   │  (QThread)  │ │  (QThread)  │ │  (QThread)  │  (工作线程) │
│   └─────────────┘ └─────────────┘ └─────────────┘            │
└─────────────────────────────────────────────────────────────────┘
```

### 8.4 核心代码实现

```cpp
// 任务数据结构
struct TaskData {
    QString id;
    TaskType type;
    TaskStatus status;
    QString novelId;
    QString text;
    QJsonObject params;
    QJsonObject result;
    int total = 0;
    int completed = 0;
};

// 任务队列单例
class TaskQueue : public QObject {
    static TaskQueue* instance();
    QString enqueue(const TaskData& task);
    void registerHandler(TaskType type, TaskHandler handler);
signals:
    void taskStarted(const QString& taskId);
    void taskProgress(const QString& taskId, int progress, const QString& message);
    void taskCompleted(const QString& taskId, const QJsonObject& result);
    void taskFailed(const QString& taskId, const QString& error);
};

// 工作线程
class TaskWorker : public QThread {
    void run() override {
        while (m_running) {
            TaskData task = dequeue();
            if (task.isValid()) {
                processTask(task);
            }
        }
    }
};
```

### 8.5 与原仓库 AWS SQS 对比

| 特性 | AWS SQS (原仓库) | TaskQueue (当前) |
|------|------------------|------------------|
| 部署环境 | AWS 云服务 | 本地内存 |
| 持久化 | AWS 托管 | MySQL 数据库 |
| 扩展性 | 自动扩展 | 固定 Worker 数量 |
| 适用场景 | 多用户分布式 | 单用户桌面 |
| 成本 | 按量付费 | 免费 |

---

## 九、多线程崩溃问题排查与修复

### 9.1 问题描述

**现象**: 点击"面板批量生成"按钮后，程序在生成第一张面板图片后立即崩溃。

### 9.2 问题分析

#### 崩溃流程追踪

```
1. 主线程创建 TaskQueue，启动 TaskWorker 线程
        │
        ▼
2. TaskWorker 执行 handler，调用 ImageService::generatePanelImage()
        │
        ▼
3. generatePanelImage() 调用 StoryboardService::getPanel()
   └── 直接使用 QSqlQuery(m_db->database())，绕过了 DatabaseManager 的互斥锁
        │
        ▼
4. 同时主线程可能也在访问数据库（如刷新 UI）
   └── 两个线程同时操作同一个 QSqlDatabase 连接
        │
        ▼
5. MySQL 驱动内部状态被破坏 → 崩溃
```

### 9.3 根因分析

| 问题 | 原因 |
|------|------|
| DatabaseManager 缺少线程安全保护 | 多个线程可以同时调用 `executeSql()` |
| StoryboardService 直接使用 QSqlQuery | 绕过 DatabaseManager 的保护 |
| 单例创建非线程安全 | 多线程可能创建多个实例 |
| 在持有锁时发射信号 | 可能导致死锁 |

### 9.4 解决方案

#### 修复一：DatabaseManager 添加递归互斥锁

```cpp
class DatabaseManager
{
private:
    static QMutex m_instanceMutex;        // 保护单例创建
    mutable QRecursiveMutex m_mutex;      // 保护数据库操作（递归锁）
};

DatabaseManager* DatabaseManager::instance()
{
    if (!m_instance) {
        QMutexLocker locker(&m_instanceMutex);  // 双重检查锁定
        if (!m_instance) {
            m_instance = new DatabaseManager();
        }
    }
    return m_instance;
}
```

#### 修复二：StoryboardService 使用 DatabaseManager 的线程安全方法

```cpp
// 修复后 - 使用 DatabaseManager 的线程安全方法
Panel StoryboardService::getPanel(const QString& panelId)
{
    QString sql = QString("SELECT %1 FROM panels WHERE id = '%2'")
        .arg(PANEL_SELECT_FIELDS).arg(panelId);
    
    QList<QVariantMap> results = m_db->executeQuery(sql);  // ✅ 通过 DatabaseManager
    return buildPanelFromMap(results.first());
}
```

#### 修复三：信号发射前释放锁

```cpp
void TaskWorker::finalizeTask(const QString& taskId, const TaskResult& result)
{
    // 在锁作用域内更新状态
    m_queue->withLock([this, &taskId, &result]() {
        TaskData* task = m_queue->getTaskRef(taskId);
        if (task) {
            task->markAsCompleted(result.result);
            m_queue->saveTaskToDatabase(*task);
        }
    });  // 锁在这里释放
    
    // 锁释放后再发射信号
    emit taskCompleted(taskId, result.result);
}
```

### 9.5 关键技术要点

| 要点 | 说明 |
|------|------|
| **QRecursiveMutex** | 允许同一线程多次获取锁，避免递归调用死锁 |
| **双重检查锁定 (DCLP)** | 单例模式的线程安全实现 |
| **Qt::QueuedConnection** | 信号槽跨线程安全调用 |
| **锁释放后发射信号** | 避免槽函数中再次获取锁导致死锁 |
| **统一数据库访问入口** | 所有数据库操作通过 DatabaseManager |

---

## 十、ViewModel 层设计

### 10.1 问题背景

**原有架构问题**：UI 层直接调用 Service 单例获取数据，导致耦合过紧。

| 问题 | 说明 |
|------|------|
| **难以测试** | UI 层直接依赖 Service 单例，无法注入 Mock |
| **状态分散** | busy、error 等状态在各页面重复实现 |
| **信号转发复杂** | 每个 Page 都要连接 Service 信号 |

### 10.2 技术选型

| 方案 | 优点 | 缺点 | 选择 |
|------|------|------|------|
| **完整 MVVM** | 数据绑定、命令模式 | Qt 没有原生支持 | ❌ |
| **轻量 ViewModel** | 简单、Qt 信号槽原生支持 | 需要手动管理状态 | ✅ 选择 |
| **Presenter 模式** | 完全解耦 | 复杂度高 | ❌ |

### 10.3 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                        UI 层 (Pages)                             │
│  - 连接 ViewModel 信号                                           │
│  - 调用 ViewModel 方法                                           │
│  - 不直接访问 Service                                            │
└─────────────────────────────────────────────────────────────────┘
                              │
                              │ 信号槽连接
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                      ViewModel 层                                │
│  ┌─────────────────────┐  ┌─────────────────────────────────┐  │
│  │ BaseViewModel       │  │ NovelViewModel                  │  │
│  │ - m_busy: bool      │  │ - m_novels: QList<Novel>        │  │
│  │ - m_lastError: str  │  │ - m_currentNovel: Novel         │  │
│  └─────────────────────┘  └─────────────────────────────────┘  │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │ StoryboardViewModel                                         ││
│  │ - m_storyboards, m_currentStoryboard, m_currentPanels       ││
│  └─────────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                        Service 层                                │
└─────────────────────────────────────────────────────────────────┘
```

### 10.4 核心代码实现

```cpp
// BaseViewModel - 基类
class BaseViewModel : public QObject
{
    Q_OBJECT
public:
    bool isBusy() const { return m_busy; }
    QString lastError() const { return m_lastError; }

signals:
    void busyChanged(bool busy);
    void errorOccurred(const QString& error);

protected:
    void setBusy(bool busy);
    void setLastError(const QString& error);

private:
    bool m_busy = false;
    QString m_lastError;
};

// NovelViewModel - 小说数据管理
class NovelViewModel : public BaseViewModel
{
    Q_OBJECT
public:
    static NovelViewModel* instance();
    
    QList<Novel> novels() const { return m_novels; }
    Novel currentNovel() const { return m_currentNovel; }
    
    void loadNovels(int page = 1, int pageSize = 20);
    void loadNovel(const QString& novelId);
    void createNovel(const QString& userId, const QString& title, const QString& text);

signals:
    void novelsLoaded(const QList<Novel>& novels, int totalCount);
    void novelLoaded(const Novel& novel);
    void currentNovelChanged(const Novel& novel);
};
```

---

## 十一、面试问答准备

### Q1: 为什么选择 Qt 开发桌面应用？

**回答要点**:
1. 跨平台支持 (Windows/macOS/Linux)
2. 丰富的 UI 组件库
3. 完善的信号槽机制
4. 内置网络、数据库支持
5. 活跃的社区和文档

### Q2: JsonRepair 库的设计思路？

**回答要点**:
1. 参考 jsonrepair (TypeScript) 实现
2. 分离词法分析和语法解析
3. 支持多种 JSON 错误修复
4. 多层兜底策略 (标准解析 → 库修复 → 手动修复)

### Q3: 如何处理大文本的 AI 分镜生成？

**回答要点**:
1. 智能文本分割算法
2. 保持段落语义完整性
3. 分块处理 + 结果合并
4. 进度反馈给用户

### Q4: 数据库设计有哪些考虑？

**回答要点**:
1. 规范化设计减少冗余
2. JSON 字段存储复杂结构
3. 索引优化查询性能
4. 单例模式管理连接

### Q5: 项目中遇到的最大挑战？

**回答要点**:
1. LLM 输出的 JSON 格式不稳定
2. 解决方案：多层 JSON 修复机制
3. 从简单正则到完整 JsonRepair 库
4. 最终实现 99%+ 的解析成功率

### Q6: 多线程崩溃问题如何排查？

**回答要点**:
1. 分析线程模型，找出并发访问点
2. 发现 QSqlDatabase 不支持多线程共享
3. 解决方案：递归互斥锁 + 统一数据库访问入口
4. 注意信号发射时机，避免死锁

### Q7: 图片缓存键优化的思路？

**回答要点**:
1. 问题：原缓存键仅基于位置，刷新后显示错误图片
2. 分析：同一位置不同 URL 应有不同缓存
3. 解决：缓存键 = 位置 + URL哈希值
4. 效果：确保每个 URL 有唯一缓存，图片显示正确

---

## 十二、技术栈总结

| 类别 | 技术 |
|------|------|
| 语言 | C++11 |
| 框架 | Qt 5.15.2 |
| 数据库 | MySQL 8.0 |
| AI API | 通义千问 (Qwen) + Qwen-Image (图像生成) |
| 存储 | 本地文件 + S3 兼容云存储 |
| 构建 | qmake |
| 版本控制 | Git |

---

## 十三、在线演示方案设计

### 13.1 问题背景

**需求**: 在简历上放置链接，让面试官点击即可体验 Qt 桌面应用。

**挑战**:
- Qt 桌面应用无法直接在浏览器运行
- 云服务器远程桌面画质不佳
- 面试官不应安装额外软件

### 13.2 技术选型对比

| 方案 | 画质 | 浏览器访问 | 改动代码 | 部署难度 | 选择 |
|------|------|-----------|---------|---------|------|
| noVNC | ⭐⭐ | ✅ | ❌ | 简单 | ❌ 画质差 |
| TurboVNC + noVNC | ⭐⭐⭐ | ✅ | ❌ | 简单 | ❌ 有提升但有限 |
| **Guacamole** | ⭐⭐⭐⭐ | ✅ | ❌ | 中等 | ✅ 选择 |
| Selkies (WebRTC) | ⭐⭐⭐⭐⭐ | ✅ | ❌ | 中等 | ❌ 需要 GPU |
| Kasm Workspaces | ⭐⭐⭐⭐ | ✅ | ❌ | 复杂 | ❌ 过于复杂 |
| Qt WebAssembly | ⭐⭐⭐⭐⭐ | ✅ | ✅ 大改 | 高 | ❌ 改动量大 |

### 13.3 为什么选择 Guacamole

```
┌─────────────────────────────────────────────────────┐
│  Guacamole 优势                                     │
├─────────────────────────────────────────────────────┤
│  ✅ 底层使用 RDP 协议，画质优于 VNC                 │
│  ✅ 浏览器直接访问，面试官无需安装软件              │
│  ✅ 不需要改动 Qt 项目代码                          │
│  ✅ Apache 开源项目，成熟稳定                       │
│  ✅ 4核4G 服务器完全够用                            │
└─────────────────────────────────────────────────────┘
```

### 13.4 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                        在线演示架构                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   面试官浏览器                                                   │
│        │                                                        │
│        │ HTTP/WebSocket                                         │
│        ▼                                                        │
│   ┌─────────────────────────────────────────┐                  │
│   │  Apache Guacamole (Docker)              │                  │
│   │  - Web 前端 (HTML5 Canvas)              │                  │
│   │  - guacd 代理服务                       │                  │
│   │  - 协议转换 (HTTP ↔ RDP)                │                  │
│   └─────────────────────────────────────────┘                  │
│        │                                                        │
│        │ RDP 协议                                               │
│        ▼                                                        │
│   ┌─────────────────────────────────────────┐                  │
│   │  xrdp 服务                              │                  │
│   │  - Linux RDP 服务端                     │                  │
│   └─────────────────────────────────────────┘                  │
│        │                                                        │
│        │ X11                                                    │
│        ▼                                                        │
│   ┌─────────────────────────────────────────┐                  │
│   │  XFCE 桌面环境                          │                  │
│   │  - 轻量级 Linux 桌面                    │                  │
│   └─────────────────────────────────────────┘                  │
│        │                                                        │
│        ▼                                                        │
│   ┌─────────────────────────────────────────┐                  │
│   │  QingNingComic Qt 应用                  │                  │
│   │  - 漫画分镜生成工具                     │                  │
│   └─────────────────────────────────────────┘                  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 13.5 为什么 RDP 画质优于 VNC

| 对比项 | VNC | RDP |
|--------|-----|-----|
| 传输内容 | 像素块 | 图形指令 + 像素 |
| 压缩方式 | JPEG/Tight | RemoteFX/Negotiate |
| 文字渲染 | 模糊 | 清晰 |
| 带宽效率 | 较低 | 较高 |
| 延迟 | 较高 | 较低 |

**核心差异**: RDP 传输的是"画一个矩形"这类图形指令，而非原始像素，因此更高效、画质更好。

### 13.6 部署方案 (CentOS)

```bash
# 1. 安装 Docker
yum install -y docker-ce
systemctl start docker && systemctl enable docker

# 2. 安装 XFCE 桌面
yum groupinstall -y "Xfce"

# 3. 安装 xrdp
yum install -y epel-release xrdp
systemctl start xrdp && systemctl enable xrdp

# 4. 开放端口
firewall-cmd --permanent --add-port=8080/tcp
firewall-cmd --permanent --add-port=3389/tcp
firewall-cmd --reload

# 5. 创建演示账号
useradd demo && echo "demo:demo123" | chpasswd

# 6. 启动 Guacamole
docker run -d --name guacamole \
  -p 8080:8080 \
  -v ~/guacamole:/config \
  --restart always \
  oznu/guacamole
```

### 13.7 服务器资源配置

```
服务器配置: 4核 CPU / 4GB 内存

资源占用预估:
├── XFCE 桌面:     ~300MB
├── xrdp 服务:     ~50MB
├── Guacamole:     ~200MB
├── Qt 应用:       ~200MB
├── 系统预留:      ~500MB
├───────────────────────
│ 剩余可用:        ~2.7GB
└───────────────────────

✅ 完全够用，支持 2-3 人同时访问
```

### 13.8 简历展示方式

```
┌──────────────────────────────────────────────────────────────┐
│  QingNingComic - AI 漫画分镜生成工具                         │
│                                                              │
│  📺 演示视频: https://bilibili.com/BVxxx                     │
│     (完整功能演示，最佳画质)                                  │
│                                                              │
│  🌐 在线体验: http://xxx.xxx.xxx:8080                        │
│     账号: demo / demo123                                     │
│     (浏览器直接访问，无需安装)                                │
│                                                              │
│  💻 源码仓库: https://github.com/xxx/qingning-comic          │
│     (代码质量 + 技术文档)                                     │
└──────────────────────────────────────────────────────────────┘
```

### 13.9 方案局限性

| 局限 | 说明 | 应对 |
|------|------|------|
| 画质有损 | 网络传输压缩 | 录制视频作为补充 |
| 依赖网络 | 需要稳定网络 | 提供视频备选 |
| 资源占用 | 服务器需要图形界面 | 4核4G 足够 |

### 13.10 后续规划

| 阶段 | 内容 |
|------|------|
| 短期 | 完善演示账号权限，限制操作范围 |
| 中期 | 添加访问日志，了解面试官行为 |
| 长期 | 考虑 Qt WebAssembly 方案，实现原生画质 |

---

*文档版本: 4.0*  
*最后更新: 2025年*

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
│  StorageClient       HTTP 远程文件存储客户端                     │
├─────────────────────────────────────────────────────────────────┤
│                        外部服务层                                │
├─────────────────────────────────────────────────────────────────┤
│  QwenClient          通义千问 API 客户端                         │
│  QwenImageClient     Qwen 图像生成 API 客户端                    │
│  StorageClient       HTTP 远程文件存储客户端                     │
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

## 六、HTTP 远程文件存储服务设计

### 6.1 问题背景

**需求**: 将图片等文件存储到云服务器磁盘，而非本地或云对象存储（如腾讯云 COS）。

**挑战**:
- 程序在本地运行，文件需要通过网络传输到云服务器
- 不想额外购买云对象存储服务
- 需要简单、低成本的解决方案

### 6.2 技术选型对比

| 方案 | 优点 | 缺点 | 选择 |
|------|------|------|------|
| **腾讯云 COS** | 成熟稳定、CDN 加速 | 额外付费 | ❌ |
| **MinIO (自建)** | 兼容 S3、功能完整 | 需要部署 Docker | ❌ |
| **HTTP 文件服务** | 简单、轻量、免费 | 功能相对简单 | ✅ 选择 |
| **Samba (SMB)** | Windows 原生支持 | 配置复杂、端口问题 | ❌ |
| **SFTP** | 安全、成熟 | 需要额外库支持 | ❌ |

### 6.3 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                      文件存储架构                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   本地 Qt 客户端                                                 │
│        │                                                        │
│        │ HTTP POST / GET                                        │
│        ▼                                                        │
│   ┌─────────────────────────────────────────┐                  │
│   │  云服务器 (腾讯云轻量应用服务器)          │                  │
│   │                                         │                  │
│   │  ┌─────────────────────────────────┐   │                  │
│   │  │  Flask 文件上传服务 (端口 8080)  │   │                  │
│   │  │  - /upload: 上传文件            │   │                  │
│   │  │  - /download/<path>: 下载文件   │   │                  │
│   │  └─────────────────────────────────┘   │                  │
│   │                  │                      │                  │
│   │                  ▼                      │                  │
│   │  ┌─────────────────────────────────┐   │                  │
│   │  │  /data/comic/                   │   │                  │
│   │  │  ├── panels/    (面板图片)      │   │                  │
│   │  │  ├── characters/(角色肖像)      │   │                  │
│   │  │  ├── scenes/    (场景参考图)    │   │                  │
│   │  │  └── exports/   (导出文件)      │   │                  │
│   │  └─────────────────────────────────┘   │                  │
│   │                                         │                  │
│   │  ┌─────────────────────────────────┐   │                  │
│   │  │  MySQL 8.0 (端口 3306)          │   │                  │
│   │  │  - 存储文件路径引用             │   │                  │
│   │  └─────────────────────────────────┘   │                  │
│   └─────────────────────────────────────────┘                  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 6.4 服务端实现

```python
# /data/upload_server.py
from flask import Flask, request, jsonify, send_from_directory
import os

app = Flask(__name__)
UPLOAD_DIR = "/data/comic"

@app.route('/upload', methods=['POST'])
def upload():
    file = request.files['file']
    path = request.form.get('path', '')
    
    full_path = os.path.join(UPLOAD_DIR, path)
    os.makedirs(os.path.dirname(full_path), exist_ok=True)
    file.save(full_path)
    
    return jsonify({"success": True, "path": path})

@app.route('/download/<path:path>', methods=['GET'])
def download(path):
    return send_from_directory(UPLOAD_DIR, path)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8080)
```

### 6.5 客户端实现

```cpp
// StorageClient.cpp
bool StorageClient::upload(const QString& path, const QByteArray& data, const QString& contentType)
{
    QNetworkRequest request(QUrl(m_endpoint + "/upload"));
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    
    // 添加文件数据
    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, 
                       QVariant("form-data; name=\"file\"; filename=\"file\""));
    filePart.setBody(data);
    multiPart->append(filePart);
    
    // 添加路径参数
    QHttpPart pathPart;
    pathPart.setHeader(QNetworkRequest::ContentDispositionHeader, 
                       QVariant("form-data; name=\"path\""));
    pathPart.setBody(path.toUtf8());
    multiPart->append(pathPart);
    
    QNetworkReply *reply = m_manager->post(request, multiPart);
    // ... 等待响应
}
```

### 6.6 与对象存储对比

| 对比项 | 腾讯云 COS | HTTP 文件服务 |
|--------|-----------|---------------|
| 成本 | 按量付费 | 免费（使用服务器磁盘） |
| 部署 | 无需部署 | 需要部署 Flask |
| 扩展性 | 自动扩展 | 受服务器磁盘限制 |
| CDN | 支持 | 不支持 |
| 适用场景 | 大规模生产 | 个人项目/演示 |

---

## 七、性能优化

### 7.1 数据库查询优化

| 优化项 | 实现方式 |
|--------|----------|
| 连接池 | 单例 + 长连接 |
| 批量插入 | 事务包装 |
| 索引优化 | novel_id, status 字段索引 |
| JSON 字段 | MySQL 8.0 原生 JSON 类型 |
| 窗口函数 | MySQL 8.0 窗口函数支持 |
| CTE | 公用表表达式优化复杂查询 |

### 7.2 内存管理

```cpp
// Qt 父子对象模型自动内存管理
class NovelPage : public QWidget {
    QWidget* m_content = new QWidget(this);  // 自动释放
};

// 智能指针用于非 QObject 对象
QScopedPointer<QNetworkReply> reply(manager->get(request));
```

### 7.3 图片缓存键优化

#### 问题背景

面板预览组件 (PanelCard) 使用异步图片加载器 (AsyncImageLoader) 缓存图片。原缓存键仅基于面板位置（如 `panel_1_2`），导致同一位置刷新新图片后仍显示旧缓存。

#### 解决方案

在缓存键中包含 URL 的哈希值，确保每个 URL 有唯一的缓存：

```cpp
void PanelCard::setPreviewUrl(const QString &url)
{
    // 使用 URL 的 MD5 哈希值作为缓存键的一部分
    QString urlHash = QString(QCryptographicHash::hash(
        url.toUtf8(), QCryptographicHash::Md5).toHex());
    
    // 缓存键 = 位置 + URL哈希
    QString id = QString("panel_%1_%2_%3")
        .arg(m_chapterNumber).arg(m_panelNumber).arg(urlHash.left(8));
    
    // ... 加载逻辑 ...
}
```

---

## 八、错误处理策略

### 8.1 分层错误处理

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

### 8.2 错误恢复机制

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

## 九、任务队列系统设计

### 9.1 问题分析

**核心问题**: AI 分镜生成耗时 30-90 秒，导致 UI 阻塞

### 9.2 技术选型

| 方案 | 优点 | 缺点 | 选择 |
|------|------|------|------|
| QThread + 信号槽 | Qt 原生，简单 | 需要自己管理 | ✅ 选择 |
| QtConcurrent | 简单并行 | 不适合长任务 | ❌ |
| QThreadPool | 灵活 | 复杂度高 | ❌ |
| 外部队列 (Redis) | 功能强大 | 依赖外部服务 | ❌ |

### 9.3 架构设计

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

### 9.4 与原仓库 AWS SQS 对比

| 特性 | AWS SQS (原仓库) | TaskQueue (当前) |
|------|------------------|------------------|
| 部署环境 | AWS 云服务 | 本地内存 |
| 持久化 | AWS 托管 | MySQL 数据库 |
| 扩展性 | 自动扩展 | 固定 Worker 数量 |
| 适用场景 | 多用户分布式 | 单用户桌面 |
| 成本 | 按量付费 | 免费 |

---

## 十、异步信号积压问题处理

### 10.1 问题背景

在 Qt 应用程序中，当用户快速连续触发多次异步操作时，会出现信号积压问题：

```
问题场景：
┌─────────────────────────────────────────────────────────────┐
│  分析1 开始 → 发送进度信号 10%, 20%, 30%...                  │
│  用户快速开始分析2 → 旧信号 40%, 50% 还在队列中               │
│  如果不处理，分析2 的进度条会从 40% 开始显示 ❌               │
└─────────────────────────────────────────────────────────────┘
```

### 10.2 解决方案：代数追踪 + 延迟重置

```cpp
void AnalysisProgressWidget::setWaitingAnalysisState() {
    m_progressBar->setValue(0);
    
    int gen = ++m_analysisGeneration;  // 递增代数
    
    // 延迟重置：在事件循环处理完积压信号后执行
    QTimer::singleShot(0, this, [this, gen]() {
        if (gen == m_analysisGeneration) {
            m_progressBar->setValue(0);
        }
    });
}

void AnalysisProgressWidget::onAnalysisProgress(int progress) {
    // 单向进度：只允许向前移动
    if (progress >= m_progressBar->value()) {
        m_progressBar->setValue(progress);
    }
}
```

### 10.3 关键技术点

| 技术 | 作用 | 说明 |
|------|------|------|
| `m_analysisGeneration` | 代数追踪 | 区分不同分析批次 |
| `QTimer::singleShot(0)` | 延迟执行 | 在当前事件循环处理完后执行 |
| `progress >= value()` | 单向进度约束 | 进度只能向前，旧信号自动被忽略 |

---

## 十一、多线程崩溃问题排查与修复

### 11.1 问题描述

**现象**: 点击"面板批量生成"按钮后，程序在生成第一张面板图片后立即崩溃。

### 11.2 根因分析

| 问题 | 原因 |
|------|------|
| DatabaseManager 缺少线程安全保护 | 多个线程可以同时调用 `executeSql()` |
| StoryboardService 直接使用 QSqlQuery | 绕过 DatabaseManager 的保护 |
| 单例创建非线程安全 | 多线程可能创建多个实例 |
| 在持有锁时发射信号 | 可能导致死锁 |

### 11.3 解决方案

```cpp
class DatabaseManager
{
private:
    static QMutex m_instanceMutex;        // 保护单例创建
    mutable QRecursiveMutex m_mutex;      // 保护数据库操作（递归锁）
};

// 信号发射前释放锁
void TaskWorker::finalizeTask(const QString& taskId, const TaskResult& result)
{
    m_queue->withLock([this, &taskId, &result]() {
        // 更新状态...
    });  // 锁在这里释放
    
    // 锁释放后再发射信号
    emit taskCompleted(taskId, result.result);
}
```

### 11.4 关键技术要点

| 要点 | 说明 |
|------|------|
| **QRecursiveMutex** | 允许同一线程多次获取锁，避免递归调用死锁 |
| **双重检查锁定 (DCLP)** | 单例模式的线程安全实现 |
| **Qt::QueuedConnection** | 信号槽跨线程安全调用 |
| **锁释放后发射信号** | 避免槽函数中再次获取锁导致死锁 |

---

## 十二、ViewModel 层设计

### 12.1 问题背景

**原有架构问题**：UI 层直接调用 Service 单例获取数据，导致耦合过紧。

### 12.2 架构设计

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
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                        Service 层                                │
└─────────────────────────────────────────────────────────────────┘
```

---

## 十三、面试问答准备

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

## 十四、技术栈总结

| 类别 | 技术 |
|------|------|
| 语言 | C++11 |
| 框架 | Qt 5.15.2 |
| 数据库 | MySQL 8.0 (云服务器) |
| AI API | 通义千问 (Qwen) + Qwen-Image (图像生成) |
| 存储 | 本地文件 / HTTP 远程文件服务 |
| 构建 | qmake |
| 版本控制 | Git |

---

## 十五、在线演示方案设计

### 15.1 技术选型对比

| 方案 | 画质 | 浏览器访问 | 改动代码 | 选择 |
|------|------|-----------|---------|------|
| noVNC | ⭐⭐ | ✅ | ❌ | ❌ 画质差 |
| TurboVNC + noVNC | ⭐⭐⭐ | ✅ | ❌ | ❌ 有提升但有限 |
| **Guacamole** | ⭐⭐⭐⭐ | ✅ | ❌ | ✅ 选择 |
| Qt WebAssembly | ⭐⭐⭐⭐⭐ | ✅ | ✅ 大改 | ❌ 改动量大 |

### 15.2 架构设计

```
面试官浏览器
      │
      │ HTTP/WebSocket
      ▼
Apache Guacamole (Docker)
      │
      │ RDP 协议
      ▼
xrdp 服务 → XFCE 桌面 → QingNingComic Qt 应用
```

### 15.3 为什么 RDP 画质优于 VNC

| 对比项 | VNC | RDP |
|--------|-----|-----|
| 传输内容 | 像素块 | 图形指令 + 像素 |
| 文字渲染 | 模糊 | 清晰 |
| 带宽效率 | 较低 | 较高 |

---

## 十六、开发问题与解决方案

### 16.1 BibleItem 稳定 ID 修复

**问题**：Bible 组件仅使用 `name` 作为标识，同名角色会导致数据错改。

**解决方案**：添加稳定 ID，使用 `getCharacterById()` 精确查询。

### 16.2 批量取消功能修复

**问题**：`QtConcurrent::run()` 返回值丢失，导致无法取消任务。

**解决方案**：保存 `QFuture` 返回值，取消时重置所有状态标志。

### 16.3 单例模式的坑点

**问题**：使用 `new` 创建新实例，而其他代码使用 `instance()` 获取另一个实例。

**解决方案**：始终使用 `instance()` 获取单例，再调用 `init()` 初始化。

### 16.4 角色一致性问题

**问题**：面板生成的人物与角色圣经中的人物外观不一致。

**解决方案**：使用 `image2image` API，将角色肖像作为源图像，保持一致性。

### 16.5 缓存一致性问题

**问题**：角色/场景图片重新生成后，UI 没有更新显示新图片。

**解决方案**：更新数据库后使缓存失效，发出信号通知 UI 更新。

### 16.6 场景重复/数量偏多问题

#### 问题描述

在 AI 分析小说生成分镜时，发现场景数量异常偏多。以《我不是戏神》第一章为例：
- AI 生成了 **15 个场景**
- 实际独立场景只有 **6 个**（暴雨荒径、家门玄关、老式客厅、老旧厨房、梦境舞台、走廊）
- 冗余率高达 **150%**

典型重复案例：
```
场景1: 暴雨荒径
场景5: 暴雨倾盆的荒芜郊野小径，泥泞不堪...  ← 与场景1重复

场景3: 老式客厅
场景7: 客厅全景：煤油灯静止燃烧...          ← 与场景3重复
场景11: 老旧客厅：木质餐桌...              ← 与场景3重复
场景14: 客厅特写：煤油灯盏...              ← 与场景3重复
```

#### 根因分析

**核心问题：把"镜头描述"当成了"场景实体"来存**

具体链路如下：

```
QwenClient::ensureStoryboardShape()
    └── panels 每一格都走 normalizePanel()
            └── extractSceneInfo()
                    └── 如果 background.sceneId 为空
                            ├── 直接拿 setting 当 sceneName
                            └── 再不行就生成 scene_1 / scene_2 ...
```

**问题代码位置：**

| 文件 | 行号 | 问题 |
|------|------|------|
| `QwenClient.cpp` | 843 | `normalizePanel()` 对每个面板都调用 `extractSceneInfo()` |
| `QwenClient.cpp` | 846 | `extractSceneInfo()` 无 sceneId 时用 setting 当 sceneName |
| `SceneExtractor.cpp` | 583 | 按名字查 + 相似度合并，但比对的是"长描述字符串"而非"稳定场景ID" |
| `SceneExtractor.cpp` | 126 | `toScene()` 从 description/layout/landmark 抽 token，动作句子混进场景圣经 |

**数据放大效应：**

第一章实际数据：
- 模型原始 scenes：**4 个**
- 从 12 个面板额外抽取的"面板级场景"：**11 个**
- 总计：**15 个场景**

```
"暴雨荒径" → 原始场景 ✅
"暴雨倾盆的荒芜郊野小径，泥泞不堪..." → 面板级场景 ❌（重复）
"老旧客厅：木质餐桌..." → 面板级场景 ❌（重复）
"客厅特写：煤油灯盏..." → 面板级场景 ❌（重复）
```

**根本原因：**

1. **场景 ID 生成策略有问题**：AI 没有生成稳定的 sceneId，代码 fallback 到用 setting 文本当场景名
2. **面板级场景污染**：每个面板的 setting 描述被当成独立场景存入数据库
3. **合并比对的是长字符串**：`saveScene()` 比对的是"暴雨倾盆的荒芜郊野小径..."这种长描述，而非稳定的场景 ID
4. **动作句子混入场景圣经**：`toScene()` 从描述中抽取 token 时，把动作描述也当成了场景特征

#### 技术调研：业界解决方案

| 领域 | 技术 | 原理 |
|------|------|------|
| **NLP/知识图谱** | 指代消解 (Coreference Resolution) | 找出"他"、"那个地方"指向的实体 |
| **NLP/知识图谱** | 实体链接 (Entity Linking) | 将不同表述映射到同一实体 |
| **AI 图像生成** | Midjourney `--cref` | 绑定参考图作为视觉锚点 |
| **LLM 结构化输出** | OpenAI Structured Outputs | 强制 JSON Schema 约束 |

核心思想：**维护一个实体别名表，将不同表述映射到同一实体**

#### 解决方案

**方案一：源头治理 - 修复 `extractSceneInfo()` 逻辑**

```cpp
// QwenClient.cpp - 修复场景 ID 生成策略
QVariantMap QwenClient::extractSceneInfo(const QVariantMap& panel)
{
    QVariantMap background = panel["background"].toMap();
    QString sceneId = background["sceneId"].toString();
    
    // 如果 AI 没有生成 sceneId，不要直接用 setting 当场景名
    // 而是返回空，让调用方知道这个面板没有关联场景
    if (sceneId.isEmpty()) {
        // 尝试从 scenes 列表中匹配
        QString setting = panel["setting"].toString();
        sceneId = matchSceneFromList(setting, m_currentScenes);
        if (sceneId.isEmpty()) {
            return QVariantMap();  // 返回空，表示无有效场景
        }
    }
    
    return buildSceneInfo(sceneId, background);
}
```

**方案二：区分"场景实体"和"镜头描述"**

```cpp
// SceneExtractor.cpp - 只保存真正的场景实体
bool SceneExtractor::saveScene(const QString& novelId, const ExtractedScene& extracted)
{
    // 过滤掉面板级场景（长描述、包含动作词汇）
    if (isPanelLevelScene(extracted.name)) {
        LOG_DEBUG("SceneExtractor", QString("Skipping panel-level scene: %1").arg(extracted.name));
        return false;
    }
    
    // 只保存稳定的场景实体
    // ...
}

bool SceneExtractor::isPanelLevelScene(const QString& name)
{
    // 面板级场景特征：描述过长、包含动作词汇
    if (name.length() > 20) return true;  // 长描述通常是镜头描述
    
    static const QStringList actionKeywords = {
        QString::fromUtf8("特写"), QString::fromUtf8("全景"),
        QString::fromUtf8("俯视"), QString::fromUtf8("仰视")
    };
    for (const QString& kw : actionKeywords) {
        if (name.contains(kw)) return true;
    }
    return false;
}
```

**方案三：后处理层 - 相似度检测 + 智能合并（已实现）**

```cpp
// 场景相似度计算
int calculateSceneSimilarity(const QString& name1, const QString& desc1, 
                             const QString& name2, const QString& desc2)
{
    int score = 0;
    
    // 1. 名称关键词匹配
    QStringList keywords1 = name1.split(QRegularExpression("[,，、\\s]+"));
    QStringList keywords2 = name2.split(QRegularExpression("[,，、\\s]+"));
    // 计算关键词重叠度...
    
    // 2. 场景关键词检测（客厅、厨房、卧室、走廊等）
    static const QStringList sceneKeywords = {
        QString::fromUtf8("客厅"), QString::fromUtf8("厨房"), 
        QString::fromUtf8("卧室"), QString::fromUtf8("走廊")
    };
    for (const QString& keyword : sceneKeywords) {
        if (name1.contains(keyword) && name2.contains(keyword)) {
            score += 15;
        }
    }
    
    return qMin(score, 100);
}

// 保存场景时检测相似度
bool SceneExtractor::saveScene(const QString& novelId, const ExtractedScene& extracted)
{
    // 遍历已有场景，检测相似度
    for (const Scene& existingScene : existingScenes) {
        if (shouldMergeScenes(scene.name(), scene.description(),
                              existingScene.name(), existingScene.description())) {
            // 相似场景自动合并
            Scene merged = mergeScenes(existingScene, extracted);
            return updateScene(merged);
        }
    }
    // 无相似场景则新建
}
```

**方案对比：**

| 方案 | 优点 | 缺点 | 推荐度 |
|------|------|------|--------|
| 源头治理 | 从根本解决问题 | 需要修改 AI 调用逻辑 | ⭐⭐⭐⭐⭐ |
| 区分场景/镜头 | 逻辑清晰 | 需要定义判断规则 | ⭐⭐⭐⭐ |
| 后处理合并 | 兜底保障 | 无法完全避免污染 | ⭐⭐⭐ |

#### 效果验证

| 指标 | 优化前 | 优化后（预期） |
|------|--------|---------------|
| 场景数量 | 15 个（4 原始 + 11 面板级） | 4-6 个（仅保留原始场景） |
| 冗余率 | 275%（15/4-1） | < 20% |
| 面板级场景污染 | 11 个 | 0 个 |

#### 后续规划

1. **源头治理优先**：修复 `extractSceneInfo()` 逻辑，从根本杜绝面板级场景污染
2. **引入场景别名表**：用户可手动标记"老式客厅" = "老旧客厅"
3. **向量相似度**：使用 Embedding 计算场景描述的语义相似度
4. **场景聚类**：对大量场景自动聚类，减少人工干预

### 16.7 AI 模型输出 DSL 与执行器约定不一致问题

#### 问题描述

**现象**：AI 模型（Qwen）解析用户自然语言后生成的 DSL JSON 字段名不稳定，与代码期望的格式不一致。

**典型场景**：

用户输入："把第一个面板的背景换成森林"

AI 可能输出：
```json
{
  "scopeType": "panel",      // 不是 scope
  "changeType": "art",       // 不是 type
  "target": "panel-123",     // 不是 targetId
  "operations": [            // 不是 ops
    { "operation": "bg_swap", "parameters": { "prompt": "森林" } }
  ]
}
```

而 Schema 定义的标准格式是：
```json
{
  "scope": "panel",
  "type": "art",
  "targetId": "panel-123",
  "ops": [
    { "action": "bg_swap", "params": { "prompt": "森林" } }
  ]
}
```

#### 根因分析

| 原因 | 说明 |
|------|------|
| AI 输出不稳定 | 同样的 prompt，AI 可能输出不同的字段名 |
| Prompt 约束不够强 | AI 没有严格按照 JSON Schema 输出 |
| 模型版本差异 | 不同版本的模型输出格式可能不同 |

#### 解决方案：多字段名兼容

```cpp
// ChangeRequest.h - 兼容多种字段名
struct ChangeRequestOp {
    QString action;
    QJsonObject params;
    
    static ChangeRequestOp fromJson(const QJsonObject& json) {
        ChangeRequestOp op;
        
        // action / operation / name 三种可能
        op.action = json.value("action").toString(
            json.value("operation").toString(
                json.value("name").toString()));
        
        // params / parameters / args 三种可能
        if (json.contains("params")) {
            op.params = json.value("params").toObject();
        } else if (json.contains("parameters")) {
            op.params = json.value("parameters").toObject();
        } else if (json.contains("args")) {
            op.params = json.value("args").toObject();
        }
        return op;
    }
};

struct ChangeRequestDsl {
    QString scope;
    QString type;
    QString targetId;
    
    static ChangeRequestDsl fromJson(const QJsonObject& json) {
        ChangeRequestDsl dsl;
        
        // scope / scopeType / changeScope / scope_type 四种可能
        dsl.scope = source.value("scope").toString(
            source.value("scopeType").toString(
                source.value("changeScope").toString(
                    source.value("scope_type").toString())));
        
        // type / changeType / operationType / typeName 四种可能
        dsl.type = source.value("type").toString(
            source.value("changeType").toString(
                source.value("operationType").toString(
                    source.value("typeName").toString())));
        
        // targetId / target_id / target 三种可能
        dsl.targetId = source.value("targetId").toString(
            source.value("target_id").toString(
                source.value("target").toString()));
        
        // ops / operations / actions 三种可能
        QJsonArray opsArray;
        if (source.contains("ops")) {
            opsArray = source.value("ops").toArray();
        } else if (source.contains("operations")) {
            opsArray = source.value("operations").toArray();
        } else if (source.contains("actions")) {
            opsArray = source.value("actions").toArray();
        }
        
        return dsl;
    }
};
```

#### 设计要点

| 要点 | 说明 |
|------|------|
| **多字段名兼容** | 识别多种可能的字段名，按优先级尝试 |
| **嵌套结构处理** | 处理 `dsl`、`change_request` 等嵌套层级 |
| **默认值兜底** | 字段缺失时提供合理的默认值 |
| **日志记录** | 记录解析过程，便于调试 |

#### 面试问答

**Q: 为什么 AI 模型输出的格式不稳定？**

A: 大语言模型本质上是概率模型，同样的 prompt 可能产生不同的输出。虽然我们通过 JSON Schema 约束输出格式，但模型仍可能使用同义词或变体字段名。

**Q: 为什么不在 Prompt 中更严格地约束？**

A: 我们尝试过，但：
1. 过于严格的约束可能影响模型的推理能力
2. 不同模型版本对指令的遵循程度不同
3. 兼容性方案更健壮，能适应模型升级

**Q: 这种兼容方案有什么缺点？**

A: 主要缺点是代码复杂度增加。但我们通过封装 `fromJson()` 方法，将兼容逻辑集中在一处，不影响业务代码的可读性。

---

*文档版本: 5.3*  
*最后更新: 2026年*

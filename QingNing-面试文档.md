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

### 6.6 配置说明

```ini
# config.ini
[storage]
type = http
endpoint = http://101.33.201.174:8080
dataDir = /data/comic
```

### 6.7 部署步骤

```bash
# 1. 安装 Python 和 Flask
yum install -y python3
pip3 install flask

# 2. 创建存储目录
mkdir -p /data/comic/{panels,characters,scenes,exports}

# 3. 启动文件服务
nohup python3 /data/upload_server.py &

# 4. 开放防火墙端口
firewall-cmd --add-port=8080/tcp --permanent
firewall-cmd --reload

# 5. 腾讯云控制台开放 8080 端口
```

### 6.8 与对象存储对比

| 对比项 | 腾讯云 COS | HTTP 文件服务 |
|--------|-----------|---------------|
| 成本 | 按量付费 | 免费（使用服务器磁盘） |
| 部署 | 无需部署 | 需要部署 Flask |
| 扩展性 | 自动扩展 | 受服务器磁盘限制 |
| CDN | 支持 | 不支持 |
| 适用场景 | 大规模生产 | 个人项目/演示 |

### 6.9 设计要点

| 要点 | 说明 |
|------|------|
| **简单优先** | Flask 服务仅 20 行代码 |
| **利用现有资源** | 复用云服务器磁盘空间 |
| **统一接口** | StorageClient 抽象层，可随时切换实现 |
| **安全考虑** | 可添加 Token 认证 |

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

### 6.2 内存管理

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

## 九、任务队列系统设计

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

### 9.3 架构设计

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

### 9.4 核心代码实现

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

### 9.5 与原仓库 AWS SQS 对比

| 特性 | AWS SQS (原仓库) | TaskQueue (当前) |
|------|------------------|------------------|
| 部署环境 | AWS 云服务 | 本地内存 |
| 持久化 | AWS 托管 | MySQL 数据库 |
| 扩展性 | 自动扩展 | 固定 Worker 数量 |
| 适用场景 | 多用户分布式 | 单用户桌面 |
| 成本 | 按量付费 | 免费 |

---

## 十、多线程崩溃问题排查与修复

### 10.1 问题描述

**现象**: 点击"面板批量生成"按钮后，程序在生成第一张面板图片后立即崩溃。

### 10.2 问题分析

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

### 10.4 解决方案

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

### 10.5 关键技术要点

| 要点 | 说明 |
|------|------|
| **QRecursiveMutex** | 允许同一线程多次获取锁，避免递归调用死锁 |
| **双重检查锁定 (DCLP)** | 单例模式的线程安全实现 |
| **Qt::QueuedConnection** | 信号槽跨线程安全调用 |
| **锁释放后发射信号** | 避免槽函数中再次获取锁导致死锁 |
| **统一数据库访问入口** | 所有数据库操作通过 DatabaseManager |

---

## 十、ViewModel 层设计

### 11.1 问题背景

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

### 11.4 核心代码实现

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

## 十三、技术栈总结

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

## 十四、在线演示方案设计

### 14.1 问题背景

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

### 14.4 架构设计

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

### 14.5 为什么 RDP 画质优于 VNC

| 对比项 | VNC | RDP |
|--------|-----|-----|
| 传输内容 | 像素块 | 图形指令 + 像素 |
| 压缩方式 | JPEG/Tight | RemoteFX/Negotiate |
| 文字渲染 | 模糊 | 清晰 |
| 带宽效率 | 较低 | 较高 |
| 延迟 | 较高 | 较低 |

**核心差异**: RDP 传输的是"画一个矩形"这类图形指令，而非原始像素，因此更高效、画质更好。

### 14.6 部署方案 (CentOS)

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

### 14.7 服务器资源配置

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

### 14.8 简历展示方式

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

### 14.9 方案局限性

| 局限 | 说明 | 应对 |
|------|------|------|
| 画质有损 | 网络传输压缩 | 录制视频作为补充 |
| 依赖网络 | 需要稳定网络 | 提供视频备选 |
| 资源占用 | 服务器需要图形界面 | 4核4G 足够 |

### 14.10 后续规划

| 阶段 | 内容 |
|------|------|
| 短期 | 完善演示账号权限，限制操作范围 |
| 中期 | 添加访问日志，了解面试官行为 |
| 长期 | 考虑 Qt WebAssembly 方案
实现原生画质 |

---

## 十五、开发问题与解决方案（2025年更新）

### 15.1 BibleItem 稳定 ID 修复

#### 问题描述

Bible 组件（角色/场景圣经条目）仅使用 `name` 作为标识，存在数据错改风险：

```
问题场景：
┌─────────────────────────────────────────────────────────────┐
│  用户有两个同名角色 "小明"                                    │
│                                                             │
│  BibleItem A (小明_1) ──┬── 发出 dataChanged("小明", ...)   │
│                         │                                   │
│  BibleItem B (小明_2) ──┘                                   │
│                                                             │
│  页面收到信号后用 getCharacterByName("小明") 查询            │
│         ↓                                                   │
│  返回第一个匹配的角色（可能是错误的那个！）                   │
│         ↓                                                   │
│  数据被错误修改到另一个角色上                                │
└─────────────────────────────────────────────────────────────┘
```

#### 问题代码位置

| 文件 | 行号 | 问题 |
|------|------|------|
| BibleItem.cpp | 130, 149, 655, 817, 868 | emit 信号传递 m_name |
| NovelDetailPage.cpp | 2344, 2431, 2463 | 使用 name 查询数据库 |
| CharacterExtractor.cpp | 404 | getCharacterByName 可能返回错误记录 |

#### 解决方案

**1. BibleItem 添加稳定 ID**

```cpp
// BibleItem.h
class BibleItem : public QFrame
{
public:
    void setItemId(const QString &id);  // 设置稳定 ID
    QString getItemId() const { return m_itemId; }

signals:
    void editClicked(const QString &id, BibleType type);      // 改为传递 ID
    void dataChanged(const QString &id, const QStringList &details);
    void uploadClicked(const QString &id, BibleType type);
    void deleteImageClicked(const QString &id, BibleType type);

private:
    QString m_itemId;  // 稳定 ID（用于数据库查询）
};
```

**2. 创建 BibleItem 时设置 ID**

```cpp
// NovelDetailPage.cpp - populateCharacterBible
for (const Character& character : characters) {
    BibleItem *item = new BibleItem(character.name(), details, BibleType::Character);
    item->setItemId(character.id());  // 设置稳定 ID
    // ...
}

// NovelDetailPage.cpp - populateSceneBible
for (const Scene& scene : scenes) {
    BibleItem *item = new BibleItem(scene.name(), details, BibleType::Scene);
    item->setItemId(scene.sceneId());  // 设置稳定 ID
    // ...
}
```

**3. 信号处理器使用 ID 查询**

```cpp
// 修复前：使用 name 查询，可能返回错误记录
Character character = CharacterExtractor::instance()->getCharacterByName(novelId, name);

// 修复后：使用 ID 精确查询
Character character = CharacterExtractor::instance()->getCharacterById(id);
```

#### 修复效果

| 场景 | 修复前 | 修复后 |
|------|--------|--------|
| 同名角色编辑 | 可能修改错误的角色 | 精确修改目标角色 |
| 同名场景上传图片 | 图片可能关联错误场景 | 图片正确关联目标场景 |
| 数据一致性 | 存在数据错改风险 | ID 唯一标识，无风险 |

### 15.2 批量取消功能修复

#### 问题描述

批量图像生成的取消功能存在两个严重问题：

**问题一：m_batchFuture 未接收返回值**

```cpp
// ImageService.cpp 第151行 - 原代码
QtConcurrent::run(this, &ImageService::runBatchGeneration);  // 返回值丢失！

// cancelCurrentBatch() 中检查
if (m_batchFuture.isRunning()) {  // 永远为 false，因为 m_batchFuture 是空的
    m_batchFuture.cancel();
}
```

**问题二：m_generating 未重置**

```cpp
// 原代码 - 缺少状态重置
void ImageService::cancelCurrentBatch()
{
    m_batchCancelled = true;
    // m_generating 没有重置为 false！
}

// runBatchGeneration 循环检查
while (!m_batchCancelled && ...) {  // 但 isGenerating() 返回 m_generating
    // UI 层可能认为还在生成中
}
```

#### 解决方案

```cpp
void ImageService::generatePanelImages(const QStringList& panelIds, GenerateMode mode)
{
    // ...
    startBatch(panelIds.size());
    
    // 修复：保存 QFuture 返回值
    m_batchFuture = QtConcurrent::run(this, &ImageService::runBatchGeneration);
}

void ImageService::cancelCurrentBatch()
{
    QMutexLocker locker(&m_mutex);
    m_batchCancelled = true;
    m_pendingPanelIds.clear();
    m_currentProcessIndex = 0;
    
    // 修复：现在可以正确取消运行中的任务
    if (m_batchFuture.isRunning()) {
        m_batchFuture.cancel();
    }
    
    // 修复：重置生成状态
    m_generating = false;
    m_batchCount = 0;
    
    emit batchGenerationCancelled();
}
```

#### 技术要点

| 要点 | 说明 |
|------|------|
| **QFuture 作用** | 用于控制和监视异步任务，支持 cancel()、isRunning() 等 |
| **QtConcurrent::run 返回值** | 必须保存才能控制任务 |
| **状态一致性** | 取消时必须重置所有相关状态标志 |
| **信号通知** | 发出 batchGenerationCancelled 通知 UI 更新 |

### 15.3 单例模式的坑点与修复

#### 问题描述

项目中大量使用了单例模式，但在初始化时出现了严重问题：

```
// 错误的初始化方式
QwenClient* client = new QwenClient();  // 创建了新实例
client->init(config);

// 但其他地方使用的是
QwenClient::instance();  // 这是另一个未初始化的实例！
```

#### 根本原因

单例模式要求**全局只有一个实例**，但 `initService` 模板函数使用 `new` 创建了新实例，而其他代码使用 `instance()` 获取的是另一个实例。

#### 正确做法

```cpp
// 正确的单例初始化
QwenClient* client = QwenClient::instance();  // 获取单例
client->init(config);  // 初始化同一个实例
```

#### 经验总结

| 问题 | 声明 | 解决方案 |
|------|------|---------|
| 单例初始化 | 使用 `new` 创建新实例 | 使用 `instance()` 获取单例 |
| 头文件保护符冲突 | 两个文件同名保护符 | 使用带命名空间的保护符 |
| 缓存不一致 | 数据库更新后缓存未更新 | 更新后调用 `invalidate` |

### 14.4 BibleItem 空指针崩溃修复

#### 问题描述

点击作品进入详情页时，程序崩溃。崩溃发生在 `BibleItem` 构造函数中。

#### 根本原因

`BibleItem` 构造函数调用链：

```
BibleItem() 
  → setupUI()
  → setDetails(details)
    → populateEditorData()
      → populateCharacterEditorData()  // 访问 m_genderCombo 等成员
```

问题是：`m_genderCombo`、`m_ageSpin` 等编辑器控件是在 `createCharacterEditorCard()` 中创建的，而这个函数只有在用户点击"编辑"按钮时才会被调用。

**所以在构造函数阶段，这些成员变量都是 nullptr，导致崩溃！**

#### 问题代码

```cpp
void BibleItem::populateCharacterEditorData()
{
    if (m_details.isEmpty()) return;
    
    // 没有检查 m_genderCombo 是否为空！
    int index = m_genderCombo->findText(gender);  // 崩溃！
    m_ageSpin->setValue(age);  // 崩溃！
}
```

#### 解决方案

在 `populateCharacterEditorData()` 和 `populateSceneEditorData()` 中添加空指针检查：

```cpp
void BibleItem::populateCharacterEditorData()
{
    // 编辑器控件可能还未创建，需要检查空指针
    if (m_details.isEmpty()) return;
    if (!m_genderCombo || !m_ageSpin || !m_hairColorEdit || !m_eyeColorEdit) return;
    
    // 安全访问成员变量...
}

void BibleItem::populateSceneEditorData()
{
    // 编辑器控件可能还未创建，需要检查空指针
    if (m_details.isEmpty()) return;
    if (!m_sceneNameEdit || !m_sceneDescEdit) return;
    
    // 安全访问成员变量...
}
```

#### 经验总结

| 问题 | 原因 | 解决方案 |
|------|------|---------|
| 构造函数中访问未初始化成员 | 编辑器控件延迟创建 | 添加空指针检查 |
| 调用顺序错误 | `setDetails` 在控件创建前被调用 | 使用惰性初始化或空指针保护 |

### 15.5 角色一致性问题

#### 问题描述

面板生成的人物与角色圣经中的人物外观不一致。

#### 原仓库方案（Google Imagen 3）

```
角色圣经生成 → 图片保存到 S3/GCS
                    ↓
面板生成时 → 获取角色圣经图片作为参考图
                    ↓
          → 将参考图传给 Google Imagen API
                    ↓
          → Imagen 根据参考图生成一致的角色
```

#### 阿里云方案限制

阿里云 `wanx` 系列文生图模型**不支持参考图输入**，与 Google Imagen 3 不同。

#### 我们的解决方案

使用 `image2image` API 实现类似功能：

```
角色圣经生成角色肖像
        ↓
面板生成时，        ↓
使用 image2image API，将角色肖像作为源图像
        ↓
用 prompt 描述新场景
        ↓
生成与角色圣经一致的面板图片
```

#### 代码实现

```cpp
// 1. 收集角色肖像路径
for (const Character& ch : characters) {
    if (!ch.portraitPath().isEmpty()) {
        ctx.referenceImages.append(ch.portraitPath());
    }
}

// 2. 使用 image2image API
if (!ctx.referenceImages.isEmpty()) {
    // 读取参考图片
    QByteArray refImageData = readFile(ctx.referenceImages.first());
    
    // 使用 edit API 而不是 generate API
    QwenImageClient::EditOptions options;
    options.prompt = ctx.prompt;
    options.sourceImage = refImageData;
    
    QwenImageClient::instance()->edit(options);
}
```

### 15.6 场景圣经不出现人物

#### 问题描述

场景圣经图片中出现了不应该出现的人物。

#### 解决方案

在 prompt 中添加正向和负向提示词：

```cpp
// 正向提示词
parts << "empty scene, no characters";

// 负向提示词
QString sceneNegative = DEFAULT_NEGATIVE_PROMPT + ", "
    "no people, no characters, no humans, no figures, "
    "empty scene, uninhabited, no faces, no bodies";
```

### 15.7 缓存一致性问题

#### 问题描述

角色/场景图片重新生成后，UI 没有更新显示新图片。

#### 原因分析

1. 数据库更新了 ✅
2. 缓存没有更新 ❌
3. UI 没有收到更新通知 ❌

#### 解决方案

```cpp
// 1. 更新数据库后，使缓存失效
BibleCache::instance()->invalidateCharacters(novelId);

// 2. 发出信号通知 UI 更新
emit characterUpdated(characterId, portraitPath);

// 3. UI 连接信号刷新显示
connect(CharacterExtractor::instance(), &CharacterExtractor::characterUpdated,
        this, [this]() { refreshBibleUI(); });
```

### 15.8 技术选型对比

| 方面 | 原仓库 (Google Imagen 3) | 我们的方案 (阿里云通义万相) |
|------|-------------------------|---------------------------|
| 参考图支持 | ✅ 原生支持 | ⚠️ 需要 image2image 模拟 |
| 角色一致性 | 参考图直接注入 | 源图像 + prompt |
| API 复杂度 | 简单 | 稍复杂（需要判断是否有参考图） |
| 效果 | 更精确 | 依赖 prompt 质量 |

### 15.9 代码质量改进

#### PromptBuilder 统一

原有两个 `SingletonUtils.h` 文件，保护符冲突导致编译错误：

```
include/SingletonUtils.h        // 保护符: SINGLETONUTILS_H
include/utils/SingletonUtils.h  // 保护符: SINGLETONUTILS_H (冲突!)
```

解决方案：
1. 统一到 `include/utils/SingletonUtils.h`
2. 修改保护符为 `UTILS_SINGLETONUTILS_H`
3. 删除旧文件，更新引用

#### 滚动条样式统一

所有弹窗的滚动条样式统一使用 `EditorStyles::scrollBarStyle()`。

### 15.10 图像服务层重构

#### 重构前的问题

| 问题类型 | 具体问题 | 影响 |
|---------|---------|------|
| **重复代码** | `generateImage` 和 `generateWithReference` 重试逻辑相同 | 维护成本高 |
| **函数过长** | `buildPromptForPanel` 约100行 | 可读性差 |
| **过度嵌套** | 多层循环和条件判断 | 难以理解 |
| **职责不清** | 角色数据转换散落在主流程中 | 耦合度高 |

#### 重构方案

**1. 提取角色数据转换函数**

```cpp
// 重构前：角色转换逻辑散落在 buildPromptForPanel 中
for (const Character& ch : characters) {
    QJsonObject charJson;
    charJson["id"] = ch.id();
    // ... 20多行转换代码
    characterRefs[ch.name()] = charJson;
}

// 重构后：提取为独立函数
QJsonObject ImageService::buildCharacterRef(const Character& ch, QStringList& outPortraitPaths)
{
    QJsonObject charJson;
    charJson["id"] = ch.id();
    charJson["name"] = ch.name();
    // ... 清晰的转换逻辑
    return charJson;
}

QMap<QString, QJsonObject> ImageService::fetchCharacterRefs(const QString& novelId, QStringList& outReferenceImages)
{
    QMap<QString, QJsonObject> characterRefs;
    QList<Character> characters = CharacterExtractor::instance()->getCharactersByNovel(novelId);
    for (const Character& ch : characters) {
        characterRefs[ch.name()] = buildCharacterRef(ch, outReferenceImages);
    }
    return characterRefs;
}
```

**2. 合并图像生成方法**

```cpp
// 重构前：两个方法有大量重复代码
bool generateImage(GenerationContext& ctx);      // text2image
bool generateWithReference(GenerationContext& ctx); // image2image

// 重构后：统一入口 + 核心方法
bool generateImage(GenerationContext& ctx)
{
    QByteArray refImageData;
    if (!ctx.referenceImages.isEmpty()) {
        // 加载参考图片
        refImageData = loadReferenceImage(ctx.referenceImages.first());
    }
    return executeImageGeneration(ctx, refImageData);
}

bool executeImageGeneration(GenerationContext& ctx, const QByteArray& refImageData)
{
    bool useRefImage = !refImageData.isEmpty();
    
    // 统一的重试逻辑
    return m_apiRetryPolicy->executeWithRetry([&]() {
        if (useRefImage) {
            return QwenImageClient::instance()->edit(...);
        } else {
            return QwenImageClient::instance()->generate(...);
        }
    }, ...);
}
```

#### 重构效果

| 指标 | 重构前 | 重构后 | 改善 |
|------|--------|--------|------|
| `buildPromptForPanel` 行数 | ~100行 | ~50行 | -50% |
| 重复代码行数 | ~80行 | ~10行 | -87% |
| 函数数量 | 2个生成方法 | 1个入口+1个核心 | 更清晰 |
| 可测试性 | 低 | 高 | 可单独测试转换函数 |

#### 重构原则应用

1. **单一职责原则**：每个函数只做一件事
2. **DRY 原则**：消除重复的重试逻辑
3. **函数提取**：将复杂逻辑拆分为小函数
4. **命名清晰**：`buildCharacterRef`、`fetchCharacterRefs`、`executeImageGeneration`

### 15.11 删除照片界面跳动问题修复

#### 问题描述

点击圣经里的删除照片选项时，界面会跳动/闪烁。

#### 问题分析

**第一层原因**：原代码在删除图片后调用 `refreshBibleUI()`

```cpp
void NovelDetailPage::onBibleItemDeleteImageClicked(const QString &id, BibleType type)
{
    // 删除图片文件并更新数据库
    // ...
    
    refreshBibleUI();  // 问题所在！
}
```

`refreshBibleUI()` 会清空并重建所有 Bible 控件，导致界面跳动。

**第二层原因（根本原因）**：信号触发二次刷新

```cpp
// NovelDetailPage 初始化时连接的信号
connect(CharacterExtractor::instance(), &CharacterExtractor::characterUpdated,
        this, [this](const QString& characterId, const QString& portraitPath) {
            QTimer::singleShot(100, this, [this]() {
                refreshBibleUI();  // 100ms 后再次重建 UI！
            });
        }, Qt::QueuedConnection);
```

当调用 `updateCharacter()` 时：
1. 数据库更新成功
2. 发出 `characterUpdated` 信号
3. 信号触发 `refreshBibleUI()`
4. 界面跳动

**问题链路**：
```
删除图片 → updateCharacter() → emit characterUpdated → refreshBibleUI() → 界面跳动
```

#### 解决方案

**方案一**：直接更新 BibleItem（已实现）

```cpp
void NovelDetailPage::onBibleItemDeleteImageClicked(const QString &id, BibleType type)
{
    // 找到对应的 BibleItem
    BibleItem *targetItem = findBibleItemById(id, type);
    
    // 删除图片并更新数据库
    // ...
    
    // 直接更新图片显示，不重建 UI
    targetItem->setImage(QString());
}
```

**方案二**：修改信号处理逻辑（根本解决）

```cpp
// 新增方法：直接更新特定 BibleItem 的图片
void NovelDetailPage::updateBibleItemImage(const QString& id, const QString& imagePath, BibleType type)
{
    QWidget *container = (type == BibleType::Character) ? m_characterBibleContainer : m_sceneBibleContainer;
    if (!container) return;
    
    QList<BibleItem*> bibleItems = container->findChildren<BibleItem*>();
    for (BibleItem *item : bibleItems) {
        if (item->getItemId() == id) {
            item->setImage(imagePath);
            break;
        }
    }
}

// 修改信号连接，直接更新而非重建
connect(CharacterExtractor::instance(), &CharacterExtractor::characterUpdated,
        this, [this](const QString& characterId, const QString& portraitPath) {
            updateBibleItemImage(characterId, portraitPath, BibleType::Character);
        }, Qt::QueuedConnection);

connect(SceneExtractor::instance(), &SceneExtractor::sceneUpdated,
        this, [this](const QString& sceneId, const QString& referenceImagePath) {
            updateBibleItemImage(sceneId, referenceImagePath, BibleType::Scene);
        }, Qt::QueuedConnection);
```

**同时修改 SceneExtractor**：让信号传递正确的参数

```cpp
// SceneExtractor.cpp
if (success) {
    // 传递 sceneId 和 referenceImagePath，而非 id 和 name
    emit sceneUpdated(scene.sceneId(), scene.referenceImagePath());
}
```

#### 技术要点

| 要点 | 说明 |
|------|------|
| **信号参数选择** | 信号应传递 UI 需要的数据（如图片路径），而非无关数据（如名称） |
| **局部更新 vs 全局刷新** | 只更新受影响的控件，避免重建整个 UI |
| **findChildren 遍历** | Qt 提供的控件查找方法，可按类型遍历子控件 |
| **稳定 ID 匹配** | 使用 `getItemId()` 精确匹配目标控件 |
| **消除冗余刷新** | 同一操作不应触发多次 UI 刷新 |

#### 修复效果

| 场景 | 修复前 | 修复后 |
|------|--------|--------|
| 删除照片 | 界面跳动/闪烁 | 平滑更新，无跳动 |
| 上传照片 | 界面跳动 | 平滑更新 |
| 编辑角色/场景 | 界面跳动 | 平滑更新 |
| 滚动位置 | 可能丢失 | 保持不变 |
| 其他条目 | 被重建（视觉跳动） | 不受影响 |

#### 设计原则

1. **最小化更新范围**：只更新必要的 UI 元素
2. **信号参数设计**：传递接收方真正需要的数据
3. **避免链式刷新**：一个操作不应触发多次 UI 重建
4. **利用现有 API**：BibleItem 已有 `setImage()` 方法，直接复用

### 15.12 面板幅面比例验证功能

#### 问题背景

图像生成 API 对不同幅面比例（1:1、3:2、16:9 等）的支持程度不同，需要验证生成的图片实际尺寸是否符合预期的宽高比。例如：
- 火山引擎 API 设置 `width=1584, height=1056`（3:2），但返回的图片是否真的是 1584x1056？
- 不同预设模式下，图片幅面比例是否正确？

#### 设计思路

**核心问题**：如何在面板预览界面直观地展示生成图片的实际尺寸和宽高比？

**技术选型考虑**：
1. **方案一：纯日志验证** — 只在控制台输出尺寸信息，不够直观
2. **方案二：独立测试窗口** — 需要额外 UI，操作繁琐
3. **方案三：嵌入 PanelCard** — 在面板卡片上直接显示尺寸标签，最直观

选择方案三，在 PanelCard 预览图下方添加尺寸标签，图片生成后自动显示 `宽x高 (比例)` 信息。

#### 实现方案

**1. 宽高比简化算法**

使用辗转相除法（欧几里得算法）求最大公约数，将尺寸化简为最简比：

```cpp
QString simplifyRatio(int w, int h)
{
    if (w <= 0 || h <= 0) return "?";
    int a = w, b = h;
    while (b != 0) {
        int t = b;
        b = a % b;
        a = t;
    }
    return QString("%1:%2").arg(w / a).arg(h / a);
}
```

例如：`1584x1056` → GCD=528 → `3:2`，`1664x936` → GCD=104 → `16:9`

**2. PanelCard 尺寸标签**

在预览图下方添加 `m_sizeLabel`，默认隐藏，图片生成后显示：

```cpp
m_sizeLabel = new QLabel();
m_sizeLabel->setAlignment(Qt::AlignCenter);
m_sizeLabel->setStyleSheet("font-size: 10px; color: #9CA3AF;");
m_sizeLabel->hide();

void PanelCard::setImageSize(int width, int height)
{
    QString ratio = simplifyRatio(width, height);
    m_sizeLabel->setText(QString("%1x%2 (%3)").arg(width).arg(height).arg(ratio));
    m_sizeLabel->show();
}
```

**3. 数据流**

```
ImageService::finalizeResult()
  → 从 imageData 解析 QImage 获取 width/height
  → GenerateResult { width, height }
  → emit panelGenerated(result)
  → PanelPreviewWidget::onPanelGenerated()
    → card->setImageSize(result.width, result.height)
    → PanelCard 显示 "1584x1056 (3:2)"
```

#### 尺寸配置体系

| 预设模式 | 火山引擎 | 通义万相 | 宽高比 |
|----------|---------|---------|--------|
| Square_1x1 | 1328x1328 | 1024x1024 | 1:1 |
| Standard_3x2 | 1584x1056 | 1536x1024 | 3:2 |
| Widescreen_16x9 | 1664x936 | 1280x720 | 16:9 |
| Preview（默认） | 1056x1584 | 720x1280 | 2:3 |

#### 验证效果

| 场景 | 显示效果 |
|------|---------|
| 3:2 预设 | `1584x1056 (3:2)` |
| 1:1 预设 | `1328x1328 (1:1)` |
| 16:9 预设 | `1664x936 (16:9)` |
| API 返回异常尺寸 | 立即可见，如 `1024x1024 (1:1)` 而非预期的 `1584x1056 (3:2)` |

#### 后续规划

| 阶段 | 内容 |
|------|------|
| 短期 | 添加尺寸与预期不符时的警告高亮（红色标签） |
| 中期 | 在批量生成完成后输出尺寸统计报告 |
| 长期 | 支持自定义幅面比例输入，API 不支持时自动选择最接近的预设 |

---

*文档版本: 4.6*  
*最后更新: 2025年*

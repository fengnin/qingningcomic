# QingNing 编码与文本处理规范

## 一、编码原则

### 1.1 统一编码标准

| 层级 | 编码标准 | 说明 |
|------|----------|------|
| **源码文件** | UTF-8 | 所有 `.cpp`、`.h`、`.json` 文件 |
| **数据库** | utf8mb4 | MySQL 8.0，支持完整 Unicode（包括 Emoji） |
| **日志输出** | UTF-8 | 文件日志和控制台输出 |
| **网络传输** | UTF-8 | API 请求/响应 |
| **配置文件** | UTF-8 | config.ini 等 |

### 1.2 编码流程图

```
外部输入 (可能多种编码)
        │
        ▼
┌─────────────────────────────────┐
│  边界层：统一转码为 UTF-8        │
│  - 文件读取 → Enc::utf8()       │
│  - 数据库 → SET NAMES utf8mb4   │
│  - API 响应 → QString::fromUtf8 │
│  - 用户输入 → QString (Unicode) │
└─────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────┐
│  内部处理：QString (Unicode)    │
│  - 所有字符串使用 QString       │
│  - Qt 内部自动处理 Unicode      │
└─────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────┐
│  输出层：统一编码输出            │
│  - 日志 → UTF-8                 │
│  - 数据库 → utf8mb4             │
│  - 文件 → UTF-8 (可选 BOM)      │
└─────────────────────────────────┘
```

## 二、源码编码规范

### 2.1 中文字符串处理

**推荐方式**：

```cpp
// 方式1：TR 宏（推荐用于普通字符串）
QString msg = TR("数据库连接失败");

// 方式2：TR_FMT 宏（推荐用于格式化字符串）
QString msg = TR_FMT("加载 %1 个文件，成功 %2 个", total, success);

// 方式3：QStringLiteral（推荐用于性能敏感场景）
static const QString ERROR_MSG = QStringLiteral("数据库连接失败");
```

**禁止方式**：

```cpp
// ❌ 禁止：直接使用中文字符串（可能在某些终端乱码）
QString msg = "数据库连接失败";

// ❌ 禁止：使用 fromLocal8Bit（编码不确定）
QString msg = QString::fromLocal8Bit("数据库连接失败");

// ❌ 禁止：使用 Latin1（不支持中文）
QString msg = QString::fromLatin1("数据库连接失败");
```

### 2.2 Unicode 转义（可选）

如果担心源码编码问题，可以使用 Unicode 转义：

```cpp
// 两种方式等价
QString msg1 = TR("数据库");
QString msg2 = QString::fromUtf8("\u6570\u636e\u5e93");
```

### 2.3 JSON 处理

```cpp
// 读取 JSON 数据
QByteArray jsonData = networkReply->readAll();
QJsonDocument doc = QJsonDocument::fromJson(jsonData);  // Qt 自动 UTF-8

// 构建 JSON 数据
QJsonObject obj;
obj["name"] = TR("青柠");  // QString 自动转 JSON UTF-8
QJsonDocument doc(obj);
QByteArray output = doc.toJson(QJsonDocument::Compact);  // 输出 UTF-8
```

## 三、数据库编码规范

### 3.1 连接设置

```cpp
// DatabaseManager.cpp
bool DatabaseManager::connectInternal(...)
{
    // ... 连接数据库 ...
    
    // 必须设置 utf8mb4
    QSqlQuery query(m_database);
    query.exec("SET NAMES utf8mb4");
    query.exec("SET CHARACTER SET utf8mb4");
    query.exec("SET character_set_connection=utf8mb4");
    
    return true;
}
```

### 3.2 表结构定义

```sql
CREATE TABLE novels (
    id VARCHAR(36) PRIMARY KEY,
    title VARCHAR(255) NOT NULL,
    content LONGTEXT,
    -- 必须使用 utf8mb4
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
```

### 3.3 数据读写

```cpp
// 写入：QString 自动转 UTF-8
QVariantMap data;
data["title"] = TR("青柠漫画");
m_db->insert("novels", data);

// 读取：Qt 自动转 QString (Unicode)
QVariantMap result = m_db->selectOne("novels", "id = ?", {id});
QString title = result["title"].toString();  // 已经是 Unicode
```

## 四、文件 I/O 编码规范

### 4.1 文本文件读取

```cpp
// 推荐：使用 QFile + QTextStream
QFile file(path);
if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QTextStream in(&file);
    in.setCodec("UTF-8");  // 显式设置编码
    QString content = in.readAll();
    file.close();
}

// 或者：直接读取字节数组后转换
QFile file(path);
if (file.open(QIODevice::ReadOnly)) {
    QByteArray data = file.readAll();
    QString content = Enc::json(data);  // 使用 Enc::json 统一转换
    file.close();
}
```

### 4.2 文本文件写入

```cpp
QFile file(path);
if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream out(&file);
    out.setCodec("UTF-8");
    out.setGenerateByteOrderMark(false);  // 不生成 BOM
    out << content;
    file.close();
}
```

### 4.3 编码检测

```cpp
// 检测文件是否为有效 UTF-8
if (Enc::isValidUtf8(fileData)) {
    QString content = QString::fromUtf8(fileData);
}

// 检测并移除 BOM
if (Enc::hasBom(fileData)) {
    fileData = Enc::removeBom(fileData);
}
```

## 五、日志编码规范

### 5.1 日志输出

```cpp
// 日志自动处理 UTF-8
LOG_INFO("Database", TR("连接成功"));
LOG_ERROR("API", TR_FMT("请求失败: %1", errorMessage));
```

### 5.2 控制台输出（Windows）

```cpp
// EncodingUtils.cpp - 初始化时设置控制台编码
void EncodingUtils::initSystemEncoding()
{
#ifdef Q_OS_WIN
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}
```

## 六、网络传输编码规范

### 6.1 HTTP 请求

```cpp
QNetworkRequest request(url);
request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=UTF-8");

QJsonObject body;
body["text"] = TR("中文内容");
QByteArray data = QJsonDocument(body).toJson();

QNetworkReply* reply = manager->post(request, data);
```

### 6.2 HTTP 响应处理

```cpp
QByteArray responseData = reply->readAll();
// Qt 的 QJsonDocument::fromJson 自动处理 UTF-8
QJsonDocument doc = QJsonDocument::fromJson(responseData);

// 如果需要手动处理
QString text = QString::fromUtf8(responseData);
```

## 七、编码工具函数速查

| 函数 | 用途 | 示例 |
|------|------|------|
| `TR(str)` | 中文字符串 | `TR("成功")` |
| `TR_FMT(str, args...)` | 格式化中文 | `TR_FMT("共 %1 个", n)` |
| `Enc::utf8(str)` | C字符串转QString | `Enc::utf8("test")` |
| `Enc::json(data)` | JSON数据转QString | `Enc::json(jsonBytes)` |
| `Enc::isValidUtf8(data)` | 验证UTF-8 | `Enc::isValidUtf8(fileData)` |
| `Enc::hasBom(data)` | 检测BOM | `Enc::hasBom(fileData)` |
| `Enc::removeBom(data)` | 移除BOM | `Enc::removeBom(fileData)` |
| `Enc::containsChinese(str)` | 检测中文 | `Enc::containsChinese(text)` |

## 八、常见问题排查

### 8.1 中文乱码排查流程

```
出现乱码
    │
    ├── 检查源码文件编码 → 应为 UTF-8
    │
    ├── 检查数据库编码 → 应为 utf8mb4
    │       │
    │       ├── SHOW CREATE TABLE table_name;
    │       └── SET NAMES utf8mb4;
    │
    ├── 检查日志输出 → 应为 UTF-8
    │       │
    │       └── Windows: SetConsoleOutputCP(CP_UTF8);
    │
    └── 检查文件读写 → 应显式设置 UTF-8
            │
            └── QTextStream::setCodec("UTF-8");
```

### 8.2 常见错误

| 错误 | 原因 | 解决方案 |
|------|------|----------|
| 终端显示乱码 | 终端编码非UTF-8 | `SetConsoleOutputCP(CP_UTF8)` |
| 数据库中文变 `?` | 连接编码错误 | `SET NAMES utf8mb4` |
| JSON解析失败 | 编码不一致 | 使用 `QJsonDocument::fromJson` |
| 文件读取乱码 | 未指定编码 | `QTextStream::setCodec("UTF-8")` |

## 九、.pro 文件配置

```qmake
# 强制源文件编码为 UTF-8
CODECFORSRC = UTF-8
CODECFORTR = UTF-8
QMAKE_DEFAULT_ENCODING = UTF-8

# Windows 编译器编码设置
win32 {
    win32-g++ {
        QMAKE_CXXFLAGS += -fexec-charset=UTF-8
        QMAKE_CXXFLAGS += -finput-charset=UTF-8
    }
    msvc {
        QMAKE_CXXFLAGS += /utf-8
        QMAKE_CXXFLAGS += /source-charset:utf-8
        QMAKE_CXXFLAGS += /execution-charset:utf-8
    }
}
```

## 十、检查清单

- [ ] 所有源文件使用 UTF-8 编码保存
- [ ] 数据库连接执行 `SET NAMES utf8mb4`
- [ ] 表定义使用 `DEFAULT CHARSET=utf8mb4`
- [ ] 中文字符串使用 `TR()` 或 `TR_FMT()` 宏
- [ ] 文件读写显式设置 UTF-8 编码
- [ ] Windows 控制台初始化 UTF-8 编码
- [ ] HTTP 请求头指定 `charset=UTF-8`

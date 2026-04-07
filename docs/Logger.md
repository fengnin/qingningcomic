# QingningComic 日志系统文档

## 概述

QingningComic 使用统一的日志系统 `Logger`，提供以下功能：

- **多级别日志**：Debug、Info、Warning、Error、Fatal
- **双输出通道**：控制台输出 + 文件输出
- **线程安全**：使用互斥锁保护日志操作
- **分类管理**：按模块/类别组织日志
- **级别过滤**：可设置最低输出级别

## 快速开始

### 初始化

在应用程序启动时初始化日志系统：

```cpp
#include "Logger.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 初始化日志系统（使用默认路径）
    Logger::instance()->init();
    
    // ... 应用程序代码 ...
    
    // 关闭日志系统
    Logger::instance()->close();
    return app.exec();
}
```

### 记录日志

使用便捷宏记录日志：

```cpp
#include "Logger.h"

// 调试信息
LOG_DEBUG("Network", "Connecting to server...");

// 一般信息
LOG_INFO("Database", "Connected successfully");

// 警告信息
LOG_WARNING("File", "Config file not found, using defaults");

// 错误信息
LOG_ERROR("Database", QString("Query failed: %1").arg(error));

// 致命错误
LOG_FATAL("System", "Critical initialization failed");
```

## 日志级别

| 级别 | 宏 | 用途 |
|------|-----|------|
| Debug | `LOG_DEBUG` | 调试信息，开发阶段使用 |
| Info | `LOG_INFO` | 一般信息，记录正常操作 |
| Warning | `LOG_WARNING` | 警告信息，潜在问题 |
| Error | `LOG_ERROR` | 错误信息，操作失败 |
| Fatal | `LOG_FATAL` | 致命错误，程序无法继续 |

## 配置选项

### 设置日志级别

```cpp
// 只输出 Warning 及以上级别的日志
Logger::instance()->setLogLevel(LogLevel::Warning);
```

### 控制输出通道

```cpp
// 禁用文件输出
Logger::instance()->setFileOutput(false);

// 禁用控制台输出
Logger::instance()->setConsoleOutput(false);
```

### 自定义日志路径

```cpp
// 使用自定义路径
Logger::instance()->init("C:/logs/qingning.log");
```

## 日志格式

日志输出格式：

```
[时间戳] [级别] [分类] 消息
```

示例输出：

```
[2026-03-09 14:30:15.123] [INFO]  [Database] Connected successfully to qingning_comic
[2026-03-09 14:30:15.456] [ERROR] [NovelService] createNovel: 保存到数据库失败
[2026-03-09 14:30:16.789] [INFO]  [Application] QingningComic starting...
```

## 日志文件位置

默认日志文件路径：

```
Windows: C:/Users/<用户名>/AppData/Local/QingningComic/logs/qingning_YYYYMMDD.log
macOS: ~/Library/Application Support/QingningComic/logs/qingning_YYYYMMDD.log
Linux: ~/.local/share/QingningComic/logs/qingning_YYYYMMDD.log
```

## 最佳实践

### 1. 使用合适的日志级别

```cpp
// ✅ 正确：使用合适的级别
LOG_INFO("Database", "Connection established");
LOG_ERROR("Database", QString("Connection failed: %1").arg(error));

// ❌ 错误：级别使用不当
LOG_ERROR("Database", "Connection established");  // 正常操作不应使用 Error
LOG_DEBUG("Database", "Connection failed");       // 错误不应使用 Debug
```

### 2. 使用有意义的分类

```cpp
// ✅ 正确：使用模块/功能作为分类
LOG_INFO("Database", "Query executed");
LOG_INFO("Network", "Request sent");
LOG_INFO("UI", "Window opened");

// ❌ 错误：分类过于模糊
LOG_INFO("App", "Something happened");
LOG_INFO("Log", "Message");
```

### 3. 包含上下文信息

```cpp
// ✅ 正确：包含相关上下文
LOG_ERROR("NovelService", QString("Failed to create novel '%1': %2").arg(title, error));

// ❌ 错误：缺少上下文
LOG_ERROR("NovelService", "Failed");
```

### 4. 避免敏感信息

```cpp
// ❌ 错误：记录敏感信息
LOG_DEBUG("Auth", QString("User password: %1").arg(password));

// ✅ 正确：脱敏处理
LOG_DEBUG("Auth", "User authenticated successfully");
```

## 项目中的使用

### 已集成日志的模块

| 模块 | 分类 | 说明 |
|------|------|------|
| main.cpp | Application | 应用程序启动/关闭 |
| DatabaseManager | Database | 数据库连接、查询 |
| NovelService | NovelService | 小说 CRUD 操作 |

### 迁移指南

将现有的 `qDebug`/`qInfo`/`qWarning`/`qCritical` 替换为日志宏：

```cpp
// 之前
qDebug() << "Connected to" << host;
qCritical() << "Failed:" << error;

// 之后
LOG_INFO("Database", QString("Connected to %1").arg(host));
LOG_ERROR("Database", QString("Failed: %1").arg(error));
```

## 文件结构

```
comic/
├── include/
│   └── Logger.h          # 日志系统头文件
├── src/
│   └── utils/
│       └── Logger.cpp    # 日志系统实现
└── docs/
    └── Logger.md         # 本文档
```

## API 参考

### Logger 类

```cpp
class Logger
{
public:
    // 获取单例实例
    static Logger* instance();
    
    // 初始化日志系统
    bool init(const QString &logPath = QString());
    
    // 设置日志级别
    void setLogLevel(LogLevel level);
    LogLevel logLevel() const;
    
    // 控制输出通道
    void setFileOutput(bool enabled);
    void setConsoleOutput(bool enabled);
    
    // 日志记录方法
    void debug(const QString &category, const QString &message);
    void info(const QString &category, const QString &message);
    void warning(const QString &category, const QString &message);
    void error(const QString &category, const QString &message);
    void fatal(const QString &category, const QString &message);
    
    // 获取日志文件路径
    QString logFilePath() const;
    
    // 关闭日志系统
    void close();
};
```

### 便捷宏

```cpp
#define LOG_DEBUG(category, message) Logger::instance()->debug(category, message)
#define LOG_INFO(category, message) Logger::instance()->info(category, message)
#define LOG_WARNING(category, message) Logger::instance()->warning(category, message)
#define LOG_ERROR(category, message) Logger::instance()->error(category, message)
#define LOG_FATAL(category, message) Logger::instance()->fatal(category, message)
```

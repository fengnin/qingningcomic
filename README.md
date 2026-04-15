# 青柠漫画 - C++ 版本

> AI 驱动的小说转漫画平台 - C++11 + Qt 5.15.2 实现

## 项目简介

这是原 QingningComic 项目的 C++ 桌面版本，将小说转化为漫画分镜的 AI 辅助创作工具。

### 核心功能

- 小说管理 (上传、编辑、删除)
- AI 文本分析 (Qwen API)
- 分镜脚本生成
- 角色圣经管理
- 面板图片生成 (通义万相 / 火山引擎)
- 图片编辑
- 导出功能 (PDF/Webtoon/Resources)

### 目标用户

| 用户类型 | 核心需求 |
|----------|----------|
| 漫画创作者 | 快速验证剧情、迭代分镜方案 |
| 小说作者 | 世界观可视化、提升读者体验 |
| 内容创作者 | 同人创作、粉丝向内容 |

---

## 技术栈

| 组件 | 技术选型 |
|------|----------|
| 编程语言 | C++11 |
| GUI 框架 | Qt 5.15.2 (LTS) |
| 编译套件 | Desktop_Qt_5_15_2_MinGW_64_bit |
| 构建系统 | qmake |
| 数据库 | MySQL 8.0 |
| AI 服务 | 阿里云 Qwen / 通义万相 / 火山引擎 |

---

## 系统架构

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           青柠漫画系统架构                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                        UI 层 (Qt Widgets)                            │   │
│  │  MainWindow / DashboardPage / NovelPage / NovelDetailPage / ...     │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                      │                                       │
│                                      ▼                                       │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                      ViewModel 层                                    │   │
│  │          NovelViewModel / StoryboardViewModel                        │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                      │                                       │
│                                      ▼                                       │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                        服务层                                        │   │
│  │  NovelService / StoryboardService / AnalysisService / ...           │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                      │                                       │
│                                      ▼                                       │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                      API 客户端层                                    │   │
│  │  QwenClient / QwenImageClient / VolcEngineImageClient / Storage     │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                      │                                       │
│                                      ▼                                       │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                       数据层                                         │   │
│  │              DatabaseManager / FileStorage / SqlBuilder              │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 项目结构

```
comic/
├── QingningComic.pro           # qmake 项目配置
├── PROJECT_STATUS.md           # 项目进度文档
├── DATABASE_DESIGN.md          # 数据库设计文档
├── README.md                   # 本文档
├── config.ini.example          # 配置文件模板
├── config.ini                  # 配置文件（不提交）
├── include/                    # 头文件目录
│   ├── api/                   # API 客户端
│   ├── components/            # UI 组件
│   ├── services/              # 服务层
│   ├── utils/                 # 工具类
│   ├── viewmodels/            # ViewModel
│   └── *.h                    # 其他头文件
├── src/                        # 源文件目录
│   ├── api/                   # API 客户端实现
│   ├── components/            # UI 组件实现
│   ├── data/                  # 数据层实现
│   ├── models/                # 模型实现
│   ├── pages/                 # 页面实现
│   ├── services/              # 服务层实现
│   ├── utils/                 # 工具类实现
│   ├── viewmodels/            # ViewModel 实现
│   ├── widgets/               # 小部件实现
│   └── app/                   # 应用入口
└── build/                      # 构建输出目录
```

---

## 快速开始

### 1. 环境准备

- Qt 5.15.2 (MinGW 64-bit)
- MySQL 8.0

### 2. 配置数据库

```bash
# 连接 MySQL
mysql -u root -p

# 创建数据库
CREATE DATABASE qingning_comic CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE USER 'qingning'@'%' IDENTIFIED BY 'qingning123';
GRANT ALL PRIVILEGES ON qingning_comic.* TO 'qingning'@'%';
FLUSH PRIVILEGES;
```

### 3. 配置应用

复制配置模板并填写：

```bash
cp config.ini.example config.ini
```

编辑 `config.ini`，填写以下配置：

```ini
[database]
host = 192.168.xxx.xxx
port = 3306
database = qingning_comic
username = qingning
password = your_password

[qwen]
apiKey = sk-your-api-key-here
model = qwen-plus

[qwenImage]
generateModel = wanx2.1-t2i-turbo

[volcengine]
accessKey = your-access-key
secretKey = your-secret-key
```

### 4. 编译运行

1. 使用 Qt Creator 打开 `QingningComic.pro`
2. 选择 "Desktop Qt 5.15.2 MinGW 64-bit" 套件
3. 按 `Ctrl+B` 编译项目
4. 按 `Ctrl+R` 运行项目

---

## 核心模块说明

### 数据模型层

| 模型 | 说明 |
|------|------|
| Novel | 小说数据模型 |
| Character | 角色数据模型 |
| Storyboard | 分镜数据模型 |
| Panel | 面板数据模型 |
| Bible | 圣经数据模型 |
| Job | 任务数据模型 |
| Task | 异步任务模型 |
| Scene | 场景数据模型 |

### API 客户端层

| 客户端 | 说明 |
|--------|------|
| QwenClient | Qwen 文本分析 API |
| QwenImageClient | 通义万相图像生成 API |
| VolcEngineImageClient | 火山引擎图像生成 API |
| StorageClient | S3 兼容存储客户端 |

### 服务层

| 服务 | 说明 |
|------|------|
| NovelService | 小说 CRUD 服务 |
| StoryboardService | 分镜 CRUD 服务 |
| AnalysisService | 小说分析服务 |
| BibleGenerator | 圣经生成服务 |
| ImageService | 图片服务 |
| ExportService | 导出服务 |

---

## AI 服务配置

### 阿里云 Qwen (文本分析)

1. 访问 [阿里云百炼](https://dashscope.console.aliyun.com/)
2. 获取 API Key
3. 在 `config.ini` 中配置

### 通义万相 (图像生成)

1. 使用与 Qwen 相同的 API Key
2. 支持模型：
   - `wanx2.1-t2i-turbo` - 快速版
   - `wanx2.1-t2i-plus` - 高质量版

### 火山引擎 (图像生成)

1. 访问 [火山引擎控制台](https://console.volcengine.com/)
2. 获取 Access Key 和 Secret Key
3. 支持 Seedream 3.0 模型

---

## 开发规范

### 代码规范

1. 使用 C++11 标准
2. 使用 Qt 5.15.2 框架
3. 关键地方添加中文注释
4. 代码结构清晰、层级分明
5. 采用模块化设计

### 提交规范

- 每次提交一个功能模块
- 提交信息清晰描述变更内容
- 确保代码可以正常编译后再提交

---

## 许可证

与原项目保持一致

## 贡献指南

欢迎提交 Issue 和 Pull Request

---

*最后更新时间：2026-04-08*

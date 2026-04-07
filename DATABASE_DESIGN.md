# 数据库设计文档

> 本文档记录 C++ 版本后端的 MySQL 数据库表设计（完整11张表方案）

---

## 〇、MySQL 开发环境（CentOS 虚拟机 + VMware）

### 0.1 环境架构

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           开发环境架构                                        │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Windows 主机                                                               │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │  Qt 应用程序                                                         │   │
│  │       │                                                              │   │
│  │       │ 连接 localhost:3306                                          │   │
│  │       ▼                                                              │   │
│  │  VMware 端口转发                                                     │   │
│  │  localhost:3306 ──────────────────▶ CentOS IP:3306                  │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                              │                                              │
│                              ▼                                              │
│  CentOS 虚拟机 (VMware Workstation)                                         │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │  MySQL Server 8.0                                                    │   │
│  │  ├── 数据库: qingning_comic                                          │   │
│  │  ├── 用户: qingning                                                  │   │
│  │  └── 密码: qingning123                                               │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 0.2 CentOS 安装 MySQL 8.0

在 CentOS 虚拟机中执行以下命令：

```bash
# 1. 下载 MySQL 官方仓库
sudo yum install -y https://dev.mysql.com/get/mysql80-community-release-el7-7.noarch.rpm

# 2. 安装 MySQL Server
sudo yum install -y mysql-community-server

# 3. 启动 MySQL
sudo systemctl start mysqld
sudo systemctl enable mysqld

# 4. 查看临时密码
sudo grep 'temporary password' /var/log/mysqld.log
```

### 0.3 配置 MySQL

```bash
# 1. 登录 MySQL（使用临时密码）
mysql -u root -p

# 2. 在 MySQL 中执行以下命令
ALTER USER 'root'@'localhost' IDENTIFIED BY 'Qingning123!';

CREATE DATABASE qingning_comic CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE USER 'qingning'@'%' IDENTIFIED BY 'qingning123';
GRANT ALL PRIVILEGES ON qingning_comic.* TO 'qingning'@'%';
FLUSH PRIVILEGES;
EXIT;

# 3. 允许远程连接
sudo sed -i '/^\[mysqld\]$/a bind-address = 0.0.0.0' /etc/my.cnf
sudo systemctl restart mysqld

# 4. 开放防火墙端口
sudo firewall-cmd --permanent --add-port=3306/tcp
sudo firewall-cmd --reload

# 或者关闭防火墙（仅开发环境）
sudo systemctl stop firewalld
sudo systemctl disable firewalld

# 5. 获取 CentOS IP 地址
ip addr show
```

### 0.4 VMware Workstation 端口转发配置

#### 方法一：虚拟机设置

```
1. 关闭 CentOS 虚拟机
2. VMware 中选择虚拟机 → 右键 → 设置
3. 选择「网络适配器」→「NAT 模式」
4. 点击「高级」→「端口转发」
5. 点击「添加」，填写：
   - 主机端口: 3306
   - 类型: TCP
   - 虚拟机 IP: (CentOS 的 IP 地址)
   - 虚拟机端口: 3306
6. 点击「确定」保存
```

#### 方法二：虚拟网络编辑器

```
1. VMware 菜单栏 → 编辑 → 虚拟网络编辑器
2. 点击「更改设置」（管理员权限）
3. 选择「VMnet8 (NAT 模式)」
4. 点击「NAT 设置」
5. 在「端口转发」区域点击「添加」：
   - 主机端口: 3306
   - 虚拟机 IP: (CentOS 的 IP 地址)
   - 虚拟机端口: 3306
6. 点击「确定」保存
```

### 0.5 验证连接

```bash
# 在 CentOS 中检查 MySQL 状态
sudo systemctl status mysqld
sudo netstat -tlnp | grep 3306
```

```powershell
# 在 Windows PowerShell 中测试连接
telnet localhost 3306

# 或使用 MySQL 客户端
mysql -h localhost -P 3306 -u qingning -pqingning123 qingning_comic
```

### 0.6 连接信息

| 参数 | 值 |
|------|------|
| 主机 | localhost |
| 端口 | 3306 |
| 数据库 | qingning_comic |
| 用户名 | qingning |
| 密码 | qingning123 |

### 0.7 常用命令

```bash
# 启动 MySQL
sudo systemctl start mysqld

# 停止 MySQL
sudo systemctl stop mysqld

# 重启 MySQL
sudo systemctl restart mysqld

# 查看 MySQL 状态
sudo systemctl status mysqld

# 登录 MySQL
mysql -u qingning -pqingning123 qingning_comic

# 查看 MySQL 日志
sudo tail -f /var/log/mysqld.log
```

---

## 一、设计背景

### 1.1 原仓库技术栈

原项目使用 **AWS DynamoDB**（NoSQL 数据库），采用**单表设计**模式：

```
┌─────────────────────────────────────────────────────────────┐
│                    DynamoDB 单表                             │
│                                                             │
│  PK + SK 决定这是什么数据                                    │
├─────────────────────────────────────────────────────────────┤
│  PK: NOVEL#xxx         SK: NOVEL#xxx         → 作品         │
│  PK: NOVEL#xxx         SK: CHAR#yyy          → 角色         │
│  PK: CHAR#yyy          SK: CONFIG#zzz        → 角色配置     │
│  PK: STORY#aaa         SK: PANEL#0001#001    → 面板         │
│  PK: JOB#bbb           SK: JOB#bbb           → 任务         │
│  PK: NOVEL#xxx         SK: CR#ccc            → 修改请求     │
└─────────────────────────────────────────────────────────────┘
```

**特点**：
- 所有数据在一张表
- 用"键前缀"区分实体类型
- 灵活，但没有严格结构约束

### 1.2 C++ 版本技术栈

C++ 后端部署到阿里云，使用 **MySQL**（关系型数据库），需要转换为**多表设计**。

---

## 二、为什么是 11 张表？

### 2.1 方案对比

| 方案 | 表数量 | 复杂度 | 适合场景 |
|------|--------|--------|----------|
| 极简版 | 5张 | 低 | 快速原型，功能不全 |
| 基础版 | 8张 | 中 | 核心功能，缺少修改/导出 |
| **完整版** | **11张** | **中高** | **完整复刻原仓库 + 场景圣经** |
| 规范版 | 15张 | 高 | 大型项目，数据量大 |

### 2.2 选择 11 张表的原因

1. **完整复刻原仓库功能** - 包含修改请求、导出功能
2. **场景圣经支持** - 新增 scenes 表存储场景信息
3. **不会太复杂** - 15张表对于中小型项目过于复杂
4. **JSON字段简化设计** - 复杂数据用JSON存储，减少表数量
5. **对应原仓库实体** - 每张表对应原仓库的一种实体类型

### 2.3 表与原仓库实体的对应关系

| 序号 | MySQL 表名 | 原仓库实体 | 说明 |
|------|-----------|-----------|------|
| 1 | users | - | 用户（原仓库用Cognito） |
| 2 | novels | NOVEL# | 作品 |
| 3 | characters | CHAR# | 角色 |
| 4 | scenes | SCENE# | 场景（新增） |
| 5 | character_configs | CONFIG# | 角色配置 |
| 6 | storyboards | STORY# | 分镜版本 |
| 7 | panels | PANEL# | 面板 |
| 8 | jobs | JOB# | 任务 |
| 9 | panel_tasks | PANEL_TASK# | 子任务 |
| 10 | change_requests | CR# | 修改请求 |
| 11 | exports | - | 导出（原仓库有API） |

---

## 三、核心概念解释

### 3.1 表（Table）

**表 = Excel 里的一张工作表**

```
┌─────────────────────────────────────────────┐
│  novels 表（作品表）                         │
├──────────┬──────────┬──────────┬────────────┤
│ id       │ title    │ user_id  │ status     │
├──────────┼──────────┼──────────┼────────────┤
│ 001      | 哈利波特 │ user-1   │ completed  │
│ 002      | 三体     │ user-2   │ analyzing  │
└──────────┴──────────┴──────────┴────────────┘

- 每一列 = 一个字段（存一种数据）
- 每一行 = 一条记录（一个作品）
```

### 3.2 主键（PRIMARY KEY）

**主键 = 身份证号**

- 每条记录有唯一的主键，不会重复
- 用于快速找到某条记录
- 例如：作品 id = "550e8400-xxx"

### 3.3 外键（FOREIGN KEY）

**外键 = 引用/指向**

- 建立表与表之间的关系
- 例如：characters 表的 novel_id 指向 novels 表
- 表示"这个角色属于哪个作品"

### 3.4 索引（INDEX）

**索引 = 书的目录**

- 加快查询速度
- 但会占用额外空间

### 3.5 JSON 字段

**JSON = 灵活的数据格式**

- 存储不确定数量的数据
- 存储复杂结构的数据
- 例如：`["铠甲", "斗篷", "头盔"]`

---

## 四、表关系图

```
users（用户表）
  │
  │ 1个用户可以有多个作品（一对多）
  │
  ▼
novels（作品表）
  │
  ├──────────────────────────────┬──────────────────────────────┐
  │                              │                              │
  │ 1个作品可以有多个角色         │ 1个作品可以有多个分镜版本     │ 1个作品可以有多个修改请求
  │ (一对多)                      │ (一对多)                      │ (一对多)
  ▼                              ▼                              ▼
characters（角色表）          storyboards（分镜表）        change_requests（修改请求表）
  │                              │
  │ 1个角色可以有多个配置         │ 1个分镜可以有多个面板
  │ (一对多)                      │ (一对多)
  ▼                              ▼
character_configs（配置表）   panels（面板表）
                                 │
                                 │ 1个任务可以有多个子任务
                                 │ (一对多)
                                 ▼
                             jobs（任务表）
                                 │
                                 │ 1个任务可以有多个子任务
                                 │ (一对多)
                                 ▼
                             panel_tasks（子任务表）

novels（作品表）
  │
  │ 1个作品可以有多个导出
  │ (一对多)
  ▼
exports（导出表）
```

---

## 五、详细表结构

### 5.1 users（用户表）

**用途**：存储用户账号信息

**原仓库对应**：原仓库使用 AWS Cognito（托管用户认证服务），网页版将使用 Supabase Auth。

```sql
CREATE TABLE users (
    id VARCHAR(36) PRIMARY KEY,              -- UUID，用户唯一标识
    username VARCHAR(50) NOT NULL UNIQUE,    -- 用户名，唯一
    email VARCHAR(100) NOT NULL UNIQUE,      -- 邮箱，唯一
    password_hash VARCHAR(255),              -- 密码哈希（使用bcrypt）
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

**字段说明**：

| 字段 | 类型 | 说明 |
|------|------|------|
| id | VARCHAR(36) | 用户唯一ID（UUID格式） |
| username | VARCHAR(50) | 用户名，用于登录 |
| email | VARCHAR(100) | 邮箱，用于找回密码 |
| password_hash | VARCHAR(255) | 密码哈希值（不要存明文！） |
| created_at | TIMESTAMP | 创建时间 |

**平替说明**：

| 原仓库 | 网页版 |
|--------|---------|
| AWS Cognito（托管服务） | Supabase Auth 托管服务 |
| JWT Token 自动生成 | Supabase 自动生成 |
| 社交登录（Google/GitHub） | Supabase 内置支持 |

---

### 5.2 novels（作品表）

**用途**：存储作品基本信息，是整个系统的核心

**原仓库对应**：`PK: NOVEL#xxx, SK: NOVEL#xxx`

```sql
CREATE TABLE novels (
    id VARCHAR(36) PRIMARY KEY,
    user_id VARCHAR(36) NOT NULL,
    title VARCHAR(200) NOT NULL,
    original_text TEXT,                      -- 短文本直接存储
    original_text_path VARCHAR(255),         -- 长文本存OSS，这里存路径
    status VARCHAR(20) DEFAULT 'created',    -- created/analyzing/analyzed/generating/completed/error
    storyboard_id VARCHAR(36),               -- 当前分镜ID
    metadata JSON,                           -- {genre, tags, language}
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    
    FOREIGN KEY (user_id) REFERENCES users(id),
    INDEX idx_user (user_id),
    INDEX idx_status (status)
);
```

**字段说明**：

| 字段 | 类型 | 说明 |
|------|------|------|
| id | VARCHAR(36) | 作品唯一ID |
| user_id | VARCHAR(36) | 所属用户（外键） |
| title | VARCHAR(200) | 作品标题 |
| original_text | TEXT | 原文内容（短文本） |
| original_text_path | VARCHAR(255) | 原文OSS路径（长文本） |
| status | VARCHAR(20) | 作品状态 |
| storyboard_id | VARCHAR(36) | 当前分镜ID |
| metadata | JSON | 元数据（类型、标签、语言） |

**状态流转**：

```
created → analyzing → analyzed → generating → completed
                         ↓
                      error
```

---

### 5.3 characters（角色表）

**用途**：存储作品中的角色信息

**原仓库对应**：`PK: NOVEL#xxx, SK: CHAR#yyy`

```sql
CREATE TABLE characters (
    id VARCHAR(36) PRIMARY KEY,
    novel_id VARCHAR(36) NOT NULL,
    name VARCHAR(50) NOT NULL,
    role VARCHAR(20),                        -- protagonist/antagonist/supporting/background
    gender VARCHAR(10),                      -- male/female/other
    age INT,
    personalities JSON,                      -- ["勇敢", "善良", "有时冲动"]
    default_config_id VARCHAR(36),           -- 默认配置ID
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    FOREIGN KEY (novel_id) REFERENCES novels(id) ON DELETE CASCADE,
    INDEX idx_novel (novel_id)
);
```

**字段说明**：

| 字段 | 类型 | 说明 |
|------|------|------|
| id | VARCHAR(36) | 角色唯一ID |
| novel_id | VARCHAR(36) | 所属作品（外键） |
| name | VARCHAR(50) | 角色名称 |
| role | VARCHAR(20) | 角色类型（主角/反派/配角/背景） |
| gender | VARCHAR(10) | 性别 |
| age | INT | 年龄 |
| personalities | JSON | 性格特征列表 |
| default_config_id | VARCHAR(36) | 默认配置ID |

**角色类型说明**：

| 值 | 中文 | 说明 |
|------|------|------|
| protagonist | 主角 | 故事核心人物 |
| antagonist | 反派 | 对立角色 |
| supporting | 配角 | 重要但非核心 |
| background | 背景角色 | 群众、路人 |

---

### 5.4 scenes（场景表）⭐ 新增

**用途**：存储作品中的场景信息，支持场景圣经功能

**原仓库对应**：原仓库场景信息存储在 panels 的 content JSON 中，本版本单独建表

```sql
CREATE TABLE scenes (
    id VARCHAR(36) PRIMARY KEY,
    novel_id VARCHAR(36) NOT NULL,
    name VARCHAR(100) NOT NULL,
    scene_id VARCHAR(50),
    details JSON,
    tags JSON,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    
    FOREIGN KEY (novel_id) REFERENCES novels(id) ON DELETE CASCADE,
    INDEX idx_novel (novel_id),
    INDEX idx_scene_id (scene_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
```

**字段说明**：

| 字段 | 类型 | 说明 |
|------|------|------|
| id | VARCHAR(36) | 场景唯一ID |
| novel_id | VARCHAR(36) | 所属作品（外键） |
| name | VARCHAR(100) | 场景名称 |
| scene_id | VARCHAR(50) | AI 返回的场景标识 |
| details | JSON | 场景详细信息 |
| tags | JSON | 标签列表 |

**details JSON 结构**：

```json
{
  "description": "森林入口，阳光透过树叶",
  "building": "古老的石制拱门",
  "color": "绿色、金色",
  "landmark": "巨大的橡树",
  "layout": "开阔的前景，远处有山脉",
  "atmosphere": "神秘、宁静",
  "type": "outdoor",
  "setting": "森林入口",
  "timeOfDay": "清晨",
  "weather": "晴朗",
  "details": ["阳光透过树叶", "小溪流淌", "鸟鸣声"]
}
```

**为什么需要场景表？**

- **场景圣经**：保持场景一致性，避免同一场景在不同章节画得不一样
- **场景复用**：同一场景可以在多个面板中复用
- **场景管理**：用户可以编辑、删除场景

---

### 5.5 character_configs（角色配置表）

**用途**：存储角色的不同造型配置（战斗模式、日常装扮等）

**原仓库对应**：`PK: CHAR#yyy, SK: CONFIG#zzz`

```sql
CREATE TABLE character_configs (
    id VARCHAR(36) PRIMARY KEY,
    character_id VARCHAR(36) NOT NULL,
    name VARCHAR(100) NOT NULL,              -- "战斗模式"、"日常装扮"
    description TEXT,                        -- 详细文字描述
    appearance JSON,                         -- {hairStyle, clothing, accessories, ...}
    tags JSON,                               -- ["战斗", "铠甲", "持剑"]
    reference_images JSON,                   -- [{ossKey, caption, uploadedAt}]
    generated_portraits JSON,                -- [{view, ossKey, generatedAt}]
    is_default BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    FOREIGN KEY (character_id) REFERENCES characters(id) ON DELETE CASCADE,
    INDEX idx_character (character_id)
);
```

**字段说明**：

| 字段 | 类型 | 说明 |
|------|------|------|
| id | VARCHAR(36) | 配置唯一ID |
| character_id | VARCHAR(36) | 所属角色（外键） |
| name | VARCHAR(100) | 配置名称 |
| description | TEXT | 详细描述 |
| appearance | JSON | 外貌描述 |
| tags | JSON | 标签列表 |
| reference_images | JSON | 用户上传的参考图 |
| generated_portraits | JSON | AI生成的标准像 |
| is_default | BOOLEAN | 是否为默认配置 |

**为什么需要角色配置？**

同一个角色在不同场景有不同造型：
- 战斗场景 → 穿铠甲、持剑
- 日常生活 → 休闲服装
- 正式场合 → 礼服

---

### 5.5 storyboards（分镜表）

**用途**：存储分镜版本信息，支持多版本管理

**原仓库对应**：`PK: NOVEL#xxx, SK: STORY#aaa#v1`

```sql
CREATE TABLE storyboards (
    id VARCHAR(36) PRIMARY KEY,
    novel_id VARCHAR(36) NOT NULL,
    version INT DEFAULT 1,
    total_pages INT DEFAULT 0,
    panel_count INT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    FOREIGN KEY (novel_id) REFERENCES novels(id) ON DELETE CASCADE,
    UNIQUE KEY uk_novel_version (novel_id, version)
);
```

**字段说明**：

| 字段 | 类型 | 说明 |
|------|------|------|
| id | VARCHAR(36) | 分镜唯一ID |
| novel_id | VARCHAR(36) | 所属作品（外键） |
| version | INT | 版本号 |
| total_pages | INT | 总页数 |
| panel_count | INT | 面板总数 |

**为什么需要分镜版本？**

- 用户可能重新分析小说，生成新版本
- 保留历史版本，方便对比和回退

---

### 5.6 panels（面板表）

**用途**：存储每个漫画格子的内容，是AI生成图片的最小单位

**原仓库对应**：`PK: STORY#aaa, SK: PANEL#0001#001`

```sql
CREATE TABLE panels (
    id VARCHAR(36) PRIMARY KEY,
    storyboard_id VARCHAR(36) NOT NULL,
    page INT NOT NULL,                       -- 页码
    panel_index INT NOT NULL,                -- 页内序号
    
    content JSON,                            -- {scene, shotType, characters, dialogue}
    visual_prompt TEXT,                      -- 给AI的提示词
    
    preview_image_path VARCHAR(255),         -- 预览图OSS路径
    hd_image_path VARCHAR(255),              -- 高清图OSS路径
    layout JSON,                             -- {width, height, x, y}
    
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    FOREIGN KEY (storyboard_id) REFERENCES storyboards(id) ON DELETE CASCADE,
    UNIQUE KEY uk_storyboard_page_index (storyboard_id, page, panel_index),
    INDEX idx_storyboard (storyboard_id)
);
```

**字段说明**：

| 字段 | 类型 | 说明 |
|------|------|------|
| id | VARCHAR(36) | 面板唯一ID |
| storyboard_id | VARCHAR(36) | 所属分镜（外键） |
| page | INT | 页码（从1开始） |
| panel_index | INT | 页内序号（从0开始） |
| content | JSON | 面板内容 |
| visual_prompt | TEXT | AI提示词 |
| preview_image_path | VARCHAR(255) | 预览图路径 |
| hd_image_path | VARCHAR(255) | 高清图路径 |
| layout | JSON | 布局信息 |

**content JSON 结构**：

```json
{
  "scene": "森林入口，阳光透过树叶",
  "shotType": "medium",
  "characters": [
    {
      "charId": "char-001",
      "name": "艾莉娅",
      "pose": "站立",
      "expression": "determined"
    }
  ],
  "dialogue": [
    {
      "speaker": "艾莉娅",
      "text": "出发吧！",
      "bubbleType": "speech"
    }
  ]
}
```

---

### 5.7 jobs（任务表）

**用途**：存储异步任务进度，用户可以看到处理状态

**原仓库对应**：`PK: JOB#bbb, SK: JOB#bbb`

```sql
CREATE TABLE jobs (
    id VARCHAR(36) PRIMARY KEY,
    novel_id VARCHAR(36),
    storyboard_id VARCHAR(36),
    
    type VARCHAR(20) NOT NULL,               -- analyze/generate_preview/generate_hd/edit/export
    status VARCHAR(20) DEFAULT 'pending',    -- pending/in_progress/completed/failed
    
    total_tasks INT DEFAULT 0,
    completed_tasks INT DEFAULT 0,
    failed_tasks INT DEFAULT 0,
    
    result JSON,                             -- 任务结果
    error_message TEXT,
    
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    
    INDEX idx_status (status),
    INDEX idx_novel (novel_id)
);
```

**字段说明**：

| 字段 | 类型 | 说明 |
|------|------|------|
| id | VARCHAR(36) | 任务唯一ID |
| novel_id | VARCHAR(36) | 关联作品 |
| storyboard_id | VARCHAR(36) | 关联分镜 |
| type | VARCHAR(20) | 任务类型 |
| status | VARCHAR(20) | 任务状态 |
| total_tasks | INT | 总子任务数 |
| completed_tasks | INT | 已完成数 |
| failed_tasks | INT | 失败数 |
| result | JSON | 任务结果 |
| error_message | TEXT | 错误信息 |

**任务类型说明**：

| 类型代码 | 中文显示名称 | 说明 |
|------|------|------|
| analyze | 分析小说 | 分析小说文本，生成角色和分镜 |
| generate_storyboard | 生成分镜 | 生成分镜脚本 |
| generate_preview | 面板预览生成 | 生成预览图（512x288） |
| generate_hd | 面板高清生成 | 生成高清图（1920x1080） |
| generate_portrait | 角色标准像 | 生成角色标准肖像 |
| change_request | 修改请求执行 | 处理用户修改请求 |
| panel_edit | 面板编辑 | 编辑面板内容 |
| export_pdf | 导出 PDF | 导出 PDF 漫画文档 |
| export_webtoon | 导出 Webtoon | 导出 Webtoon 长图 |
| export_resources | 导出资源包 | 导出资源包（ZIP） |
| novel_analysis | 小说分析 | 分析小说内容 |
| panel_generation | 面板生成 | 生成分镜面板 |
| export | 导出任务 | 通用导出任务 |

---

### 5.8 panel_tasks（面板子任务表）

**用途**：存储每个面板的生成任务，追踪每张图的生成状态

**原仓库对应**：`PK: JOB#bbb, SK: PANEL_TASK#ccc`

```sql
CREATE TABLE panel_tasks (
    id VARCHAR(36) PRIMARY KEY,
    job_id VARCHAR(36) NOT NULL,
    panel_id VARCHAR(36) NOT NULL,
    
    mode VARCHAR(10) NOT NULL,               -- preview/hd
    status VARCHAR(20) DEFAULT 'pending',
    character_refs JSON,                     -- 角色参考图
    
    result_path VARCHAR(255),                -- 生成的图片路径
    retry_count INT DEFAULT 0,
    error_message TEXT,
    
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    FOREIGN KEY (job_id) REFERENCES jobs(id) ON DELETE CASCADE,
    FOREIGN KEY (panel_id) REFERENCES panels(id) ON DELETE CASCADE,
    INDEX idx_job (job_id),
    INDEX idx_status (status)
);
```

**字段说明**：

| 字段 | 类型 | 说明 |
|------|------|------|
| id | VARCHAR(36) | 子任务唯一ID |
| job_id | VARCHAR(36) | 所属任务（外键） |
| panel_id | VARCHAR(36) | 关联面板（外键） |
| mode | VARCHAR(10) | 生成模式 |
| status | VARCHAR(20) | 任务状态 |
| character_refs | JSON | 角色参考图 |
| result_path | VARCHAR(255) | 结果图片路径 |
| retry_count | INT | 重试次数 |
| error_message | TEXT | 错误信息 |

**为什么需要子任务表？**

- 一次生成可能包含几百张图
- 需要追踪每张图的生成状态
- 失败时可以单独重试

---

### 5.9 change_requests（修改请求表）⭐ 新增

**用途**：存储用户的自然语言修改请求，AI解析后执行

**原仓库对应**：`PK: NOVEL#xxx, SK: CR#ccc`

**应用场景**：
- 用户输入："把第3页第2格的表情改为微笑"
- AI 解析成结构化指令
- 执行修改操作

```sql
CREATE TABLE change_requests (
    id VARCHAR(36) PRIMARY KEY,
    novel_id VARCHAR(36) NOT NULL,
    user_id VARCHAR(36) NOT NULL,
    
    natural_language TEXT NOT NULL,          -- 用户的自然语言输入
    dsl JSON,                                -- AI解析后的结构化指令
    status VARCHAR(20) DEFAULT 'parsing',    -- parsing/pending/executing/completed/failed
    
    job_id VARCHAR(36),                      -- 关联的任务ID
    
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    
    FOREIGN KEY (novel_id) REFERENCES novels(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(id),
    INDEX idx_novel (novel_id),
    INDEX idx_status (status)
);
```

**字段说明**：

| 字段 | 类型 | 说明 |
|------|------|------|
| id | VARCHAR(36) | 修改请求唯一ID |
| novel_id | VARCHAR(36) | 所属作品（外键） |
| user_id | VARCHAR(36) | 提交用户（外键） |
| natural_language | TEXT | 用户的自然语言输入 |
| dsl | JSON | AI解析后的结构化指令 |
| status | VARCHAR(20) | 处理状态 |
| job_id | VARCHAR(36) | 关联的任务ID |

**dsl JSON 结构示例**：

```json
{
  "scope": "panel",           // 作用域：global/character/panel/page
  "targetId": "panel-uuid",   // 目标ID
  "type": "art",              // 修改类型：art/dialogue/layout/style
  "ops": [
    {
      "action": "inpaint",    // 操作：inpaint/outpaint/bg_swap/repose
      "params": {
        "region": "face",
        "instruction": "change expression to smile"
      }
    }
  ]
}
```

**状态流转**：

```
parsing → pending → executing → completed
              ↓
           failed
```

---

### 5.10 exports（导出表）⭐ 新增

**用途**：存储导出任务记录，生成PDF/长图/资源包

**原仓库对应**：原仓库有导出API，但数据存储在jobs表中

**应用场景**：
- 用户点击"导出PDF"
- 系统生成导出文件
- 用户下载

```sql
CREATE TABLE exports (
    id VARCHAR(36) PRIMARY KEY,
    novel_id VARCHAR(36) NOT NULL,
    
    format VARCHAR(20) NOT NULL,             -- pdf/webtoon/resources
    status VARCHAR(20) DEFAULT 'pending',    -- pending/processing/completed/failed
    
    file_path VARCHAR(255),                  -- OSS文件路径
    file_size BIGINT,                        -- 文件大小（字节）
    download_url VARCHAR(500),               -- 下载链接（预签名URL）
    url_expires_at TIMESTAMP,                -- 链接过期时间
    
    error_message TEXT,
    
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    
    FOREIGN KEY (novel_id) REFERENCES novels(id) ON DELETE CASCADE,
    INDEX idx_novel (novel_id),
    INDEX idx_status (status)
);
```

**字段说明**：

| 字段 | 类型 | 说明 |
|------|------|------|
| id | VARCHAR(36) | 导出唯一ID |
| novel_id | VARCHAR(36) | 所属作品（外键） |
| format | VARCHAR(20) | 导出格式 |
| status | VARCHAR(20) | 导出状态 |
| file_path | VARCHAR(255) | OSS文件路径 |
| file_size | BIGINT | 文件大小 |
| download_url | VARCHAR(500) | 下载链接 |
| url_expires_at | TIMESTAMP | 链接过期时间 |
| error_message | TEXT | 错误信息 |

**导出格式说明**：

| 格式 | 说明 | 输出文件 |
|------|------|----------|
| pdf | PDF漫画 | comic.pdf |
| webtoon | 长图格式 | webtoon.png |
| resources | 资源包 | resources.zip（PNG + JSON） |

---

## 六、OSS 存储结构设计

### 6.1 什么是 OSS？

**OSS = 对象存储服务**

| 原仓库 | C++版本（阿里云） |
|--------|------------------|
| AWS S3 | 阿里云 OSS |
| 存储图片、文件 | 存储图片、文件 |
| 预签名URL下载 | 签名URL下载 |

### 6.2 为什么需要 OSS？

| 存储在数据库 | 存储在OSS |
|-------------|----------|
| ❌ 占用数据库空间 | ✅ 便宜，按量付费 |
| ❌ 查询变慢 | ✅ 数据库只存路径 |
| ❌ 备份困难 | ✅ 自动备份 |
| ❌ 无法直接访问 | ✅ 生成下载链接 |

### 6.3 OSS 目录结构

```
qingning-comic-assets/                    # OSS Bucket 名称
│
├── novels/                               # 作品相关
│   └── {novelId}/
│       └── original.txt                  # 原文文件
│
├── characters/                           # 角色相关
│   └── {charId}/
│       ├── {configId}/                   # 配置目录
│       │   ├── ref-001.png               # 用户上传的参考图
│       │   ├── ref-002.png
│       │   ├── portrait-front.png        # AI生成的标准像（正面）
│       │   ├── portrait-side.png         # 侧面
│       │   └── portrait-three-quarter.png # 四分之三视角
│       │
│       └── {configId-2}/                 # 另一个配置
│           └── ...
│
├── panels/                               # 面板图片
│   └── {jobId}/
│       ├── {panelId}-preview.png         # 预览图（512x288）
│       └── {panelId}-hd.png              # 高清图（1920x1080）
│
├── edits/                                # 编辑结果
│   └── {changeRequestId}/
│       ├── {panelId}-edit-1.png          # 编辑版本1
│       └── {panelId}-edit-2.png          # 编辑版本2
│
└── exports/                              # 导出文件
    └── {exportId}/
        ├── comic.pdf                     # PDF导出
        ├── webtoon.png                   # 长图导出
        └── resources.zip                 # 资源包
```

### 6.4 OSS 与数据库的关系

```
数据库存储路径                    OSS存储实际文件
─────────────────               ─────────────────
panels.preview_image_path   →   panels/{jobId}/{panelId}-preview.png
panels.hd_image_path        →   panels/{jobId}/{panelId}-hd.png
novels.original_text_path   →   novels/{novelId}/original.txt
exports.file_path           →   exports/{exportId}/comic.pdf
```

### 6.5 C++ Qt 中使用 OSS

**阿里云 OSS SDK**：

```cpp
// 安装阿里云 OSS C++ SDK
// 使用 Qt 的网络模块也可以直接调用 OSS REST API

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

// 上传文件到 OSS
void uploadToOSS(const QString& localPath, const QString& ossPath) {
    QNetworkAccessManager* manager = new QNetworkAccessManager();
    
    QString url = QString("https://qingning-comic-assets.oss-cn-hangzhou.aliyuncs.com/%1")
                  .arg(ossPath);
    
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", generateOSSSignature());
    
    QFile file(localPath);
    file.open(QIODevice::ReadOnly);
    manager->put(request, &file);
}

// 生成预签名下载URL
QString generatePresignedUrl(const QString& ossPath, int expireSeconds = 900) {
    // 使用阿里云 OSS SDK 生成签名URL
    // 返回类似：https://bucket.oss-cn-hangzhou.aliyuncs.com/path?OSSAccessKeyId=xxx&Expires=xxx&Signature=xxx
}
```

---

## 七、AWS 到阿里云的平替方案

### 7.1 服务对照表

| 功能 | 原仓库（AWS） | 网页版（Cloudflare） |
|------|--------------|---------------------|
| 用户认证 | AWS Cognito | Supabase Auth |
| 数据库 | DynamoDB | MySQL |
| 对象存储 | S3 | Cloudflare R2 |
| 函数计算 | Lambda | Cloudflare Workers |
| API网关 | API Gateway | Cloudflare Workers |
| 消息队列 | SQS | Cloudflare Queues |
| CDN | CloudFront | Cloudflare Pages |

### 7.2 详细平替说明

#### 用户认证

| 原仓库 | 网页版 |
|--------|---------|
| AWS Cognito 托管服务 | Supabase Auth 托管服务 |
| 自动生成 JWT | Supabase 自动生成 |
| 支持社交登录 | Supabase 内置 OAuth |

**Supabase Auth 使用**：

```javascript
// 网页端注册
const { data, error } = await supabase.auth.signUp({
  email: 'user@example.com',
  password: 'password123'
})

// 网页端登录
const { data, error } = await supabase.auth.signInWithPassword({
  email: 'user@example.com',
  password: 'password123'
})

// OAuth 登录（Google/GitHub）
const { data, error } = await supabase.auth.signInWithOAuth({
  provider: 'google'
})
```

#### 数据库

| 原仓库 | C++版本 |
|--------|---------|
| DynamoDB（NoSQL） | MySQL（关系型） |
| 单表设计 | 多表设计 |
| 按需付费 | 按规格付费 |

**C++ 连接 MySQL**：

```cpp
// 使用 MySQL Connector/C++ 或 Qt SQL 模块
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
db.setHostName("rm-xxx.mysql.rds.aliyuncs.com");
db.setDatabaseName("qingning_comic");
db.setUserName("root");
db.setPassword("password");

if (db.open()) {
    QSqlQuery query;
    query.exec("SELECT * FROM novels WHERE user_id = 'xxx'");
}
```

#### 对象存储

| 原仓库 | C++版本 |
|--------|---------|
| AWS S3 | 阿里云 OSS |
| 预签名 URL | 签名 URL |
| SDK: aws-sdk-s3 | SDK: aliyun-oss-cpp-sdk |

#### 函数计算

| 原仓库 | C++版本 |
|--------|---------|
| AWS Lambda（Node.js） | 阿里云函数计算 FC / ECS |
| 自动扩缩容 | 需要配置 |

**建议**：C++ 后端部署到 **ECS 云服务器**，使用 Docker 容器化部署。

---

## 八、与原仓库的对比

### 8.1 数据库对比

| 方面 | 原仓库（DynamoDB） | C++版本（MySQL） |
|------|-------------------|------------------|
| 表数量 | 1张表 | 10张表 |
| 设计理念 | 单表设计，键前缀区分 | 多表设计，外键关联 |
| 查询方式 | Query + begins_with | JOIN + WHERE |
| 索引 | GSI | INDEX |
| 复杂数据 | Map/List | JSON字段 |
| 事务 | 有限支持 | 完整ACID支持 |

### 8.2 功能对比

| 功能 | 原仓库 | 网页版 |
|------|--------|---------|
| 用户认证 | AWS Cognito | Supabase Auth |
| 作品管理 | ✅ | ✅ |
| 角色管理 | ✅ | ✅ |
| 角色配置 | ✅ | ✅ |
| 分镜版本 | ✅ | ✅ |
| 面板生成 | ✅ | ✅ |
| 任务追踪 | ✅ | ✅ |
| 修改请求 | ✅ | ✅ |
| 导出功能 | ✅ | ✅ |
| 文件存储 | S3 | OSS |

---

## 九、常用查询示例

### 9.1 获取作品的所有角色

```sql
-- 对应原仓库：Query(PK=NOVEL#xxx, SK begins_with CHAR#)
SELECT * FROM characters WHERE novel_id = 'xxx';
```

### 9.2 获取角色的所有配置

```sql
-- 对应原仓库：Query(PK=CHAR#xxx)
SELECT * FROM character_configs WHERE character_id = 'xxx';
```

### 9.3 获取分镜的所有面板（按顺序）

```sql
-- 对应原仓库：Query(PK=STORY#xxx)
SELECT * FROM panels 
WHERE storyboard_id = 'xxx' 
ORDER BY page, panel_index;
```

### 9.4 获取待处理任务

```sql
-- 对应原仓库：Query(GSI2, GSI2PK=JOB#status#pending)
SELECT * FROM jobs 
WHERE status = 'pending' 
ORDER BY created_at DESC;
```

### 9.5 获取任务的所有子任务

```sql
-- 对应原仓库：Query(PK=JOB#xxx, SK begins_with PANEL_TASK#)
SELECT * FROM panel_tasks WHERE job_id = 'xxx';
```

### 9.6 获取作品的修改请求历史

```sql
-- 对应原仓库：Query(PK=NOVEL#xxx, SK begins_with CR#)
SELECT * FROM change_requests 
WHERE novel_id = 'xxx' 
ORDER BY created_at DESC;
```

### 9.7 获取作品的导出历史

```sql
SELECT * FROM exports 
WHERE novel_id = 'xxx' 
ORDER BY created_at DESC;
```

### 9.8 获取作品的完整信息（多表JOIN）

```sql
SELECT 
    n.id as novel_id,
    n.title,
    c.id as character_id,
    c.name as character_name,
    cc.name as config_name
FROM novels n
LEFT JOIN characters c ON c.novel_id = n.id
LEFT JOIN character_configs cc ON cc.character_id = c.id
WHERE n.id = 'xxx';
```

---

## 十、设计总结

### 10.1 设计原则

1. **实体分离** - 每种实体一张表，结构清晰
2. **外键关联** - 通过外键建立父子关系
3. **JSON简化** - 复杂数据用JSON存储，减少表数量
4. **索引优化** - 为常用查询字段创建索引
5. **OSS分离** - 大文件存储在OSS，数据库只存路径

### 10.2 为什么这样设计

| 设计决策 | 原因 |
|----------|------|
| 分10张表 | 完整复刻原仓库功能 |
| 使用外键 | 保证数据一致性，支持级联删除 |
| 使用JSON字段 | 灵活存储复杂数据，减少表数量 |
| 创建索引 | 加快常用查询速度 |
| OSS存储文件 | 节省数据库空间，支持直接下载 |

### 10.3 注意事项

1. **MySQL版本要求** - 5.7+ 才支持JSON字段
2. **字符集设置** - 使用 utf8mb4 支持中文和表情
3. **存储引擎** - 使用 InnoDB 支持事务和外键
4. **OSS存储** - 大文件（图片、原文）存储在OSS，数据库只存路径
5. **密码安全** - 使用 bcrypt 加密，不要存明文

---

## 十一、附录：完整建表SQL

```sql
-- 创建数据库
CREATE DATABASE qingning_comic CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE qingning_comic;

-- 1. 用户表
CREATE TABLE users (
    id VARCHAR(36) PRIMARY KEY,
    username VARCHAR(50) NOT NULL UNIQUE,
    email VARCHAR(100) NOT NULL UNIQUE,
    password_hash VARCHAR(255),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 2. 作品表
CREATE TABLE novels (
    id VARCHAR(36) PRIMARY KEY,
    user_id VARCHAR(36) NOT NULL,
    title VARCHAR(200) NOT NULL,
    original_text TEXT,
    original_text_path VARCHAR(255),
    status VARCHAR(20) DEFAULT 'created',
    storyboard_id VARCHAR(36),
    metadata JSON,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id),
    INDEX idx_user (user_id),
    INDEX idx_status (status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 3. 角色表
CREATE TABLE characters (
    id VARCHAR(36) PRIMARY KEY,
    novel_id VARCHAR(36) NOT NULL,
    name VARCHAR(50) NOT NULL,
    role VARCHAR(20),
    gender VARCHAR(10),
    age INT,
    personalities JSON,
    default_config_id VARCHAR(36),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (novel_id) REFERENCES novels(id) ON DELETE CASCADE,
    INDEX idx_novel (novel_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 4. 角色配置表
CREATE TABLE character_configs (
    id VARCHAR(36) PRIMARY KEY,
    character_id VARCHAR(36) NOT NULL,
    name VARCHAR(100) NOT NULL,
    description TEXT,
    appearance JSON,
    tags JSON,
    reference_images JSON,
    generated_portraits JSON,
    is_default BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (character_id) REFERENCES characters(id) ON DELETE CASCADE,
    INDEX idx_character (character_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 5. 分镜表
CREATE TABLE storyboards (
    id VARCHAR(36) PRIMARY KEY,
    novel_id VARCHAR(36) NOT NULL,
    version INT DEFAULT 1,
    total_pages INT DEFAULT 0,
    panel_count INT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (novel_id) REFERENCES novels(id) ON DELETE CASCADE,
    UNIQUE KEY uk_novel_version (novel_id, version)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 6. 面板表
CREATE TABLE panels (
    id VARCHAR(36) PRIMARY KEY,
    storyboard_id VARCHAR(36) NOT NULL,
    page INT NOT NULL,
    panel_index INT NOT NULL,
    content JSON,
    visual_prompt TEXT,
    preview_image_path VARCHAR(255),
    hd_image_path VARCHAR(255),
    layout JSON,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (storyboard_id) REFERENCES storyboards(id) ON DELETE CASCADE,
    UNIQUE KEY uk_storyboard_page_index (storyboard_id, page, panel_index),
    INDEX idx_storyboard (storyboard_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 7. 任务表
CREATE TABLE jobs (
    id VARCHAR(36) PRIMARY KEY,
    novel_id VARCHAR(36),
    storyboard_id VARCHAR(36),
    type VARCHAR(20) NOT NULL,
    status VARCHAR(20) DEFAULT 'pending',
    total_tasks INT DEFAULT 0,
    completed_tasks INT DEFAULT 0,
    failed_tasks INT DEFAULT 0,
    result JSON,
    error_message TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_status (status),
    INDEX idx_novel (novel_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 8. 面板子任务表
CREATE TABLE panel_tasks (
    id VARCHAR(36) PRIMARY KEY,
    job_id VARCHAR(36) NOT NULL,
    panel_id VARCHAR(36) NOT NULL,
    mode VARCHAR(10) NOT NULL,
    status VARCHAR(20) DEFAULT 'pending',
    character_refs JSON,
    result_path VARCHAR(255),
    retry_count INT DEFAULT 0,
    error_message TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (job_id) REFERENCES jobs(id) ON DELETE CASCADE,
    FOREIGN KEY (panel_id) REFERENCES panels(id) ON DELETE CASCADE,
    INDEX idx_job (job_id),
    INDEX idx_status (status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 9. 修改请求表
CREATE TABLE change_requests (
    id VARCHAR(36) PRIMARY KEY,
    novel_id VARCHAR(36) NOT NULL,
    user_id VARCHAR(36) NOT NULL,
    natural_language TEXT NOT NULL,
    dsl JSON,
    status VARCHAR(20) DEFAULT 'parsing',
    job_id VARCHAR(36),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (novel_id) REFERENCES novels(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(id),
    INDEX idx_novel (novel_id),
    INDEX idx_status (status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 10. 导出表
CREATE TABLE exports (
    id VARCHAR(36) PRIMARY KEY,
    novel_id VARCHAR(36) NOT NULL,
    format VARCHAR(20) NOT NULL,
    status VARCHAR(20) DEFAULT 'pending',
    file_path VARCHAR(255),
    file_size BIGINT,
    download_url VARCHAR(500),
    url_expires_at TIMESTAMP,
    error_message TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (novel_id) REFERENCES novels(id) ON DELETE CASCADE,
    INDEX idx_novel (novel_id),
    INDEX idx_status (status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

---

**文档版本**: v2.1  
**最后更新**: 2026-03-09  
**作者**: AI Assistant

---

## 十二、网页部署配置

### 12.1 部署架构

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           网页部署架构                                        │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  用户浏览器                                                                  │
│      │                                                                      │
│      │ HTTPS                                                                │
│      ▼                                                                      │
│  Cloudflare Pages (前端静态托管)                                            │
│      │                                                                      │
│      │ API 请求                                                             │
│      ▼                                                                      │
│  Cloudflare Workers (后端 API)                                              │
│      │                                                                      │
│      ├──────────────────────────────┬──────────────────────────────┐       │
│      │                              │                              │       │
│      ▼                              ▼                              ▼       │
│  MySQL 数据库                   Cloudflare R2                  AI API       │
│  (云数据库服务)                  (对象存储)                    (Qwen/Imagen)│
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 12.2 MySQL 云数据库选项

| 服务商 | 产品 | 特点 | 价格 |
|--------|------|------|------|
| 阿里云 | RDS MySQL | 国内首选，稳定可靠 | 按规格付费 |
| 腾讯云 | 云数据库 MySQL | 价格优惠 | 按规格付费 |
| AWS | RDS for MySQL | 全球部署 | 按规格付费 |
| PlanetScale | Serverless MySQL | 无服务器，自动扩缩容 | 按使用量付费 |
| Neon | Serverless PostgreSQL | 可考虑迁移到 PostgreSQL | 免费额度 |

### 12.3 推荐配置：阿里云 RDS MySQL

```yaml
# 推荐配置
实例规格: mysql.n2.small.1 (1核2GB)
存储空间: 20GB SSD
地域: 华东1(杭州) 或 华北2(北京)
版本: MySQL 8.0
字符集: utf8mb4
时区: Asia/Shanghai
```

### 12.4 连接配置

```cpp
// DatabaseManager.cpp 网页版配置
DatabaseConfig config;
config.host = "rm-xxxxx.mysql.rds.aliyuncs.com";  // RDS 内网地址
config.port = 3306;
config.database = "qingning_comic";
config.username = "qingning";
config.password = "your_secure_password";
```

### 12.5 安全配置

```sql
-- 1. 创建专用数据库用户
CREATE USER 'qingning_app'@'%' IDENTIFIED BY 'strong_password_here';

-- 2. 授予最小权限
GRANT SELECT, INSERT, UPDATE, DELETE ON qingning_comic.* TO 'qingning_app'@'%';
FLUSH PRIVILEGES;

-- 3. 禁止 root 远程登录
DELETE FROM mysql.user WHERE User='root' AND Host NOT IN ('localhost', '127.0.0.1');
FLUSH PRIVILEGES;
```

### 12.6 环境变量配置

```bash
# .env 文件 (不要提交到 Git)
DB_HOST=rm-xxxxx.mysql.rds.aliyuncs.com
DB_PORT=3306
DB_NAME=qingning_comic
DB_USER=qingning_app
DB_PASSWORD=your_secure_password

# AI API Keys
QWEN_API_KEY=sk-xxxxx
IMAGEN_API_KEY=xxxxx

# Cloudflare R2
R2_ACCOUNT_ID=xxxxx
R2_ACCESS_KEY=xxxxx
R2_SECRET_KEY=xxxxx
R2_BUCKET=qingning-comic-assets
```

### 12.7 数据库连接池配置

```cpp
// 推荐配置
#define DB_POOL_MIN_CONNECTIONS 2
#define DB_POOL_MAX_CONNECTIONS 10
#define DB_CONNECTION_TIMEOUT 30000  // 30秒
#define DB_IDLE_TIMEOUT 60000        // 60秒
```

### 12.8 备份策略

```bash
# 每日自动备份
# 阿里云 RDS 控制台 → 备份恢复 → 设置备份策略

# 手动备份命令
mysqldump -h rm-xxxxx.mysql.rds.aliyuncs.com \
  -u qingning_app -p \
  --single-transaction \
  --routines \
  --triggers \
  qingning_comic > backup_$(date +%Y%m%d).sql

# 恢复命令
mysql -h rm-xxxxx.mysql.rds.aliyuncs.com \
  -u qingning_app -p \
  qingning_comic < backup_20260309.sql
```

### 12.9 性能优化建议

```sql
-- 1. 查看慢查询
SHOW VARIABLES LIKE 'slow_query%';
SET GLOBAL slow_query_log = 'ON';
SET GLOBAL long_query_time = 2;

-- 2. 查看索引使用情况
EXPLAIN SELECT * FROM novels WHERE user_id = 'xxx';

-- 3. 定期优化表
OPTIMIZE TABLE novels;
OPTIMIZE TABLE panels;
OPTIMIZE TABLE jobs;

-- 4. 查看表状态
SHOW TABLE STATUS LIKE 'novels';
```

### 12.10 监控指标

| 指标 | 说明 | 告警阈值 |
|------|------|----------|
| 连接数 | 当前连接数 | > 80% 最大连接数 |
| CPU 使用率 | 数据库 CPU | > 80% |
| 内存使用率 | 缓冲池使用 | > 90% |
| 磁盘使用率 | 存储空间 | > 80% |
| 慢查询数 | 每分钟慢查询 | > 10/分钟 |

# 青柠漫画

> AI 驱动的小说转漫画桌面应用，C++11 + Qt 5.15.2 实现

将小说文本通过 AI 分析，自动生成分镜脚本和漫画面板图片。

---

## 功能

- 小说管理（上传、编辑、删除）
- AI 文本分析与分镜脚本生成（Qwen API）
- 角色圣经管理（角色一致性维护）
- 漫画面板图片生成（通义万相 / 火山引擎）
- 图片编辑与调整
- 导出（PDF / Webtoon / 资源包）

---

## 技术栈

| 组件 | 选型 |
|------|------|
| 语言 | C++11 |
| GUI | Qt 5.15.2 |
| 数据库 | MySQL 8.0 |
| AI 文本 | 阿里云 Qwen |
| AI 图像 | 通义万相 / 火山引擎 Seedream 3.0 |

---

## 架构

```
UI 层 (Qt Widgets)
    │
ViewModel 层
    │
服务层 (NovelService / StoryboardService / ImageService / ...)
    │
API 客户端层 (QwenClient / QwenImageClient / VolcEngineImageClient)
    │
数据层 (DatabaseManager / FileStorage)
```

---

## 项目结构

```
comic/
├── include/                      # 头文件
├── src/
│   ├── api/                      # API 客户端
│   │   ├── QwenClient            # Qwen 文本分析，支持流式 SSE
│   │   ├── QwenImageClient       # 通义万相图像生成
│   │   ├── VolcEngineImageClient # 火山引擎图像生成（Seedream 3.0）
│   │   ├── QwenPromptBuilder     # Prompt 构建与管理
│   │   ├── QwenStreamHandler     # SSE 流式响应处理
│   │   └── QwenStoryboardMerger  # 分镜结果合并
│   ├── app/                      # 应用入口与初始化
│   ├── components/               # 可复用 UI 组件
│   │   ├── PanelCard             # 面板卡片（图片+编辑操作）
│   │   ├── BibleSectionWidget    # 角色圣经章节展示
│   │   ├── AnalysisProgressWidget # AI 分析进度展示
│   │   ├── AnalysisResultWidget  # 分析结果展示
│   │   ├── ChapterCard           # 章节卡片
│   │   ├── StoryboardItem        # 分镜列表项
│   │   └── ...
│   ├── pages/                    # 页面
│   │   ├── LoginPage             # 登录页
│   │   ├── DashboardPage         # 主页/仪表盘
│   │   ├── NovelPage             # 小说列表页
│   │   ├── NovelDetailPage       # 小说详情与分镜编辑页
│   │   ├── CharacterDetailPage   # 角色详情与圣经管理页
│   │   └── ExportPage            # 导出页
│   ├── services/                 # 业务服务层
│   │   ├── NovelService          # 小说 CRUD
│   │   ├── StoryboardService     # 分镜 CRUD
│   │   ├── AnalysisService       # 小说 AI 分析
│   │   ├── BibleGenerator        # 角色圣经生成
│   │   ├── BibleImageService     # 圣经角色图片生成
│   │   ├── ImageService          # 面板图片生成与管理
│   │   ├── ExportService         # 导出（PDF/Webtoon/资源包）
│   │   ├── ChangeRequestService  # 自然语言编辑请求处理
│   │   ├── TaskQueue             # 异步任务队列
│   │   └── ServiceContainer      # 服务依赖注入容器
│   ├── viewmodels/               # ViewModel
│   │   ├── NovelViewModel        # 小说列表状态管理
│   │   └── StoryboardViewModel   # 分镜编辑状态管理
│   ├── models/                   # 数据模型
│   │   ├── Novel                 # 小说
│   │   ├── Character             # 角色
│   │   ├── Storyboard            # 分镜
│   │   ├── Panel                 # 面板
│   │   ├── Bible                 # 角色圣经
│   │   ├── Scene                 # 场景
│   │   ├── Job                   # 后台任务
│   │   ├── Task                  # 异步任务
│   │   └── ChangeRequest         # 编辑请求
│   ├── data/                     # 数据层
│   │   ├── DatabaseManager       # MySQL 数据库操作封装
│   │   ├── DatabaseWorker        # 数据库异步工作线程
│   │   ├── FileStorage           # 本地文件存储
│   │   └── SqlBuilder            # SQL 构建器
│   └── utils/                    # 工具类
│       ├── AppConfig             # 配置文件读取
│       ├── Logger                # 日志工具
│       ├── ExportRenderer        # 导出渲染器
│       ├── ExportUtils           # 导出工具函数
│       ├── ImageColorUtils       # 图片颜色处理
│       ├── ImageBorderTrimmer    # 图片边框裁剪
│       ├── BackgroundWhitener    # 背景白化处理
│       ├── BibleCache            # 圣经数据缓存
│       ├── ChangeRequestIntentUtils  # 编辑意图识别
│       └── AsyncImageLoader      # 异步图片加载
├── sql/                          # 数据库建表与迁移脚本
└── resources/                    # 图标、样式等静态资源
```

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
├── include/        # 头文件
│   ├── api/        # API 客户端
│   ├── components/ # UI 组件
│   ├── services/   # 服务层
│   ├── viewmodels/ # ViewModel
│   └── utils/      # 工具类
├── src/            # 源文件实现
├── sql/            # 数据库建表与迁移脚本
└── resources/      # 图标、样式等静态资源
```

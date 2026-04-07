# 面板详情页面重构计划

## 一、重构目标

重构 PanelCard 组件中的编辑卡片（面板详情）功能，完善图片编辑和生成流程。

## 二、当前问题分析

### 2.1 现有功能
- 主卡片：预览图、编号、描述
- 编辑卡片：场景描述编辑、编辑模式选择、编辑指令输入、遮罩文件选择

### 2.2 存在问题
1. **预览图不同步**：编辑卡片头部的预览图与主卡片预览图未同步
2. **缺少 API 集成**：提交编辑按钮只更新状态文字，未调用实际 API
3. **缺少 ImageService 集成**：未使用已有的 ImageService 进行图片生成
4. **缺少加载状态**：图片生成过程中没有进度反馈
5. **代码耦合度高**：编辑卡片逻辑全部在 PanelCard 中

## 三、重构方案

### 3.1 技术选型

| 方案 | 优点 | 缺点 | 选择 |
|------|------|------|------|
| 保持现状，完善功能 | 改动小 | 耦合度高 | ❌ |
| 拆分 PanelEditorWidget | 职责分离，可复用 | 需要新增文件 | ✅ |

### 3.2 架构设计

```
PanelCard (主卡片)
    │
    ├── 主卡片区域 (现有)
    │   ├── 预览图
    │   ├── 编号标签
    │   └── 描述标签
    │
    └── PanelEditorWidget (新增：编辑卡片组件)
        │
        ├── EditorHeader (头部)
        │   ├── 预览图（同步主卡片）
        │   ├── 面板信息
        │   └── 关闭按钮
        │
        ├── SceneDescSection (场景描述)
        │   ├── 文本编辑框
        │   └── 保存按钮
        │
        ├── EditModeSection (编辑模式)
        │   ├── 局部重绘
        │   ├── 扩展生成
        │   └── 背景替换
        │
        ├── InstructionSection (编辑指令)
        │   ├── 指令输入框
        │   └── 遮罩文件选择
        │
        └── EditorFooter (底部)
            ├── 提交按钮
            └── 状态标签
```

## 四、实现步骤

### 步骤 1：创建 PanelEditorWidget 头文件
- 定义独立的编辑卡片组件类
- 包含编辑模式、预览状态枚举
- 声明信号和槽函数

### 步骤 2：创建 PanelEditorWidget 实现文件
- 实现编辑卡片 UI 构建
- 添加 ImageService 集成
- 实现图片生成逻辑

### 步骤 3：修改 PanelCard
- 使用 PanelEditorWidget 替代原有编辑卡片代码
- 添加预览图同步机制
- 连接信号槽

### 步骤 4：更新 EditorStyles
- 添加编辑卡片相关样式（如需要）

### 步骤 5：更新项目文件
- 在 .pro 文件中添加新文件

## 五、关键代码设计

### 5.1 PanelEditorWidget 信号定义

```cpp
signals:
    void sceneDescriptionChanged(const QString &description);
    void editSubmitted(int editMode, const QString &instruction, const QString &maskPath);
    void imageGenerated(const QString &imageUrl);
    void closed();
```

### 5.2 ImageService 集成

```cpp
void PanelEditorWidget::onSubmitEditClicked()
{
    if (!m_imageService) return;
    
    QString instruction = m_editInstructionEdit->toPlainText();
    
    ImageService::GenerateOptions options;
    options.prompt = instruction;
    options.mode = static_cast<ImageService::GenerateMode>(m_currentEditMode);
    
    m_imageService->generatePanelImage(m_panelId, options);
}
```

### 5.3 预览图同步

```cpp
void PanelCard::setPreviewPixmap(const QPixmap &pixmap)
{
    if (m_previewLabel) {
        m_previewLabel->setPixmap(pixmap.scaled(...));
    }
    // 同步到编辑卡片
    if (m_editorWidget) {
        m_editorWidget->setPreviewPixmap(pixmap);
    }
}
```

## 六、文件变更清单

| 文件 | 操作 | 说明 |
|------|------|------|
| `include/components/PanelEditorWidget.h` | 新增 | 编辑卡片组件头文件 |
| `src/components/PanelEditorWidget.cpp` | 新增 | 编辑卡片组件实现 |
| `include/components/PanelCard.h` | 修改 | 使用 PanelEditorWidget |
| `src/components/PanelCard.cpp` | 修改 | 简化编辑卡片逻辑 |
| `comic.pro` | 修改 | 添加新文件 |

## 七、后续规划

1. 添加图片生成进度动画
2. 支持批量编辑多个面板
3. 添加编辑历史记录
4. 支持撤销/重做操作

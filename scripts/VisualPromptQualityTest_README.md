# VisualPrompt质量测试工具

这个工具用于测试Qwen生成的visualPrompt是否符合规范。

## 📁 文件说明

| 文件 | 语言 | 说明 |
|------|------|------|
| `VisualPromptQualityTest.cpp` | C++ | C++版本测试工具（需要编译）|
| `VisualPromptQualityTest.py` | Python | Python版本测试工具（可直接运行）|

## 🚀 快速开始

### 方法1：使用Python版本（推荐）

```bash
cd e:\QingningComic\qingning\comic\scripts
python VisualPromptQualityTest.py
```

**优点**：
- ✅ 不需要编译
- ✅ 可以直接运行
- ✅ 易于修改和扩展

---

### 方法2：使用C++版本

```bash
# 需要将文件添加到项目编译
# 或使用独立的C++编译器
g++ VisualPromptQualityTest.cpp -o test
./test
```

---

## 📊 测试内容

### 测试项

| 测试项 | 说明 | 示例 |
|--------|------|------|
| **场景位置** | 检查是否使用模糊词 | ❌ "门口" → ✅ "从店内朝向玻璃门" |
| **人物位置** | 检查是否使用抽象表达 | ❌ "在画面中央" → ✅ "on left side" |
| **对话元素** | 检查是否包含气泡 | ✅ "speech bubble with text: \"xxx\"" |
| **光影描述** | 检查是否具体 | ❌ "柔光" → ✅ "soft light from window" |
| **构图语言** | 检查是否使用几何语言 | ✅ "diagonal composition" |
| **文学表达** | 检查是否使用文学性表达 | ❌ "立于" → ✅ "standing in" |

---

## 🎯 测试用例

### 测试用例1：当前版本（预期失败）

```
文艺少女漫画风格，拾光书店门口，铜风铃轻晃，门外绿意朦胧，
白发老人佝偻立于逆光门框中，攥旧布包，眼神急切；
年轻女子坐于柜台后，已放下薄荷糖，微微前倾，目光温静，
两人视线在画面中央交汇，柔光漫溢，空气中有飘浮栀子花瓣
```

**预期结果**：
- ❌ 场景位置模糊："门口"
- ❌ 文学性表达："立于"
- ❌ 光影描述抽象："逆光"、"柔光漫溢"
- ❌ 人物位置抽象："视线交汇"
- ❌ 缺少对话气泡

---

### 测试用例2：优化版本（预期通过）

```
文艺少女漫画风格，拾光书店内部从柜台视角朝向玻璃门方向，
白发老人（陈伯）standing at entrance, visible from inside,
身穿洗旧藏青布衫，body leaning forward 20 degrees, anxious expression;
年轻女子（青柠）sitting behind counter on left side,
body turning right facing old man, concerned expression;
two characters in L-shaped composition,
speech bubble: "爷爷，您找什么书？" said by 青柠;
natural light from glass door creating warm tones
```

**预期结果**：
- ✅ 场景位置明确："从柜台视角朝向玻璃门方向"
- ✅ 人物位置具体："on left side"、"standing at entrance"
- ✅ 包含对话气泡："speech bubble"
- ✅ 光影具体："natural light from glass door"
- ✅ 构图明确："L-shaped composition"

---

## 📈 如何添加新的测试用例

### Python版本

在 `VisualPromptQualityTest.py` 中添加：

```python
test_cases.append(TestCase(
    name="测试用例X：你的测试名称",
    visual_prompt="你的visualPrompt内容",
    expected_scene_position="期望的场景位置关键词",
    expected_character_position="期望的人物位置关键词",
    has_dialogue=True/False  # 是否有对话
))
```

---

### C++版本

在 `VisualPromptQualityTest.cpp` 中添加：

```cpp
TestCase caseX;
caseX.name = QString::fromUtf8("测试用例X：你的测试名称");
caseX.visualPrompt = QString::fromUtf8("你的visualPrompt内容");
caseX.expectedScenePosition = QString::fromUtf8("期望的场景位置关键词");
caseX.expectedCharacterPosition = QString::fromUtf8("期望的人物位置关键词");
caseX.hasDialogue = true;  // 或 false
testCases.append(caseX);
```

---

## 🔧 如何扩展测试规则

### 添加新的检查项

在 `VisualPromptQualityChecker` 类中添加新的检查方法：

```python
@staticmethod
def check_new_rule(visual_prompt: str) -> TestResult:
    """检查新规则"""
    # 你的检查逻辑
    if "问题关键词" in visual_prompt:
        return TestResult(
            passed=False,
            issue="问题描述",
            suggestion="改进建议"
        )
    return TestResult(passed=True)
```

然后在 `check_visual_prompt` 方法中调用：

```python
results.append(VisualPromptQualityChecker.check_new_rule(visual_prompt))
```

---

## 📝 输出示例

```
============================================================
VisualPrompt质量测试工具
============================================================

【测试用例1：青柠第一章面板1-2（当前版本）】
VisualPrompt: 文艺少女漫画风格，拾光书店门口，铜风铃轻晃，门外绿意朦胧，白发老人佝偻立于逆光门框中...

  ❌ 问题：场景位置描述模糊：'门口'
     建议：建议改为：'从店内朝向玻璃门方向' 或 'sitting behind counter facing door'

  ❌ 问题：使用文学性表达：'立于'
     建议：建议改为：'standing in' 或 'looking at'

  ❌ 问题：光影描述抽象：'逆光'
     建议：建议改为：'soft light from left window' 或 'natural light from glass door'

测试结果：1 通过，5 失败
------------------------------------------------------------

【测试用例2：优化后的版本（预期）】
VisualPrompt: 文艺少女漫画风格，拾光书店内部从柜台视角朝向玻璃门方向，白发老人（陈伯）standing at...

测试结果：6 通过，0 失败
------------------------------------------------------------

测试完成！

总结：
1. 测试用例1（当前版本）应该有多个失败项
2. 测试用例2（优化版本）应该全部通过
3. 测试用例3（简单场景）应该全部通过
```

---

## 🎯 使用建议

1. **测试现有数据**：先用当前生成的visualPrompt运行测试，了解问题
2. **优化System Prompt**：根据测试结果调整Qwen的System Prompt
3. **重新生成**：重新生成分镜，再次测试
4. **对比效果**：对比修改前后的测试结果

---

## 💡 常见问题

### Q: 测试用例2也失败了怎么办？

A: 检查测试规则是否过于严格，或者优化后的visualPrompt还有问题。

### Q: 如何添加更多测试规则？

A: 在 `VisualPromptQualityChecker` 类中添加新的检查方法。

### Q: 测试结果如何保存？

A: 可以将输出重定向到文件：
```bash
python VisualPromptQualityTest.py > test_results.txt
```

---

## 📚 相关文档

- [QwenPromptBuilder.cpp](../src/api/QwenPromptBuilder.cpp) - System Prompt配置
- [PromptBuilder.cpp](../src/utils/PromptBuilder.cpp) - 映射表定义
- [storyboard.json](../schemas/storyboard.json) - Schema定义

---

**创建日期**：2026-05-08  
**作者**：青柠漫画项目  
**版本**：1.0

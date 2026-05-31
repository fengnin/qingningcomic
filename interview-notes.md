# 青柠漫画 — 面试复习提纲

---

## 模块一：自然语言修改请求链路

### 整体流程

用户输入一句自然语言（如"把第3个面板的背景改成夜晚"），系统经过两次 AI 调用完成图片编辑：

| 调用 | 说明 |
|------|------|
| 第一次 | 文本 AI（Qwen）把自然语言解析成结构化 DSL |
| 第二次 | 图像 AI（通义万相 / 火山引擎）根据 DSL 生成编辑后的图片 |

---

### 1. 用户输入 & 面板定位

**问题**
用户输入的是自然语言，系统需要知道改的是哪个面板。

**方案**
NovelDetailPage 调用 ChangeRequestTargetUtils::extractRequestedPanelNumber() 从文本里提取面板号，再查出对应的 panelId，连同 storyboardId、chapterNumber 一起打包成上下文，写入 change_requests 表（status=queued）。

**细节（追问）**
写入数据库后立刻用 QtConcurrent::run() 在后台线程异步执行，不阻塞 UI。

---

### 2. 自然语言 → DSL（第一次 AI 调用）

**问题**
图像编辑 API 不能直接接受自然语言，需要结构化的操作指令。

**方案**
`ChangeRequestService::parseNaturalLanguage()` 分四步完成解析。

首先 `ChangeRequestParseUtils::buildChangeRequestSchema()` 动态生成一份 JSON Schema 作为约束，`required` 只有 `scope/type/ops` 三个字段，`params` 故意不约束内部结构——不同 action 差异太大，约束反而干扰 AI，字段校验放到后续归一化步骤里做。

然后 `QwenClient::parseChangeRequest()` 把用户输入、schema 和 system prompt 一起发给 Qwen，返回结构化 JSON，例如：
```json
{
  "scope": "panel",
  "type": "art",
  "ops": [{ "action": "bg_swap", "params": { "background": "夜晚城市" } }]
}
```

LLM 偶尔输出截断或格式不规范的 JSON，`QwenJsonRepair::parseWithRepair()` 先尝试直接解析，失败则修复截断、补全括号等，再交给 `ChangeRequestDsl::fromJson()` 反序列化。`fromJson` 内部做了字段别名兼容（`action`/`operation`/`name` 都能识别），防止 AI 用了不同字段名。

最后归一化和目标解析：`normalizeParsedDsl()` 统一 action 名称大小写、修正拼写变体；`normalizeChangeRequestExpression()` 把表情词（"微笑"、"smile"、"happy"）映射到内部枚举值；`normalizeChangeRequestArtEdits()` 补全 AI 漏填的 editIntent 字段；`resolveDslTarget()` 把 context 里的 targetPanelId 填入 `dsl.targetId`。最终得到一个完整的 `ChangeRequestDsl` 对象，targetId 已指向具体面板。

**细节（追问）**
scope 目前只实现了 panel。type 有 4 种：art（图像编辑）、dialogue（台词编辑）、layout（布局调整）、style（风格修改）。action 有 9 种：bg_swap（背景替换）、inpaint（局部重绘）、repose（姿态调整）、regen_panel（重新生成）、setExpression（表情修改）、add_effect（添加特效）、resize（调整尺寸）、台词增删改、reorder（面板排序）。

---

### 3. DSL → 图像编辑 prompt（ChangeRequestIntentUtils）

**问题**
不同 action 类型需要构建不同结构的图像编辑 prompt，逻辑复杂。

**方案**
ChangeRequestIntentUtils 根据 action 类型构建四段式 prompt：
- 局部编辑目标：改的是图里哪个部分（如"背景"）
- 允许变化：只能改什么（如"仅修改背景环境"）
- 变化目标：改成什么样（根据用户输入动态填入）
- 绝对约束：固定文本，所有未提及内容一律不得改动

**细节（追问）**
以 bg_swap 为例：从 params 里提取 background 字段作为变化目标，约束里写死"保持人物、构图、透视、镜头、光线、色调和画幅比例不变"，防止 AI 改动不该改的部分。

`editIntent` 字段写入 panel content 后，下游 PromptBuilder 看到它就走精简模式，只用 `visualPrompt`，不拼接角色/场景描述，防止稀释局部编辑指令。

所有从 `ChangeRequestService` 出去的生成请求，`runPanelImageGeneration()` 统一默认 `forceProvider = "qwen"`，走千问编辑模型，不需要在每个 action 里单独设置。

---

### 4. 图像编辑（第二次 AI 调用）& 结果写回

**问题**
图像编辑需要原图 + prompt，结果要持久化并刷新 UI。

**方案**
ImageService::regeneratePanelImage() 把原图（Base64）和构建好的 prompt 发给图像 API，拿到编辑后的图片后上传到存储服务，把新的 image_url 写回 panels 表，更新 change_requests 状态为 completed，emit changeRequestCompleted 信号，UI 收到信号后重新加载面板。

**细节（追问）**
生成前备份 `originalPanel`，生成失败后 `restorePanelAfterFailedArtEdit()` 把面板数据恢复原状，防止 panel 内容被污染。

---

### 实际运行案例

用户输入：**"把第1个面板中的角色改成站着的姿势"**

**① 面板定位**
- `extractRequestedPanelNumber()` 用正则从文本里提取出数字 `1`
- 遍历内存中的面板列表 `m_currentPanels`，用公式 `(page-1)*6 + panel_index + 1` 算出每个面板的编号
- page=1、index=0 → 编号 = 1，匹配，取出该面板的 UUID 作为 panelId
- 把 storyboardId、chapterNumber=1、targetPanelNumber=1、panelId 打包成 context，写入 change_requests 表，status=queued

> 面板编号不存数据库，数据库只存 page 和 panel_index，编号每次按公式推算。这样面板顺序调整时只需要改 index，不用批量更新编号字段。

**② Qwen 解析 DSL（第一次 AI 调用）**
- 把自然语言 + JSON Schema 约束发给 Qwen
- Qwen 返回：
```json
{ "scope": "panel", "type": "art", "ops": [{ "action": "repose", "params": { "prompt": "角色改为站立姿势" } }] }
```
- 归一化后识别出 editIntent = `change_pose`，把 panelId 填入 dsl.targetId，DSL 完整可执行

**③ 构建图像编辑 prompt**
- action = repose，走四段式结构：
  - 局部编辑目标：中的角色
  - 允许变化：仅修改角色姿势
  - 变化目标：角色改为站立姿势
  - 绝对约束：其余所有内容（服装、表情、背景、构图、其他人物）保持不变

**④ 图像编辑（第二次 AI 调用）& 写回**
- 读取原图（1024×1024），连同 prompt 发给千问图片编辑 API
- 约 8 秒后返回新图，覆盖保存到原路径
- panels 表写入新图片路径，change_requests 状态改为 completed
- emit 信号，UI 刷新面板预览

---

### 面试官常问问题

**Q：用户输入一句话，系统是怎么处理的？**
先从文本里提取目标面板号，查出 panelId，把请求写入数据库（status=queued），然后后台线程异步执行。执行分两步：第一步调用 Qwen 把自然语言解析成 DSL（scope/type/action/params），第二步根据 action 类型构建图像编辑 prompt，连同原图一起发给图像 AI，拿到结果写回数据库，通知 UI 刷新。

**Q：为什么要两次 AI 调用，不能一次搞定吗？**
文本 AI 和图像 AI 是两个完全不同的模型，职责不同。文本 AI 擅长理解意图、输出结构化指令；图像 AI 只接受图片 + prompt，不理解自然语言里的"第3个面板"这种上下文。中间加一层 DSL 的好处是：意图解析和图像生成解耦，DSL 可以复用（同一个 DSL 可以换不同图像引擎执行），也方便调试（能看到 AI 理解了什么）。

**Q：DSL 是什么，为什么要设计这一层？**
DSL（Domain Specific Language，领域特定语言）是一个结构化的操作描述，包含 scope（改哪个范围）、type（改什么类型）、action（具体操作）、params（操作参数）。设计这一层是因为图像编辑 prompt 的构建逻辑很复杂，不同 action 需要不同的约束文本，如果直接把自然语言传给图像 AI，约束控制不住，AI 容易改动不该改的部分。DSL 把"理解意图"和"构建 prompt"分开，每一步职责清晰。

**Q：图像编辑 prompt 怎么保证 AI 只改你想改的部分？**
用四段式结构：局部编辑目标（改哪里）、允许变化（只能改什么）、变化目标（改成什么样）、绝对约束（其余一律不动）。以背景替换为例，约束里写死"保持人物、构图、透视、镜头、光线、色调和画幅比例不变"，把不能动的东西全部锁死，AI 只能在允许的范围内修改。

**Q：为什么用数据库持久化 change_requests，直接内存里跑不行吗？**
持久化有两个好处：一是应用崩溃或重启后任务不丢失，用户不需要重新提交；二是可以查历史记录，知道每个面板改过什么、改了几次、有没有失败。如果只放内存，进程一退出就全没了。

**Q：Schema 的 params 为什么不约束内部结构？**
不同 action 的 params 差异太大，全部枚举进 schema 会让 prompt 变得很长，反而干扰 AI。字段校验放到归一化步骤里做。

---

## 模块二：数据库设计（10 张表）

### 开场怎么说

这个项目我设计了 10 张表，核心思路是以 novels 为中心，所有业务数据都通过 novel_id 串联。设计时主要解决三个问题：一是 AI 任务耗时长，需要持久化任务队列支持断点续传；二是角色和场景的结构不固定，用 JSON 字段灵活存储；三是多线程并发写库，需要连接池和事务保证安全。

---

### 各表设计要点

#### users

密码用 SHA-256 哈希存 `password_hash`，不存明文——数据库泄露后哈希无法反推原始密码。`is_online` 字段在登录/退出时由 `AppInitializer` 维护。

---

#### novels

这张表是整个系统的入口。`status` 字段驱动全流程状态机：uploading → analyzing → storyboard_generating → ready，每个阶段对应一个异步任务，失败时停在对应状态方便定位。

`original_text` 和 `original_text_path` 并存——短文本直接存字段方便查询，长文本只存文件路径，避免大字段撑大数据库行。`storyboard_id` 指向当前激活的分镜版本，支持多版本切换。

---

#### storyboards / panels

storyboards 用 `chapter_number` 支持多章节，`version` 字段支持重新生成时保留旧版本。

panels 是系统里数据量最大的表。`content` 用 JSON 存对白/旁白/动作等结构化内容，`visual_prompt` 单独一列——因为图片生成时只需要 visual_prompt，单独列出来避免每次都解析 JSON。图路径分 `preview_image_path` 和 `hd_image_path` 两个字段，先生成低分辨率预览（速度快），用户确认后再生成高清，节省 API 调用成本。

---

#### jobs

图片生成单张要 10~30 秒，批量生成必须异步。我用 jobs 表做持久化任务队列，而不是放内存——放内存的话应用一崩溃任务就全丢了，用户得重新提交。

状态机：pending → running → completed / failed。失败后最多重试 3 次，错误信息写入 `error_message`。UI 定时器每 500ms 查一次 jobs 表刷新进度条。jobs 表横跨三类任务：AI 分析、图片生成、导出，统一管理。

---

#### change_requests

`natural_language` 存用户原始输入，`dsl` 存 AI 解析后的结构化结果，两个都保留。原因是出问题时需要对比——如果只存 DSL，不知道 AI 是不是理解错了用户的意图；只存自然语言，又没法直接重跑。两者都有，调试和重试都方便。

---

#### characters / character_portrait_versions

角色的 `appearance` 和 `personalities` 用 JSON 存，因为外观维度多（发色、发型、眼睛、服装……），用 JSON 比单独建列灵活，AI 输出什么结构就存什么结构。

肖像版本表用 `version_no` 递增记录每次编辑，`field_diff` 存这次相对上一版本改了哪些字段（比如"发色从黑色改为金色"），不需要对比两个完整快照就能知道改了什么。`characters` 表里的 `current_portrait_version_id` 指向当前激活版本，查询时 LEFT JOIN 直接取，不用每次查最新版本。

删除角色时先清 `character_portrait_versions` 和肖像文件，再删 `characters` 本身，整个操作包在事务里，删完后使 BibleCache 失效，避免圣经缓存残留旧数据。

---

#### scenes

`details` 用 JSON 存氛围/天气/色调等信息，结构和角色外观一样不固定，JSON 更合适。删除场景后同样使 BibleCache 失效。

---

#### exports

`url_expires_at` 管理下载链接有效期——下载链接是临时签名 URL，过期自动失效，防止链接被无限期传播。导出走异步，任务写入 jobs 表，完成后回写 exports 表的状态和文件路径。

---

### 追问准备

**JSON 字段这么多，查询性能怎么保证？**
JSON 字段只存结构不固定的内容（外观、性格、场景详情），核心查询字段（id、status、novel_id）都是普通列并加了索引，不走 JSON 查询，性能没有问题。

**并发安全怎么保证？**
DatabaseManager 实现线程安全连接池，每个线程持有独立连接，不共享。TransactionScope 用 RAII 管理事务，析构时自动回滚，防止忘记回滚。所有 SQL 用 `QSqlQuery::bindValue` 参数化查询，杜绝 SQL 注入。

**删除 novel 时关联数据怎么处理？**
外键设了 `ON DELETE CASCADE`，删 novel 时数据库自动级联删掉 storyboards、panels、characters、scenes、change_requests、exports，不需要手动逐表清理。

---

## 模块三：上传小说链路

---

## 1. SSE 流式调用

**问题**
AI 生成分镜要十几秒，如果等全部生成完再返回，用户界面会一直卡着没有反馈，体验很差。

**方案**
用 SSE 流式接口，请求加 stream=true，服务端每生成几个 token 就推一帧过来，客户端用 Qt 的 readyRead 信号边收边处理，用户能看到文字逐渐出现，有实时进度感。

**细节（追问）**
TCP 不保证每次收到的恰好是一个完整事件，所以用缓冲区加 \n\n 分隔符来切割；流结束后用 QwenJsonRepair 修复可能截断的 JSON，再交给分镜服务写库。

---

## 2. 指数退避重试

**问题**
调用 Qwen API 偶尔会失败，直接重试可能让情况更糟。

**方案**
失败后等待时间翻倍再重试：1s → 2s → 4s，上限 30s，最多重试 3 次。

**细节（追问）**
API 失败最常见原因是服务端过载（429 Too Many Requests），立刻重试相当于在服务端最忙的时候继续砸请求；等待翻倍给服务端喘息机会，等它恢复了再试。实现在 RetryPolicy 类里，用 qPow(2.0, attempt) 计算延迟，QThread::msleep 阻塞等待。

---

## 3. 长文本分块

**问题**
小说文本可达数万字，超出 Qwen API 单次 token 限制（约 8000 字）。

**方案**
splitTextIntelligently() 按段落边界切分，每块不超过 8000 字；超长段落再按句子二次切分。逐块调用 API，最后用 QwenStoryboardMerger::mergeStoryboards() 合并所有块的输出，重新计算页码（每页 6 个面板）。

**细节（追问）**
第一块携带完整的角色/场景圣经上下文，后续块只带空数组，避免每块都重复传大量上下文浪费 token。

---

## 4. Job 异步队列（图片生成）

**问题**
图片生成单张耗时 10~30 秒，批量生成会阻塞 UI。

**方案**
用 jobs 表做持久化任务队列。UI 点击批量生成时，为每个面板写入一条 jobs 记录（status=pending）；Worker 线程轮询取出 pending job，调用图像 API，实时更新进度；UI 定时器每 500ms 查询 jobs 表刷新进度条。

**细节（追问）**
任务状态机：pending → running → completed / failed。失败后最多重试 3 次，错误信息写入 jobs 表，支持用户手动重试。持久化到数据库的好处是应用重启后任务不丢失。

---

## 5. 火山引擎 AWS Signature V4 签名

**问题**
火山引擎不用简单的 Bearer Token，而是要求 AWS Signature V4（HMAC-SHA256），鉴权逻辑复杂。

**方案**
按规范实现签名流程：规范化 URI/请求头/查询参数 → 计算 body SHA256 哈希 → 拼 StringToSign → HMAC-SHA256 生成签名 → 注入 Authorization 头。

**细节（追问）**
上层 ImageService 只调用统一接口，不感知底层鉴权差异。通义万相用 Bearer Token，火山引擎用 V4 签名，两套逻辑封装在各自的 Client 里，可按配置热切换，互为备份。

---

## 6. 自然语言修改请求 — 完整链路（代码级）

### 调用链

```
NovelDetailPage（UI）
  ↓ 用户提交自然语言
createChangeRequest(novelId, naturalLanguage, context)   [ChangeRequestService.cpp:915]
  ├─ extractRequestedPanelNumber() 从文本提取目标面板号
  ├─ resolveStoryboardIdForChangeRequest() 补全 storyboardId / targetPanelId
  ├─ buildDraft() 构建草稿，status = "queued"
  ├─ m_db->insert() 写入 change_requests 表
  └─ emit changeRequestCreated(cr)

executeChangeRequestAsync(crId)   [ChangeRequestService.cpp:1183]
  └─ QtConcurrent::run() 异步触发，不阻塞 UI

executeChangeRequest(crId)   [ChangeRequestService.cpp:1015]
  ├─ writeChangeRequestStatus("in_progress")   [第1034行]
  ├─ emit progressChanged(10%)
  ├─ prepareChangeRequestDsl(cr)   [第1044行]
  │    └─ parseNaturalLanguage()
  │         ├─ buildChangeRequestSchema() 生成 JSON Schema
  │         ├─ QwenClient::parseChangeRequest() 调 Qwen 文本 API
  │         ├─ QwenJsonRepair::parseWithRepair() 容错解析
  │         ├─ ChangeRequestDsl::fromJson() 反序列化（字段别名兼容）
  │         ├─ normalizeParsedDsl() 统一 action 名称
  │         ├─ normalizeChangeRequestExpression() 表情词映射枚举
  │         ├─ normalizeChangeRequestArtEdits() 补全 editIntent
  │         └─ resolveDslTarget() 填入 targetId
  │    DSL 写回 change_requests 表，status = "pending"
  ├─ emit progressChanged(25%)
  ├─ executeDslOperations(crId, dsl)   [第1053行]
  │    └─ executeOperationByType() 按 type 分发
  │         ├─ type="art"      → executeArtOperation()
  │         │    ├─ 更新 panels.content["visualPrompt"]
  │         │    └─ runPanelImageGeneration() 触发图片重生成
  │         ├─ type="dialogue" → executeDialogueOperation()
  │         │    ├─ 更新 panels.content["dialogue"] + visual_prompt
  │         │    └─ runPanelImageGeneration() 触发图片重生成
  │         └─ type="layout"   → executeLayoutOperation()
  │              └─ updatePanelLayout() 只更新尺寸，不重生成图片
  ├─ emit progressChanged(95%)
  ├─ writeChangeRequestStatus("completed")   [第1069行]
  └─ emit changeRequestCompleted(crId, result)   [第1076行]
  （异常时）→ writeChangeRequestStatus("failed") + emit changeRequestFailed
```

---

### 状态流转

```
queued → in_progress → pending（DSL写回后）→ completed / failed
```

| 状态 | 设置位置 | 触发条件 |
|------|---------|---------|
| `queued` | `buildDraft()`，createChangeRequest 写库时 | 记录创建 |
| `in_progress` | `executeChangeRequest` 第1034行 | 开始执行 |
| `pending` | `saveChangeRequestDsl` 第1854行 | DSL 解析完成写回 |
| `completed` | `executeChangeRequest` 第1069行 | 全部操作执行成功 |
| `failed` | `executeChangeRequest` 第1081行 | 任意步骤抛异常 |

---

### runPanelImageGeneration 关键逻辑

```cpp
// ChangeRequestService.cpp:2059
if (effectiveHint.forceProvider.isEmpty()) {
    effectiveHint.forceProvider = "qwen";  // 无条件默认走千问图片编辑
}
```

- `forceProvider="qwen"` → `isEditOperation=true` → 走千问图片编辑 API
- layout 路径不调用此函数，直接更新数据库字段

---

### 容错设计

- **JSON 修复**：`QwenJsonRepair` 处理 Qwen 输出截断、括号不完整等问题
- **字段别名兼容**：`fromJson` 同时识别 `action`/`operation`/`name`，防止 AI 用不同字段名
- **失败回滚**：art 操作生成前备份 `originalPanel`，失败后 `restorePanelAfterFailedArtEdit()` 恢复面板数据
- **异常全捕获**：`executeChangeRequest` 用 try-catch 包住全流程，任何步骤失败都写 `failed` 状态并 emit 信号

---

## 8. 自然语言修改请求 — 完整链路（流程图）

```
用户输入自然语言
（如"把第3个面板的背景改成夜晚"）
        │
        ▼
┌─────────────────────────────────────────┐
│  面板定位                                │
│  · 从文本提取目标面板号                   │
│  · 查出 panelId / storyboardId          │
│  · 原始文本写入 change_requests 表        │
│    status = queued                       │
└─────────────────┬───────────────────────┘
                  │
                  │  QtConcurrent::run()
                  │  后台线程异步执行，不阻塞 UI
                  ▼
┌─────────────────────────────────────────┐
│  第一次 AI 调用：自然语言 → DSL           │
│                                         │
│  · 生成 JSON Schema 约束输出结构          │
│  · 调用 Qwen 文本模型解析意图             │
│  · 容错：修复截断 JSON、字段别名兼容       │
│  · 归一化：统一 action 名称、表情词映射    │
│  · 填入 targetId，得到完整 DSL           │
│                                         │
│  DSL 结构：                              │
│    scope  → panel（当前只支持面板级）      │
│    type   → art / dialogue / layout     │
│    action → bg_swap / inpaint / ...     │
│    params → 操作参数（背景描述等）         │
│                                         │
│  DSL 写回 change_requests 表             │
│  status = pending                       │
└─────────────────┬───────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────┐
│  DSL 验证 & 按 type 分发                 │
└──────┬──────────────┬────────────┬──────┘
       │              │            │
       ▼              ▼            ▼
  type=art       type=dialogue  type=layout
       │              │            │
  更新面板          更新面板       更新面板
  visualPrompt     dialogue      尺寸字段
  字段              + visual      （仅数据库）
       │            _prompt        │
       │              │            │
       ▼              ▼            │
┌──────────────────────────┐       │
│  构建图像编辑 Prompt       │       │
│                          │       │
│  四段式结构：              │       │
│  · 局部编辑目标（改哪里）   │       │
│  · 允许变化（只能改什么）   │       │
│  · 变化目标（改成什么样）   │       │
│  · 绝对约束（其余不得动）   │       │
│                          │       │
│  editIntent 写入 panel    │       │
│  content，下游 Prompt     │       │
│  Builder 走精简模式        │       │
└────────────┬─────────────┘       │
             │                     │
             ▼                     │
┌─────────────────────────────────────────┐
│  第二次 AI 调用：图像编辑                 │
│                                         │
│  · forceProvider = "qwen"（默认）        │
│  · 原图（Base64）+ prompt → 千问图片编辑  │
│  · 生成前备份原始面板数据                 │
│  · 失败时恢复备份，防止数据污染            │
└─────────────────┬───────────────────────┘
                  │                   ↑
                  │              layout 路径
                  │              直接到这里
                  ▼
┌─────────────────────────────────────────┐
│  结果写回 & 通知 UI                      │
│                                         │
│  · 新图片路径写回 panels 表               │
│  · change_requests.status = completed   │
│  · emit changeRequestCompleted          │
│  · UI 收到信号，重新加载面板图片           │
└─────────────────────────────────────────┘

失败时任意步骤抛异常：
  → status = failed
  → emit changeRequestFailed
  → art 操作自动恢复备份数据
```

### 状态流转

```
queued → in_progress → pending → completed
                                ↘ failed
```

| 状态 | 含义 |
|------|------|
| `queued` | 记录已创建，等待执行 |
| `in_progress` | 后台线程已接管，正在处理 |
| `pending` | DSL 解析完成，等待执行操作 |
| `completed` | 全部操作成功，图片已更新 |
| `failed` | 任意步骤失败，原始数据已恢复 |

### 关键设计决策

| 决策 | 原因 |
|------|------|
| 两次 AI 调用而非一次 | 文本模型和图像模型职责不同，DSL 层解耦意图理解与图像生成 |
| JSON Schema 约束 Qwen 输出 | 保证输出可直接解析，减少修复成本 |
| params 不约束内部结构 | 不同 action 差异太大，过度约束反而干扰 AI |
| change_requests 持久化 | 崩溃恢复、历史追溯、调试对比（原始语言 vs DSL） |
| layout 不触发图片重生成 | 布局只改尺寸，不涉及图像内容，无需重新生成 |
| art 操作备份原始数据 | 图像生成失败率不为零，防止面板内容被污染 |

---

## 7. 数据库连接池 + RAII 事务

**问题**
多线程环境下共享同一个数据库连接会有竞态问题；事务忘记回滚会导致数据不一致。

**方案**
DatabaseManager 实现线程安全连接池，每个线程获取独立连接。TransactionScope 用 RAII 管理事务，析构时自动回滚未提交的事务，避免忘记回滚。

**细节（追问）**
所有 SQL 用 QSqlQuery::bindValue 参数化查询，禁止字符串拼接，杜绝 SQL 注入。DatabaseWorker 将耗时 DB 操作派发到专用线程，避免主线程卡顿。

---

## 8. 自然语言修改请求 — 完整链路详解（真实案例）

以用户输入 `把第1个面板中的角色改成站着的姿势` 为例，完整走一遍。

---

### 第一步：用户提交请求 & 面板定位

用户点击提交，触发 `onSubmitChangeRequestClicked()`，做三件事：

1. 从输入框读取自然语言文本
2. 调 `extractRequestedPanelNumber()` 从文本里提取面板编号 `1`，再遍历当前页面已加载的面板列表，通过 `page + index` 计算出面板编号，找到对应 panelId `9c3255f0`
3. 打包上下文：
```json
{
  "storyboardId": "0c59e17b",
  "chapterNumber": 1,
  "targetPanelNumber": 1,
  "targetPanelId": "9c3255f0"
}
```

字段含义：storyboardId 是第1章的数据库 ID，chapterNumber 是章节编号，targetPanelNumber 是格子编号，targetPanelId 是格子的数据库 ID。

兜底逻辑：如果文本里没有面板编号，但用户在界面上点选了某个面板，就用选中的面板作为目标。

---

### 第二步：创建请求记录

把请求写入 `change_requests` 表，分配唯一 ID `5242761c`，状态设为 `queued`。这一步只是登记存档，不做任何 AI 调用。目的：保证请求不丢失，UI 可以通过这个 ID 实时查询进度。

---

### 第三步：异步执行，状态流转

请求丢进 `QtConcurrent::run()` 异步线程，状态从 `queued` → `in_progress`。

为什么异步：Qwen 文本解析约等 2 秒，图像编辑约等 8 秒，同步执行会卡死 UI。

---

### 第四步：自然语言 → 结构化 DSL（最核心）

**4a. 构建发给 Qwen 的内容**

- 系统 prompt：角色设定 + 规则（type/action 对应关系、绝对规则"用户没提到的内容一律不改"等）
- 用户 prompt：原始输入 + 上下文 + 各字段填写规则
- JSON Schema：约束 Qwen 只能输出合法枚举值

**4b. 验证 Qwen 返回的 DSL**

`isValidParsedDsl()` 校验：scope/type 必须是合法枚举值，targetId 不能为空，ops 不能为空，action 必须和 type 匹配。校验失败则重新调 Qwen 解析。

Qwen 返回毛坯 DSL：
```json
{
  "scope": "panel",
  "type": "art",
  "ops": [{"action": "repose", "params": {
    "prompt": "角色改为站立姿势，其余所有内容（包括服装、表情、背景、构图、其他人物）保持不变"
  }}]
}
```

**4c. 两轮归一化补全**

第一轮（`normalizeParsedDsl`）：用 Qwen 生成的 prompt 文本推断 `editIntent`。

**这里有一个 bug**：prompt 里提到"背景"会命中 `pureBackgroundKeywords`，误判为 `replace_background`。根本原因是括号里的"保持不变"内容也参与了关键词匹配。修复方案：在 `inferEditIntentFromText` 开头用正则剥离括号内容，再做关键词匹配：
```cpp
stripped.remove(QRegularExpression(QString::fromUtf8("[（(][^）)]*[）)]")));
```

第二轮（`normalizeChangeRequestArtEdits`）：用原始自然语言覆盖推断，更可靠：
- `editIntent` ← "姿势"命中 `poseKeywords`，得到 `change_pose` ✓
- `sourceText` ← 原始自然语言
- `maskRegion` ← 提取"角色"（要编辑的区域）
- `targetId` ← 从上下文注入 `9c3255f0`

为什么要两次推断：Qwen 生成的 prompt 里会提到"保持不变"的内容（如"背景保持不变"），容易误判；原始自然语言只描述用户真正想改的东西，更可靠，所以第二次覆盖第一次。

最终完整 DSL：
```json
{
  "scope": "panel", "type": "art",
  "targetId": "9c3255f0",
  "ops": [{"action": "repose", "params": {
    "editIntent": "change_pose",
    "maskRegion": "中的角色",
    "prompt": "角色改为站立姿势，其余所有内容保持不变",
    "sourceText": "把第1个面板中的角色改成站着的姿势"
  }}]
}
```

---

### 第五步：按 type 路由到执行器

`type=art` → `executeArtOperation`，`action=repose`，`editIntent=change_pose` 告诉执行器用局部编辑策略。

`editIntent` 还有一个下游作用：`PromptBuilder` 看到它存在就切换精简模式，只用 `visualPrompt`，不拼接角色外貌、场景描述等圣经内容。原因：局部编辑需要 prompt 精准聚焦，拼太多内容会让图像 AI 改动不该改的部分。

---

### 第六步：构建图像编辑请求

- 正向 prompt：描述要改成什么样
- 负向 prompt：排除写实、黑白、模糊等不想要的效果
- 参考图：加载 `panels/9c3255f0/hd.png`（原图 1024×1024）

---

### 第七步：调千问图像编辑 API

原图 + prompt 发给千问，等待约 8 秒返回新图。

---

### 第八步：保存结果，刷新 UI

新图覆盖保存为 `hd.png` 和 `preview.png`，状态写回 `completed`，UI 重新加载全部面板预览图。

---

## 9. 数据库表结构（9 张表）

所有表主键均为 VARCHAR(36) UUID，所有业务表通过 `novel_id` 串联到 novels 表。

### users（用户表）
| 字段 | 类型 | 说明 |
|------|------|------|
| id | VARCHAR(36) | 主键 |
| username | VARCHAR(50) | 唯一用户名 |
| email | VARCHAR(100) | 唯一邮箱 |
| password_hash | VARCHAR(255) | SHA-256 哈希，不存明文 |
| is_online | TINYINT(1) | 在线状态 |

### novels（作品表）
| 字段 | 类型 | 说明 |
|------|------|------|
| id | VARCHAR(36) | 主键 |
| user_id | VARCHAR(36) | 外键 → users |
| title | VARCHAR(200) | 作品标题 |
| original_text | TEXT | 原始小说文本 |
| original_text_path | VARCHAR(255) | 文本文件路径 |
| status | VARCHAR(20) | 状态机：created / analyzing / ready 等 |
| storyboard_id | VARCHAR(36) | 当前章节分镜 ID |
| metadata | JSON | 扩展元信息 |

### characters（角色表）
| 字段 | 类型 | 说明 |
|------|------|------|
| id | VARCHAR(36) | 主键 |
| novel_id | VARCHAR(36) | 外键 → novels |
| name | VARCHAR(50) | 角色名 |
| role | VARCHAR(20) | 角色定位（主角/配角等） |
| gender | VARCHAR(10) | 性别 |
| age | INT | 年龄 |
| appearance | JSON | 外貌描述 |
| personalities | JSON | 性格列表 |
| portrait_path | VARCHAR(255) | 肖像图路径，版本管理在代码层面做 |
| default_config_id | VARCHAR(36) | 默认配置 ID |

### scenes（场景表）
| 字段 | 类型 | 说明 |
|------|------|------|
| id | VARCHAR(36) | 主键 |
| novel_id | VARCHAR(36) | 外键 → novels |
| name | VARCHAR(100) | 场景名 |
| details | JSON | 氛围/天气/色调等描述 |
| tags | JSON | 标签列表 |
| reference_image_path | VARCHAR(255) | 参考图路径 |
| created_at / updated_at | TIMESTAMP | 时间戳 |

### storyboards（分镜表）
| 字段 | 类型 | 说明 |
|------|------|------|
| id | VARCHAR(36) | 主键 |
| novel_id | VARCHAR(36) | 外键 → novels |
| chapter_number | INT | 章节编号，支持多章节 |
| version | INT | 版本号 |
| total_pages | INT | 总页数 |
| panel_count | INT | 面板总数 |
| style_overrides | JSON | 风格覆盖设置 |

### panels（面板表）
| 字段 | 类型 | 说明 |
|------|------|------|
| id | VARCHAR(36) | 主键 |
| storyboard_id | VARCHAR(36) | 外键 → storyboards |
| page | INT | 第几页（1开始） |
| panel_index | INT | 页内第几格（0开始），每页固定6格 |
| content | JSON | 台词、角色、场景描述、editIntent 等 |
| visual_prompt | TEXT | 图像生成用的 prompt |
| preview_image_path | VARCHAR(255) | 缩略图路径，UI 列表展示用 |
| hd_image_path | VARCHAR(255) | 高清图路径，导出/大图查看用 |
| layout | JSON | 面板布局信息 |

面板编号计算公式：`(page - 1) * 6 + panel_index + 1`，用于匹配用户说的"第几个面板"。

### jobs（任务队列表）
| 字段 | 类型 | 说明 |
|------|------|------|
| id | VARCHAR(36) | 主键 |
| novel_id | VARCHAR(36) | 关联作品 |
| storyboard_id | VARCHAR(36) | 关联分镜 |
| type | VARCHAR(20) | 任务类型：AI分析 / 图片生成 / 导出 |
| status | VARCHAR(20) | pending / running / completed / failed |
| params | JSON | 任务参数（text、chapterNumber 等） |
| progress | INT | 进度百分比 0-100 |
| total_tasks / completed_tasks / failed_tasks | INT | 子任务统计 |
| result | JSON | 任务结果 |
| error_message | TEXT | 失败原因 |

### change_requests（修改请求表）
| 字段 | 类型 | 说明 |
|------|------|------|
| id | VARCHAR(36) | 主键 |
| novel_id | VARCHAR(36) | 外键 → novels |
| user_id | VARCHAR(36) | 外键 → users |
| storyboard_id | VARCHAR(36) | 外键 → storyboards |
| natural_language | TEXT | 用户原始输入 |
| dsl | JSON | Qwen 解析后的结构化 DSL |
| context | JSON | 面板定位上下文（panelId、chapterNumber 等） |
| status | VARCHAR(20) | queued / in_progress / pending / completed / failed |
| error_message | TEXT | 失败原因 |

### exports（导出表）
| 字段 | 类型 | 说明 |
|------|------|------|
| id | VARCHAR(36) | 主键 |
| novel_id | VARCHAR(36) | 外键 → novels |
| format | VARCHAR(20) | 导出格式：pdf / webtoon / zip |
| status | VARCHAR(20) | pending / completed / failed |
| file_path | VARCHAR(255) | 本地文件路径 |
| file_size | BIGINT | 文件大小（字节） |
| download_url | VARCHAR(500) | 下载链接 |
| url_expires_at | TIMESTAMP | 链接过期时间 |

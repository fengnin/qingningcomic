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

## 模块二：上传小说链路

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

## 6. 数据库连接池 + RAII 事务

**问题**
多线程环境下共享同一个数据库连接会有竞态问题；事务忘记回滚会导致数据不一致。

**方案**
DatabaseManager 实现线程安全连接池，每个线程获取独立连接。TransactionScope 用 RAII 管理事务，析构时自动回滚未提交的事务，避免忘记回滚。

**细节（追问）**
所有 SQL 用 QSqlQuery::bindValue 参数化查询，禁止字符串拼接，杜绝 SQL 注入。DatabaseWorker 将耗时 DB 操作派发到专用线程，避免主线程卡顿。

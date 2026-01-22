# OpenSpec 助手指令

使用OpenSpec进行规范驱动开发的AI编程助手指令。

## TL;DR 快速检查清单

- 搜索现有工作：`openspec spec list --long`、`openspec list`（仅在全文搜索时使用 `rg`）
- 决定范围：新功能 vs 修改现有功能
- 选择唯一的`change-id`：kebab-case格式，动词引导（`add-`、`update-`、`remove-`、`refactor-`）
- 创建脚手架：`proposal.md`、`tasks.md`、`design.md`（仅在需要时）以及按影响功能的增量规范
- 编写增量：使用`## ADDED|MODIFIED|REMOVED|RENAMED Requirements`；每个需求至少包含一个`#### Scenario:`
- 验证：`openspec validate [change-id] --strict`并修复问题
- 请求批准：在提案被批准前不要开始实施

## 三阶段工作流程

### 第一阶段：创建变更
在需要时创建提案：
- 添加功能或特性
- 进行破坏性变更（API、模式）
- 改变架构或模式
- 优化性能（改变行为）
- 更新安全模式

触发器（示例）：
- "帮我创建一个变更提案"
- "帮我规划一个变更"
- "帮我创建一个提案"
- "我想创建一个规范提案"
- "我想创建一个规范"

宽松匹配指导：
- 包含以下之一：`proposal`、`change`、`spec`
- 以及以下之一：`create`、`plan`、`make`、`start`、`help`

跳过提案的情况：
- Bug修复（恢复预期行为）
- 拼写错误、格式、注释
- 依赖更新（非破坏性）
- 配置变更
- 现有行为的测试

**工作流程**
1. 查看`openspec/project.md`、`openspec list`和`openspec list --specs`了解当前上下文。
2. 选择唯一的动词引导`change-id`，并在`openspec/changes/<id>/`下创建`proposal.md`、`tasks.md`、可选的`design.md`和规范增量的脚手架。
3. 使用`## ADDED|MODIFIED|REMOVED Requirements`起草规范增量，每个需求至少包含一个`#### Scenario:`。
4. 运行`openspec validate <id> --strict`并在共享提案前解决任何问题。

### 第二阶段：实施变更
将这些步骤作为TODOs逐一跟踪并完成。
1. **读取proposal.md** - 了解要构建什么
2. **读取design.md**（如果存在） - 审查技术决策
3. **读取tasks.md** - 获取实施检查清单
4. **按顺序实施任务** - 按顺序完成
5. **确认完成** - 在更新状态前确保`tasks.md`中的每个项目都已完成
6. **更新检查清单** - 所有工作完成后，将每个任务设置为`- [x]`，使列表反映实际情况
7. **批准门槛** - 在提案被审查和批准前不要开始实施

### 第三阶段：归档变更
部署后，创建单独的PR来：
- 将`changes/[name]/`移动到`changes/archive/YYYY-MM-DD-[name]/`
- 如果功能发生变化，更新`specs/`
- 对于仅工具变更，使用`openspec archive <change-id> --skip-specs --yes`（始终明确传递变更ID）
- 运行`openspec validate --strict`确认归档的变更通过检查

## 任何任务之前

**上下文检查清单：**
- [ ] 阅读`specs/[capability]/spec.md`中的相关规范
- [ ] 检查`changes/`中的待处理变更是否存在冲突
- [ ] 阅读`openspec/project.md`了解约定
- [ ] 运行`openspec list`查看活跃变更
- [ ] 运行`openspec list --specs`查看现有功能

**创建规范之前：**
- 始终检查功能是否已存在
- 优先修改现有规范而非创建重复项
- 使用`openspec show [spec]`查看当前状态
- 如果请求不明确，在创建脚手架前提出1-2个澄清问题

### 搜索指导
- 枚举规范：`openspec spec list --long`（或脚本使用`--json`）
- 枚举变更：`openspec list`（或`openspec change list --json` - 已弃用但可用）
- 显示详情：
  - 规范：`openspec show <spec-id> --type spec`（使用`--json`进行过滤）
  - 变更：`openspec show <change-id> --json --deltas-only`
- 全文搜索（使用ripgrep）：`rg -n "Requirement:|Scenario:" openspec/specs`

## 快速开始

### CLI 命令

```bash
# 基本命令
openspec list                  # 列出活跃变更
openspec list --specs          # 列出规范
openspec show [item]           # 显示变更或规范
openspec validate [item]       # 验证变更或规范
openspec archive <change-id> [--yes|-y]   # 部署后归档（非交互运行添加--yes）

# 项目管理
openspec init [path]           # 初始化OpenSpec
openspec update [path]         # 更新指令文件

# 交互模式
openspec show                  # 提示选择
openspec validate              # 批量验证模式

# 调试
openspec show [change] --json --deltas-only
openspec validate [change] --strict
```

### 命令标志

- `--json` - 机器可读输出
- `--type change|spec` - 区分项目类型
- `--strict` - 综合验证
- `--no-interactive` - 禁用提示
- `--skip-specs` - 不更新规范的归档
- `--yes`/`-y` - 跳过确认提示（非交互归档）

## 目录结构

```
openspec/
├── project.md              # 项目约定
├── specs/                  # 当前事实 - 已构建的内容
│   └── [capability]/       # 单一专注功能
│       ├── spec.md         # 需求和场景
│       └── design.md       # 技术模式
├── changes/                # 提案 - 应该改变什么
│   ├── [change-name]/
│   │   ├── proposal.md     # 为什么、什么、影响
│   │   ├── tasks.md        # 实施检查清单
│   │   ├── design.md       # 技术决策（可选；见标准）
│   │   └── specs/          # 增量变更
│   │       └── [capability]/
│   │           └── spec.md # ADDED/MODIFIED/REMOVED
│   └── archive/            # 已完成变更
```

## 创建变更提案

### 决策树

```
新请求？
├─ 恢复规范行为的bug修复？ → 直接修复
├─ 拼写/格式/注释？ → 直接修复
├─ 新功能/能力？ → 创建提案
├─ 破坏性变更？ → 创建提案
├─ 架构变更？ → 创建提案
└─ 不明确？ → 创建提案（更安全）
```

### 提案结构

1. **创建目录：** `changes/[change-id]/`（kebab-case格式，动词引导，唯一）

2. **编写proposal.md：**
```markdown
# Change: [变更的简要描述]

## Why
[问题/机会的1-2句话说明]

## What Changes
- [变更的要点列表]
- [用**BREAKING**标记破坏性变更]

## Impact
- 影响的规范：[功能列表]
- 影响的代码：[关键文件/系统]
```

3. **创建规范增量：** `specs/[capability]/spec.md`
```markdown
## ADDED Requirements
### Requirement: 新功能
系统 SHALL 提供...

#### Scenario: 成功案例
- **WHEN** 用户执行操作
- **THEN** 预期结果

## MODIFIED Requirements
### Requirement: 现有功能
[完整的修改后需求]

## REMOVED Requirements
### Requirement: 旧功能
**Reason**: [删除原因]
**Migration**: [处理方式]
```
如果影响多个功能，在`changes/[change-id]/specs/<capability>/spec.md`下创建多个增量文件——每个功能一个。

4. **创建tasks.md：**
```markdown
## 1. 实施
- [ ] 1.1 创建数据库模式
- [ ] 1.2 实现API端点
- [ ] 1.3 添加前端组件
- [ ] 1.4 编写测试
```

5. **需要时创建design.md：**
如果适用以下任何情况则创建`design.md`；否则省略：
- 跨领域变更（多个服务/模块）或新架构模式
- 新的外部依赖或重要数据模型变更
- 安全、性能或迁移复杂性
- 编码前从技术决策中受益的模糊性

最小`design.md`骨架：
```markdown
## Context
[背景、约束、利益相关者]

## Goals / Non-Goals
- Goals: [...]
- Non-Goals: [...]

## Decisions
- Decision: [什么和为什么]
- Alternatives considered: [选项 + 理由]

## Risks / Trade-offs
- [Risk] → 缓解措施

## Migration Plan
[步骤、回滚]

## Open Questions
- [...]
```

## 规范文件格式

### 关键：场景格式

**正确**（使用####标题）：
```markdown
#### Scenario: 用户登录成功
- **WHEN** 提供有效凭据
- **THEN** 返回JWT令牌
```

**错误**（不要使用要点或粗体）：
```markdown
- **Scenario: 用户登录**  ❌
**Scenario**: 用户登录     ❌
### Scenario: 用户登录      ❌
```

每个需求必须至少有一个场景。

### 需求措辞
- 对规范性需求使用 SHALL/MUST（除非故意非规范性，否则避免 should/may）

### 增量操作

- `## ADDED Requirements` - 新功能
- `## MODIFIED Requirements` - 改变的行为
- `## REMOVED Requirements` - 已弃用功能
- `## RENAMED Requirements` - 名称变更

标题使用`trim(header)`匹配 - 忽略空白。

#### 何时使用ADDED vs MODIFIED
- ADDED：引入可以作为需求独立存在的新功能或子功能。当变更是正交的（例如，添加"斜杠命令配置"）而不是改变现有需求的语义时，优先使用ADDED。
- MODIFIED：改变现有需求的行为、范围或验收标准。始终粘贴完整的更新后需求内容（标题 + 所有场景）。归档器将用您在此处提供的内容替换整个需求；部分增量将丢失之前的详细信息。

常见陷阱：使用MODIFIED添加新关注点而不包含之前的文本。这会在归档时丢失详细信息。如果您没有明确更改现有需求，而是在ADDED下添加新需求。

正确编写MODIFIED需求：
1) 在`openspec/specs/<capability>/spec.md`中定位现有需求。
2) 复制整个需求块（从`### Requirement: ...`到其场景）。
3) 将其粘贴到`## MODIFIED Requirements`下并编辑以反映新行为。
4) 确保标题文本完全匹配（空白不敏感）并保持至少一个`#### Scenario:`。

RENAMED示例：
```markdown
## RENAMED Requirements
- FROM: `### Requirement: Login`
- TO: `### Requirement: User Authentication`
```

## 故障排除

### 常见错误

**"Change must have at least one delta"**
- 检查`changes/[name]/specs/`存在且包含.md文件
- 验证文件具有操作前缀（## ADDED Requirements）

**"Requirement must have at least one scenario"**
- 检查场景使用`#### Scenario:`格式（4个井号）
- 不要对场景标题使用要点或粗体

**静默场景解析失败**
- 需要精确格式：`#### Scenario: Name`
- 调试：`openspec show [change] --json --deltas-only`

### 验证技巧

```bash
# 始终使用严格模式进行综合检查
openspec validate [change] --strict

# 调试增量解析
openspec show [change] --json | jq '.deltas'

# 检查特定需求
openspec show [spec] --json -r 1
```

## 快速路径脚本

```bash
# 1) 探索当前状态
openspec spec list --long
openspec list
# 可选全文搜索：
# rg -n "Requirement:|Scenario:" openspec/specs
# rg -n "^#|Requirement:" openspec/changes

# 2) 选择变更id并创建脚手架
CHANGE=add-two-factor-auth
mkdir -p openspec/changes/$CHANGE/{specs/auth}  # 展开为: openspec/changes/$CHANGE/specs/auth
printf "## Why\n...\n\n## What Changes\n- ...\n\n## Impact\n- ...\n" > openspec/changes/$CHANGE/proposal.md
printf "## 1. Implementation\n- [ ] 1.1 ...\n" > openspec/changes/$CHANGE/tasks.md

# 3) 添加增量（示例）
cat > openspec/changes/$CHANGE/specs/auth/spec.md << 'EOF'
## ADDED Requirements
### Requirement: 双重认证
用户在登录期间 MUST 提供第二个因素。

#### Scenario: 需要OTP
- **WHEN** 提供有效凭据
- **THEN** 需要OTP挑战
EOF

# 4) 验证
openspec validate $CHANGE --strict
```

## 多功能示例

```
openspec/changes/add-2fa-notify/
├── proposal.md
├── tasks.md
└── specs/
    ├── auth/
    │   └── spec.md   # ADDED: 双重认证
    └── notifications/
        └── spec.md   # ADDED: OTP邮件通知
```

auth/spec.md
```markdown
## ADDED Requirements
### Requirement: 双重认证
...
```

notifications/spec.md
```markdown
## ADDED Requirements
### Requirement: OTP邮件通知
...
```

## 最佳实践

### 简单优先
- 默认<100行新代码
- 在证明不足之前使用单文件实现
- 避免没有明确理由的框架
- 选择无聊的、经过验证的模式

### 复杂性触发器
仅在以下情况添加复杂性：
- 性能数据显示当前解决方案太慢
- 具体规模要求（>1000用户，>100MB数据）
- 多个经过验证的用例需要抽象

### 清晰引用
- 对代码位置使用`file.ts:42`格式
- 将规范引用为`specs/auth/spec.md`
- 链接相关变更和PR

### 功能命名
- 使用动词-名词：`user-auth`、`payment-capture`
- 每个功能单一用途
- 10分钟可理解性规则
- 如果描述需要"AND"则拆分

### 变更ID命名
- 使用kebab-case格式，简短且描述性：`add-two-factor-auth`
- 优先动词引导前缀：`add-`、`update-`、`remove-`、`refactor-`
- 确保唯一性；如果已被占用，附加`-2`、`-3`等

## 工具选择指南

| 任务 | 工具 | 原因 |
|------|------|-----|
| 按模式查找文件 | Glob | 快速模式匹配 |
| 搜索代码内容 | Grep | 优化的正则搜索 |
| 读取特定文件 | Read | 直接文件访问 |
| 探索未知范围 | Task | 多步骤调查 |

## 错误恢复

### 变更冲突
1. 运行`openspec list`查看活跃变更
2. 检查重叠规范
3. 与变更所有者协调
4. 考虑组合提案

### 验证失败
1. 使用`--strict`标志运行
2. 检查JSON输出获取详细信息
3. 验证规范文件格式
4. 确保场景格式正确

### 缺少上下文
1. 首先阅读project.md
2. 检查相关规范
3. 查看最近的归档
4. 请求澄清

## 快速参考

### 阶段指示器
- `changes/` - 已提议，尚未构建
- `specs/` - 已构建和部署
- `archive/` - 已完成变更

### 文件用途
- `proposal.md` - 为什么和什么
- `tasks.md` - 实施步骤
- `design.md` - 技术决策
- `spec.md` - 需求和行为

### CLI基本命令
```bash
openspec list              # 进行中有什么？
openspec show [item]       # 查看详情
openspec validate --strict # 是否正确？
openspec archive <change-id> [--yes|-y]  # 标记完成（自动化添加--yes）
```

记住：规范是事实。变更是提案。保持它们同步。

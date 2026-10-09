# TreeJev as the Sequencer: Flows, Lanes and One Turn Record

> Status: Implemented
> Implemented 2026-10-09, uncommitted (TreeJev on 2a388b8, research-suite on 6bfd402, SequenceTree on ecd9950).
> Built from 31bccc7 (SequenceTree), TreeJev 2a388b8 (its initial commit), research-suite ecdd8e0 plus uncommitted work from another session; drift: none in the files the plan touches.
> Written 2026-10-09 at 31bccc7 (SequenceTree), research-suite plugin 0.13.0, TreeJev working tree (no commits).
> Source: the owner's design decisions from the 2026-10-09 session, not a reviewed report. No `Reviewed/` report covers this. The proposal builds on two implemented proposals, `treejev_route_context_and_tree_composition.md` and `treejev_fail_closed_turn_decisions.md`, and every claim about current code below was read at the versions above.

## Summary

TreeJev decides what happens at one moment: a tree walks and returns an action. Deciding **when** each tree runs, and what its answer **does**, currently happens outside TreeJev, in research-suite's shell scripts, `refactor.toml` keys, temp marker files and a hand-written protocol file. None of that is visible in TreeJev, and the parts disagree with each other. The missing signifier was one such disagreement.

This proposal makes TreeJev the sequencer. It is built on the owner's decisions:

1. **TreeJev owns the sequence.** research-suite keeps its checks (design rules, readability, tests, collectors) as commands the flow names. Its scripts stop deciding when they run.
2. **Agent-called trees are allowed, in three forms:** a flow step, a `delegate` effect, and an encapsulator inside a tree marked `runBy: agent`. The flow tells the agent at the right moment and checks at Stop that the call happened.
3. **Sequences are drawn as lanes**, one per hook event, rather than as one large tree of encapsulators.
4. **Input steps** collect what a tree reads by running project commands.
5. **The agent's instructions are generated from the flow**, so they can't drift from it.
6. **Hooks are registered by whoever owns them.** Hooks that make TreeJev work are installed by `treejev init`. A plugin such as research-suite keeps registering only hooks that are specific to it and that no tree decision affects. Anything a tree decision controls runs as a flow step (§9).
7. **A broken TreeJev stops the task and is repaired.** Any TreeJev failure is reported to the owner at once and stops the work in progress. The agent then finds and fixes the cause on its own and resumes the original request. Only if three repair attempts fail does everything stop and wait for the owner (§4).
8. **A delegated tree that never ran halts the agent.** It is reported to the owner, and the agent stops until the owner has looked at it (§5).
9. **Traces live with the TreeJev application**, under `~/.treejev/`, never in the project being steered (§2, §8).

The design has six parts:

- a **flow document** (`treejev/flows/<id>.json`) with five lanes;
- one **turn record** per session that replaces the four temp files;
- a closed set of **effects** an action can have;
- a **lane runner** behind the existing `treejev hook claude` entry point;
- a **lanes view** in the TreeJev editor, with a trace of the last turn;
- **generated agent instructions**.

SequenceTree is ported to the new design, and the port is verified by comparing its output with today's hook output for the same prompts. No `Source/` file changes. The work is about 2,000 lines of TypeScript (UI and tests included), plus a net deletion of about 300 lines of shell in research-suite. **P2.**

## The Problem

### Mechanism

The sequencing in effect on every SequenceTree turn is spread across these places:

1. **Which event runs what:**
   - research-suite `hooks/hooks.json` registers 12 commands across SessionStart, UserPromptSubmit, PreToolUse (two matchers), PostToolUse and Stop.
   - `treejev init` separately writes Stop and `mcp__treejev__.*` PostToolUse entries into `.claude/settings.json` (`setClaudeTreeJevHook`, `TreeJev/src/cli/init-project.ts:106`).
   - The two lists don't know about each other.
2. **Which tree runs on the prompt:** `turn_tree = "sequence_tree_turn"` in `.claude/refactor.toml` is read by `lib/gates.sh:42`, and `bin/claude-md-hook.sh:26-33` pipes the payload into `treejev hook claude "$turn_tree"`.
3. **What an action means is written into shell:**
   - `claude-md-hook.sh:39-55` maps the labels `skip|light|research|code|deep` to modes. An unknown label becomes `code`. A confidence below `turn_confidence_floor` turns `skip/light/research` into `code`, and `-deep` in the prompt forces `deep`.
   - `claude-md-hook.sh:89-158` then writes a different paragraph of instructions for each mode and decides whether to run `design-rules.sh` and `session-rules.sh`.
4. **Which trees run at Stop, and what their answers mean:** `systematic-check.sh:133-195` runs `change_trees` only when the marker `claude-systematic-deep-<session>` exists. It treats `outcome == "flag"` as a block and passes the actions' `checks` on to `route-checks.sh --hook stop`.
5. **How results reach later steps:** through four files.
   - `$TMPDIR/treejev/<session>.json`: the decision file (`turnDecisionPath`, `hook-cli.ts:42`).
   - `<session>.runs.json`: the runs log (`turnRunsPath`, `:51`).
   - `claude-turn-mode-<session>` and `claude-route-checks-<session>`: written by `claude-md-hook.sh:58-59`, read by `stop_gates_wanted` (`lib/gates.sh:94`) and `route-checks.sh:10`.
   - `claude-systematic-deep-<session>`: written by `claude-md-hook.sh:62-68`.
6. **What the agent is told:**
   - `TreeJevAgentProtocol.md` is written once by `writeAgentProtocol` (`init-project.ts:137`) and then edited by hand.
   - The MCP tool descriptions (`server.ts:75`, `:270`) hard-code "ALWAYS include the compact signifier".

### Evidence

- **The signifier:** the protocol required it, but the hook path never asked for it, so it was left out (2026-10-08 session). Moving the report into the Stop hook fixed the symptom but not the cause.
- **Contradicting directives:** `treejev_turn_directive_gate_wording.md` (Unimplemented) records that the tree's `skip`/`light` directives say "no gates" while `stop_gates_wanted` still runs them. The tree's words and the shell's behaviour are written in different files.
- **Change trees:** adding the change trees on 2026-10-08 needed edits to five files in two repositories (`systematic-check.sh`, `hooks.json`, `refactor.toml`, `run-cli.ts`, `batch_edit.py`). None of those edits is visible from the TreeJev editor.

### Why It Matters

Each new rule about when a tree runs adds more shell and another marker file. The owner can't see the sequence, can't edit it in one place, and can't confirm what ran last turn. And because the agent's instructions are written separately from the behaviour they describe, they drift.

## Current Design

### API Surface

- **TreeJev:**
  - `runHookCli(args)` (`hook-cli.ts:252`) picks a handler by `hook_event_name`: `runClaudePromptHook` (`:123`), `runClaudeToolHook` (`:203`) or `runClaudeStopHook` (`:215`).
  - `runTreeToCompletion(graph, treeId, state, options, maxSteps)` (`runner.ts:58`) returns a `TraversalExecutionResult` whose `terminalAction` carries `label`, `directive`, `outcome`, `context` and `checks`.
  - `formatEvaluationReport` (`server.ts:36`) is the only producer of the `[TreeJev: … | Final Action: …]` text.
  - `loadTree` (`tree-repository.ts:247`) resolves encapsulator `treeRef`s.
  - `ActionDecisionConfig` (`types/tree.ts:27`) has `description`, `directive`, `outcome`, `context` and `checks`. `EncapsulatorDecisionConfig` (`:43`) has `treeRef`, `subgraph`, `exitNodeId` and `outputBranches`.
- **research-suite:**
  - `claude-md-hook.sh`, `systematic-check.sh` (`--hook stop`, `--state`) and `route-checks.sh` (`--hook pre-tool|post-tool|stop`).
  - Gate scripts that call `stop_gates_wanted`: `design-rules.sh:34`, `readability.sh:32` and `source-lines.sh`.
  - `lib/gates.sh` reads `[gates]` keys from `refactor.toml`.
- **Project config:**
  - `treejev/config.json`: `activeTree`, `evaluationMode`, `hooksEnabled`, `reportTreesOnStop`.
  - `.claude/refactor.toml [gates]`: `turn_tree`, `turn_confidence_floor`, `fallback_route_checks`, `change_trees`, `[gates.route_checks]`.

### Structure

TreeJev is a Next.js and Electron app. `app/page.tsx` hosts `PlaybackBar`, `ProjectSidebar`, `PatchTabBar` and `TreeCanvas` (React Flow, `@xyflow/react`). Workspace tabs (`types/tabs.ts`) each hold one `TreeGraph`. The CLI runs from `src/` through `tsx` (`bin/treejev`), and `tsconfig.json` sets `noEmit`. Trees live in `<project>/treejev/trees/`.

### Data Flow (one deep SequenceTree turn today)

1. **Session start:** the plugin runs `session-rules.sh --hook`, which adds the rules to the context.
2. **Prompt:**
   - The plugin runs `claude-md-hook.sh`.
   - `turn-scope.sh --snapshot` runs.
   - `treejev hook claude sequence_tree_turn` walks the tree and writes the decision file and the runs log.
   - The script reads the decision file back, applies the floor and the `-deep` override, and writes three markers.
   - It runs `design-rules.sh` and `session-rules.sh`, writes the mode paragraph, and prints `additionalContext`.
3. **Bash call:** `route-checks.sh --hook pre-tool` reads the route-checks marker and runs `edit-tools-check.py`.
4. **Edit or Write:** `refactor-gate.sh` runs before the tool. `design-rules.sh`, `readability.sh` and `route-checks.sh --hook post-tool` run after it.
5. **Call to a `mcp__treejev__` tool:** `treejev hook claude` (from `settings.json`) appends to the runs log.
6. **Stop:**
   - `design-rules.sh`, `readability.sh` and `source-lines.sh` run, each gated by `stop_gates_wanted`.
   - `systematic-check.sh` checks the deep marker, runs `--state`, runs each change tree through `treejev run --json`, and calls `route-checks.sh` for the checks the trees owe.
   - `route-checks.sh --hook stop` runs.
   - `treejev hook claude` (from `settings.json`) prints the tree-runs report.

## The Design

### 1. The flow document

`treejev/flows/<id>.json`. The project's `treejev/config.json` names the active flow (`activeFlow`).

```jsonc
{
  "id": "sequence_tree",
  "name": "SequenceTree turn flow",
  "reportTrees": true,                    // replaces config.reportTreesOnStop
  "checks": {                             // replaces [gates.route_checks]; commands never live in tree JSON
    "edit-tools": { "event": "PreToolUse", "matcher": "Bash", "command": "$TREEJEV_PLUGIN_RESEARCH_SUITE/bin/edit-tools-check.py Source Tests" },
    "repeat":     { "event": "PostToolUse", "matcher": "Edit|Write", "command": "$TREEJEV_PLUGIN_RESEARCH_SUITE/bin/repeat-check.sh --hook post-tool" },
    "tests":      { "event": "Stop", "command": "cmake --build cmake-build-debug --target SequenceTree_Tests SequenceTree_GraphTests && cd cmake-build-debug && ctest --output-on-failure" }
  },
  "lanes": {
    "SessionStart":     { "steps": [ … ] },
    "UserPromptSubmit": { "steps": [ … ] },
    "PreToolUse":       { "steps": [ … ] },
    "PostToolUse":      { "steps": [ … ] },
    "Stop":             { "steps": [ … ] }
  }
}
```

**Steps.** Each step has `id` (unique in the flow), `label`, `kind`, an optional `matcher` (a tool-name regex, valid only in the two tool lanes), an optional `when`, a `timeoutMs`, and fields that depend on its kind. `kind` is an enum of five values:

| `kind` | What it does | Fields |
|---|---|---|
| `tree` | Walks a tree on an input and applies the effects of the action it reaches | `treeId`, `input`, `overrides`, `confidenceFloor` |
| `agentTree` | Hands a tree to the agent: adds the instruction now, and checks at Stop that the call happened | `treeId`, `instruction`, `stateHint` |
| `command` | Runs a project command and applies its output (see *Command output* below) | `command`, `required` |
| `owedChecks` | Runs the registry checks for this event that earlier effects marked as owed | — |
| `treeReport` | Shows the user the turn's tree runs (Stop lane only) | — |

`owedChecks` and `treeReport` are explicit steps rather than built-in behaviour, so the lanes view shows exactly where they happen.

**Inputs.** `input` is one of two shapes:

- `{ "from": "payload" }`: the hook payload's `prompt`, `cwd`, `tool_name` and `tool_input`, plus `sessionId`. This is what `runClaudePromptHook` builds today (`hook-cli.ts:130-134`).
- `{ "from": "command", "command": "…" }`: the command's stdout, parsed as a JSON object. For example, the change trees read `"$TREEJEV_PLUGIN_RESEARCH_SUITE/bin/systematic-check.sh" --state`.

**Conditions** (`when`) form a closed grammar: `{ "all": [Cond…] }` or `{ "any": [Cond…] }`. Each `Cond` is `{ "path", "equals" | "in" | "matches" | "exists" }`. A path reads either the turn record (`turn.steps.<stepId>.action`, `turn.values.<key>`) or the payload (`payload.prompt`, `payload.tool_name`, `payload.tool_input.file_path`). There are no expressions, arithmetic or loops.

**Tree-step resolution** moves the shell's label handling into the flow:

```jsonc
{ "id": "turn", "kind": "tree", "treeId": "sequence_tree_turn", "input": { "from": "payload" },
  "overrides": [ { "when": { "all": [ { "path": "payload.prompt", "matches": "-deep([^[:alnum:]_-]|$)" } ] }, "action": "deep" } ],
  "confidenceFloor": { "floor": 0.5, "appliesTo": ["skip", "light", "research"], "action": "code" } }
```

- `overrides` and `confidenceFloor` each name an action node in the tree by its label. The validator checks that the label exists.
- When the floor or an override changes the action, the runner adds the same note the shell adds today ("Turn mode: code - the tree chose research at 0.35, below the confidence floor 0.5.").
- A low confidence is a judgement, not a failure, so the floor stays.
- A walk that errors or ends on no action counts as TreeJev being broken, and enters repair (§4). There is no `fallbackAction`. This replaces the "unknown label becomes `code`" rule from `treejev_fail_closed_turn_decisions.md`: that rule kept work going on a guess, and the owner now wants the failure fixed first.

**Commands get no text interpolation.** Each command runs under `bash -c` with:

- the hook payload on stdin;
- `TREEJEV_SESSION`, `TREEJEV_TURN_FILE`, `TREEJEV_EVENT` and `TREEJEV_STEP` in the environment, plus `TREEJEV_PLUGIN_<NAME>` for each plugin the flow names in `plugins` (§9), plus the inherited environment.

Prompt text never enters a command string. A script that needs a turn value reads `TREEJEV_TURN_FILE` with `jq`.

### 2. The turn record

One file replaces the decision file, the runs log and the three research-suite markers. It lives with the TreeJev application, not in the project or in `$TMPDIR`:

```
~/.treejev/sessions/<projectKey>/<sessionId>/turn.json        the current turn
~/.treejev/sessions/<projectKey>/<sessionId>/history/<n>.json the last 20 turns
```

- `~/.treejev` is the global directory TreeJev already uses for its config (`config-storage.ts:31`), and `TREEJEV_GLOBAL_DIR` still overrides it.
- `<projectKey>` is the project folder's name plus the first 8 hex digits of a SHA-1 of its absolute path, for example `SequenceTree-3f9a1c2e`. The TreeJev app can then list sessions by project.
- Commands get the path as `TREEJEV_TURN_FILE`, so no script needs to know the layout.

The file looks like this:

```jsonc
{
  "sessionId": "…", "turn": 14, "startedAt": "…",
  "steps": { "turn": { "kind": "tree", "treeId": "sequence_tree_turn", "action": "deep", "rawAction": "research",
                       "resolvedBy": "override", "confidence": 0.35, "outcome": "pass", "status": "completed", "ms": 212 } },
  "values": { },                                   // written by `set` effects
  "checksOwed": ["edit-tools", "tests"],
  "delegations": [ { "treeId": "change_surface", "fromStep": "turn", "satisfied": false } ],
  "treeRuns": [ { "treeId": "sequence_tree_turn", "outcome": "Final Action: deep", "via": "hook" } ],
  "events": [ { "event": "PreToolUse", "stepId": "edit-tools", "result": "allowed", "ms": 61 } ]
}
```

- **A new turn starts the record afresh.** The first UserPromptSubmit step does this, as `logTreeRuns(…, true)` does today. The previous record moves to `history/<turn>.json`, and the last 20 are kept for the trace. Session folders untouched for 30 days are deleted the next time any session starts.
- **Writes are atomic.** Each write goes to a temp file and is then renamed. Read-modify-write sections hold a per-session lock file (created with `O_EXCL`, retried for up to 2 s), because parallel tool calls fire PostToolUse hooks at the same time.
- **The old files keep being written** (the decision file, plus mode and route-checks markers) until step 8 has moved every reader. That way each step can be released without breaking research-suite.

### 3. Action effects

`ActionDecisionConfig` keeps `directive`, `context`, `checks` and `outcome`. It gains `effects`, and every member of `effects` is optional:

```ts
export interface ActionEffects {
  block?: string;                         // Stop: decision block; PreToolUse: deny; UserPromptSubmit/PostToolUse: decision block. {{var}} interpolation
  report?: string;                        // shown to the user as a systemMessage line
  set?: Record<string, string | number | boolean>;  // written to turn.values
  delegate?: { treeId: string; instruction: string }[];  // agent-called trees, see §5
}
```

How the existing fields behave in a flow:

- **`directive` and `context`** go into the agent's context through the existing report text (`formatEvaluationReport`), on events where context is allowed.
- **`checks`** adds names to `turn.checksOwed`.
- **`outcome`** keeps its per-unit meaning only. A flow never reads `flag` as "block". The change trees' flagged actions get an explicit `block` instead.

This retires the shell rule `outcome == flag → block` (`systematic-check.sh:163`). It makes no behaviour change, because every action that flags today gets a `block` with its old directive text.

### 4. The lane runner

`treejev hook claude` keeps its name and its dispatch on `hook_event_name`, so `settings.json` and `hooks.json` entries never change again. For the event's lane, the runner:

1. loads the active flow and the turn record;
2. drops steps whose `matcher` or `when` doesn't hold;
3. runs the remaining steps in order, each with its timeout, and records each result in the turn record;
4. merges the effects into **one** hook output for the event:

| Event | Context | Block | Report |
|---|---|---|---|
| SessionStart | `additionalContext`, joined in step order | — (validator rejects) | `systemMessage` |
| UserPromptSubmit | `additionalContext` | `decision: block` + reason | `systemMessage` |
| PreToolUse | `additionalContext` | `permissionDecision: deny` + reasons | `systemMessage` |
| PostToolUse | `additionalContext` | `decision: block` + reason | `systemMessage` |
| Stop | — | `decision: block` + reasons. When `stop_hook_active` is true, blocks become reports, so a Stop can block at most once | `systemMessage` |

**Command output.**

- If stdout is a Claude Code hook object (`hookSpecificOutput`, `decision`, `systemMessage`), the runner merges it as the matching context, block or report. That is why research-suite's gate scripts can run under the flow unchanged.
- If stdout is a TreeJev effects object (`{ "context", "block", "report", "set" }`), the runner applies it.
- Otherwise, plain stdout becomes context in the prompt and PostToolUse lanes and is dropped elsewhere.
- A non-zero exit is reported and doesn't block, unless the step sets `required: true`. In that case it blocks with the last 40 lines, as `route-checks.sh:70-76` does for Stop checks.

**`owedChecks`** ports `route-checks.sh` to TypeScript. It runs the registry entries whose `event` (and `matcher`) fits this hook and whose names are in `turn.checksOwed`:

- **PreToolUse:** a deny output denies the call.
- **PostToolUse:** the output becomes context. `ROUTE_CHECK_FILE` is set to the edited path when it falls under the project's sources.
- **Stop:** a non-zero exit blocks.

**When TreeJev is broken, the agent stops its task, repairs TreeJev, then resumes.** The owner's rule: any TreeJev failure is reported right away, and the work in progress stops. The agent then switches on its own to finding and fixing the cause, and goes back to the original request once TreeJev works again. Nothing falls back silently, and nothing waits for a special prompt.

- **What counts as TreeJev being broken:**
  - the flow fails to load or validate;
  - a tree fails to load or its walk errors, including a live Jev API that can't be reached;
  - a walk ends on no action;
  - the turn record can't be read or written;
  - the runner itself throws;
  - the `treejev` command is missing or crashes.
- **What doesn't count:** a project command that fails, such as a research-suite gate or a check. That is the project's own result. It is reported, and it blocks only if `required`, as in the table above.
- **Entering repair.** When a failure happens, the runner writes `~/.treejev/repair/<sessionId>.json` with:
  - the reason (what failed, in which step of which lane);
  - the event;
  - the original prompt of the turn;
  - the active flow's step ids at the moment of failure;
  - an attempt count, starting at 0.

  `halted: { reason, event, stepId }` also goes in the turn record, so the trace shows where it happened. The owner is told at once through `systemMessage`, which Claude Code shows directly, so the notice doesn't depend on the agent passing it on: "TreeJev is broken: <reason>. Your request is paused while Claude repairs it."
- **How the task stops, by event.** The hook doesn't end the session (`continue: false` would leave nothing able to repair TreeJev). Instead it ends the work in progress and gives the agent a repair directive:
  - **UserPromptSubmit:** the prompt goes through, but `additionalContext` says: "TreeJev is broken: <reason>. Do not start the request yet. Find the cause and fix it (TreeJev, this project's `treejev/` folder, or a plugin the flow names). Do not remove or disable steps, trees or hooks to make it pass. Say what you changed. When you stop, the Stop hook checks TreeJev and hands the request back."
  - **PreToolUse:** the call that was about to run is denied with the same directive as its reason. So the task stops at exactly that point.
  - **PostToolUse and Stop:** `decision: block` with the same directive.
- **Repair mode.** While the repair file exists, `treejev-hook` skips the flow on every event except Stop, so the agent's repair edits aren't stopped by the broken flow. research-suite's own hooks (§9) still run.
- **Resuming.** At Stop in repair mode, `treejev-hook` runs `treejev doctor --health`. That loads and validates the flow, resolves its plugins, and runs the UserPromptSubmit lane on the saved original prompt.
  - **Healthy:** the repair file is deleted. The owner gets the `systemMessage` "TreeJev repaired: <reason was>. Resuming your request." The hook blocks with "TreeJev is fixed. Now carry out the original request: <prompt>", followed by the prompt lane's normal output (turn decision, mode text, owed checks). The resumed work therefore runs under the working flow, exactly as if the prompt had just arrived.
  - **Healthy, but steps are missing:** if the flow's step ids are fewer than those recorded at the failure, the fix removed part of the sequence. That counts as still broken, with the reason "the repair removed steps <ids>". A fix that hides the failure never counts as a fix.
  - **Still broken:** the hook blocks with "Still broken: <reason>. Keep repairing." and the attempt count goes up by one.
  - **After 3 failed attempts:** the hook gives up and returns `{"continue": false, "stopReason": "TreeJev is still broken after 3 repair attempts: <reason>. Stopped; this needs you."}`. This is the one case where everything stops and waits for the owner.
  - Repair mode ignores `stop_hook_active`, because the attempt count bounds the loop instead.
- **Catching a crash or a missing binary.** TreeJev can't report its own crash, so every TreeJev hook entry is registered as `"$HOME/.treejev/bin/treejev-hook" || printf '{"continue":false,"stopReason":"TreeJev hook script is missing (exit %s). Stopped; this needs you."}' "$?"`.
  - `treejev-hook` is a POSIX `sh` script that `treejev init` installs into `~/.treejev/bin/`. It doesn't need Node.
  - It runs `treejev hook claude`. A non-zero exit or non-JSON output enters repair exactly as above, with the JSON written by `printf` in the shell, because Node may be what is broken. A `doctor --health` that crashes counts as still broken.
  - The `printf` after `||` covers only the wrapper itself being missing. Claude Code would otherwise treat that as a non-blocking error and ignore it. With no wrapper, nothing could run the repair loop, so this case stops and waits for the owner.
- **No active flow:** the runner builds a **default flow** from `treejev/config.json`, with `activeTree` on the prompt and `treeReport` at Stop when `reportTreesOnStop` is set. Projects that never adopt flows get the same hook output as today for every non-failing run. Their failures now halt too.

### 5. Agent-called trees

The owner said this is "kind of the purpose of encapsulators". An encapsulator hands control to another tree, and an agent-called tree hands a tree to the agent. Both use the same `treeRef` and are drawn the same way. There are three ways to declare one:

- **A flow step** `{ "kind": "agentTree", "treeId": "pr_code_review", "instruction": "before you commit" }` in any lane.
- **An action effect** `delegate: [{ treeId, instruction }]`: a tree's answer can decide that the agent owes another tree.
- **An encapsulator inside a tree** with `runBy: "agent"`. `runBy` is an enum, `walker` or `agent`, and defaults to `walker`.
  - The walker doesn't step into it. The walk completes on the encapsulator, and the runner treats it as a terminal action whose only effect is `delegate` for its `treeRef`.
  - Because the label has to be an action label, the result's label is the encapsulator node's own label.

In all three cases, the runner:

1. adds a delegation to the turn record;
2. adds one line to the context: `Run tree <id> (mcp__treejev__treejev_run_<id>) <instruction>. Pass the situation in "state".`;
3. lets PostToolUse on `mcp__treejev__.*`, which the runner logs whether or not the flow lists a step for it, mark the delegation satisfied when the tool result carries that tree's signifier (`parseTreeRuns`, `hook-cli.ts:58`);
4. checks at Stop, before any other Stop step. An unsatisfied delegation **halts**, the same way a broken TreeJev does (§4): `{"continue": false, "stopReason": "Tree <id> was handed to the agent (<instruction>) and was not run. Work is stopped until the flow is fixed."}`.
   - It doesn't block and let the agent try again, because the owner wants to see the failure, not have it fixed quietly.
   - The halt goes in the turn record, and the trace marks the delegation as missed.
   - The next prompt starts a normal turn, so the owner decides what happens next.

The tree the agent runs then applies its own action's effects in the PostToolUse lane. Its `checks` are added to `checksOwed`, and its `block` blocks the tool result. So a delegated tree can steer the rest of the turn just like a hook-run tree.

### 6. Generated agent instructions

`renderFlowInstructions(flow)` produces the agent's protocol from the flow:

- which trees run on which event and what their results do;
- which trees the agent may be asked to run, and how;
- what a block looks like.

It is used in three places:

- **SessionStart:** the default flow, and any flow with `"injectInstructions": true`, adds it to the context. Claude Code then gets it from the running flow, not from a file.
- **`TreeJevAgentProtocol.md`:** regenerated by `treejev flow sync` and every time the editor saves a flow, with a "generated from flows/<id>.json, do not edit" header. Agents without hooks (Cursor, Antigravity) read this file, and `.cursor/rules/treejev.mdc` names the trees from the flow's UserPromptSubmit and Stop lanes.
- **The MCP tool descriptions** (`registerTreeTool`, `server.ts:54`): a tree that a flow lists as agent-called gets its `instruction` added to its tool description. The hard-coded "ALWAYS include the compact signifier" sentence goes away, because the Stop `treeReport` step shows the user the runs instead.

### 7. The lanes view

This is a new workspace tab kind. `WorkspaceTab` gains `kind: 'tree' | 'flow'`, and a flow tab holds a `FlowDocument` instead of a `TreeGraph`.

- **Layout:** five horizontal lanes in event order. Each step is a card, ordered left to right.
- **Card contents:**
  - a kind icon and the label;
  - a matcher chip (`Bash`, `Edit|Write`);
  - a condition chip (`when turn.action ∈ code, deep`);
  - for tree cards, a strip of the tree's action labels, with flagging and blocking actions marked.
  - Agent-called cards use the encapsulator card's look (`EncapsulatorNodeView`) with a dashed "run by agent" border.
- **Data edges:** a dashed edge runs from a step to every later step whose `when` reads it. For example, `turn` in UserPromptSubmit links to `change_surface` in Stop. This shows the order across events that a tree can't express.
- **Editing:**
  - drag to reorder within a lane;
  - an add-step menu per lane;
  - an inspector panel for the selected step, with `FlowStepEditor`, `ConditionEditor` and `InputEditor`;
  - a checks-registry panel.
  - Double-clicking a tree card opens that tree in a tab (`addOrSwitchTab`).
  - The action inspector (`ActionConfigEditor.tsx`) gains `EffectsEditor` for `block` / `report` / `set` / `delegate`.
  - The encapsulator inspector gains the `runBy` toggle.
- **Validation:** runs on every change and shows problems inline on the cards. Saving an invalid flow is refused.
- **External hooks:** hooks registered by plugins or in `settings.json` that aren't TreeJev's own (§9) appear in their lanes as grey, read-only cards marked with where they come from (`research-suite`, `settings.json`). The view therefore shows everything that runs on each event, not only what the flow runs. TreeJev reads them from the project's `.claude/settings.json` and from the `hooks/hooks.json` of each enabled plugin. It finds each plugin's install path in `~/.claude/plugins/installed_plugins.json`, choosing the entry whose `projectPath` matches the project, else the user-scope one.

### 8. The last-turn trace

The lanes view has a **Last turn** toggle. It reads `~/.treejev/sessions/<projectKey>/` through `app/api/jev/traces/route.ts`. Because traces live with the application, the TreeJev app can list every project and session it has steered, and the steered project's folder never holds any of it.

- Steps that ran show their action badge and duration.
- Skipped steps are dimmed, with the condition that was false.
- Blocks and denials are red, with their reason.
- Delegations are shown as satisfied, owed or missed.
- A halted turn shows its stop reason on the card where the halt happened.

While the toggle is on, the view polls once a second, so the owner can watch a turn as it runs. The CLI equivalent is `treejev flow trace [session]`.

### 9. Who registers which hook

The owner's rule: hooks that make TreeJev work belong to TreeJev's setup. A plugin keeps only hooks that are specific to it. Anything a tree decision controls is a flow step, so that it shows in the lanes view as part of the sequence.

| Hook | Registered by | Why |
|---|---|---|
| `treejev-hook` on all five events | `treejev init`, in `.claude/settings.json` | This is TreeJev's own entry point: the flow runner, the turn record, logging of MCP tree runs, the tree report and the halt. Every project that uses TreeJev needs it, with or without research-suite. |
| research-suite `session-rules.sh --hook` (SessionStart) | research-suite `hooks.json` | It always runs and no tree decides it. |
| research-suite `refactor-gate.sh` (PreToolUse `Edit\|Write`) | research-suite `hooks.json` | It always runs and is specific to research-suite. |
| research-suite `design-rules.sh` and `readability.sh --hook post-tool` (PostToolUse `Edit\|Write`) | research-suite `hooks.json` | They always run on every edit, in every mode. |
| The prompt-turn work of `claude-md-hook.sh` (the turn tree, `-deep`, the confidence floor, mode text, design-rule context) | **flow steps** | A tree decides it. |
| `route-checks.sh` (all three events) | **flow** `owedChecks` steps | Tree actions decide which checks are owed. |
| `design-rules.sh`, `readability.sh` and `source-lines.sh --hook stop` | **flow steps** | `stop_gates_wanted` depends on the turn tree's mode. |
| `systematic-check.sh --hook stop` (the change trees) | **flow steps** | These are trees, run only when the turn tree chose `deep`. |

Flow commands find research-suite's scripts through a top-level `"plugins": ["research-suite"]` list in the flow. For each plugin listed, the runner resolves the install path from `installed_plugins.json` the same way §7 does, and exports it to every command as `TREEJEV_PLUGIN_<NAME>`, for example `TREEJEV_PLUGIN_RESEARCH_SUITE`. `${CLAUDE_PLUGIN_ROOT}` isn't available to hooks registered in `settings.json`. Resolving the path this way also follows plugin updates, since each version is cached under its own path (for example `…/research-suite/0.13.0`). If the plugin can't be found, the flow fails validation, which counts as TreeJev being broken and halts (§4). The owner sees "plugin research-suite is not installed for this project" rather than gates silently not running.

## Approaches Considered

- **Keep the sequencing in shell and generate documentation from it.** Rejected: the shell would still be the source of truth, and nothing could be edited visually.
- **A top-level tree made of encapsulated trees.** Rejected by the owner (decision 3). One walk happens at one moment, and it can't say "later, on Stop, if the prompt tree chose deep".
- **A general workflow engine** with expressions, loops and script steps. Rejected because of scope: conditions stay at equality, membership, regex and existence, and the effects list stays closed. Anything more goes in a project command.
- **A long-running TreeJev process for latency.** Deferred. A compiled CLI entry (step 1) brings startup from about 300 ms to well under 100 ms. A process is only worth adding if the PreToolUse measurements in step 8 call for it.
- **Lanes, closed conditions, closed effects and project commands** (chosen).

## Plan

Each step can be released on its own. Until step 8 the default flow reproduces today's behaviour, so SequenceTree keeps working throughout. Tests are written first and run with `npx jest` in TreeJev.

### Step 0 — Baselines

- **Goal:** every later step can be reverted.
- **Changes:**
  - Make an initial commit in TreeJev. It has none, so nothing there can be reverted yet.
  - Commit SequenceTree's move from `.treejev/` to `treejev/` (git status shows `D .treejev/…` and `?? treejev/`).
  - Make sure research-suite is clean.
- **Verification:** `git log` shows a commit in each repository. TreeJev's 272 tests pass.
- **Rollback:** none needed.

### Step 1 — A compiled CLI entry

- **Goal:** fast enough startup for hooks on every tool call.
- **Changes:**
  - Add `esbuild` as a dev dependency and a script `build:cli` that bundles `src/cli/index.ts` into `dist/cli.js` (platform node, cjs).
  - `bin/treejev` runs `dist/cli.js` when it exists **and** no file under `src/` is newer than it. Otherwise it runs through `tsx` as now, and prints `treejev: dist is stale, running from source` on stderr. A stale bundle can therefore never run old code without warning.
  - `postinstall` runs `build:cli`.
- **Tests first:** `tests/test_bin_launcher.test.ts`. A temp copy with an older `dist` picks `tsx`. A fresh `dist` picks `dist`.
- **Behaviour delta:** none in output. The baseline measured today is `treejev hook claude` on a no-op PostToolUse payload at 0.30–0.38 s (three runs). Record the after number in the implementation notes.
- **Verification:**
  - The jest suite passes.
  - Hook payloads give byte-identical output from `dist` and from `tsx`.
- **Rollback:** delete `dist/` and the launcher can only use `tsx`. Or revert.

### Step 2 — Flow model, validation and storage

- **Goal:** flows exist as validated files that the CLI can list and show.
- **Changes:**
  - `src/types/flow.ts`: `FlowEvent` (an enum-like union of the five event names), `FlowStepKind`, `FlowStep`, `FlowCondition`, `FlowInput`, `FlowCheck`, `FlowDocument`.
  - `src/models/flow-invariants.ts` `validateFlow(flow, treeLookup)`. It checks that:
    - step ids are unique;
    - `matcher` appears only in the tool lanes;
    - `treeReport` appears only on Stop and `block` never on SessionStart;
    - every `treeId` loads;
    - every override or floor label exists as an action label in its tree;
    - every plugin in `plugins` resolves in `installed_plugins.json` for this project;
    - every check name used by any reachable action exists in `checks`, with an event that has an `owedChecks` step;
    - every `when` path names a step that runs earlier, in an earlier lane or earlier in the same lane. A path to a later step is a warning.
  - `src/server/flow-repository.ts`: `listFlows`, `loadFlow`, `saveFlow` (validates first) and `getActiveFlowId` (`config.activeFlow`). It sits next to `tree-repository.ts` and uses the same `findLocalProjectTreeDir`.
  - CLI: `treejev flow list | show <id> | validate [id]`.
- **Tests first:** `tests/test_flow_invariants.test.ts` has one failing fixture per rule above and one valid fixture.
- **Behaviour delta:** none. Nothing reads flows yet.
- **Verification:** jest. `treejev flow validate` on a hand-written fixture in a temp project, never in a repository.
- **Rollback:** revert. The new files have no callers.

### Step 3 — The turn record

- **Goal:** one store for everything a turn decides.
- **Changes:**
  - `src/flow/turn-record.ts`:
    - `projectKey(projectDir)`;
    - `turnRecordPath(projectDir, session)`, under the global directory `config-storage.ts:31` resolves;
    - `startTurn`, which archives the previous record, keeps the last 20, and prunes session folders older than 30 days;
    - `readTurn`;
    - `updateTurn(…, mutate)`, which writes atomically under a lock.
  - `runClaudePromptHook`, `runClaudeToolHook` and `runClaudeStopHook` write the record **as well as** the `$TMPDIR` decision file and runs log they write now. research-suite still reads those until step 8.
  - `TurnDecision` stays the same.
- **Tests first:** `tests/test_turn_record.test.ts`, run with `TREEJEV_GLOBAL_DIR` set to a temp directory:
  - two project paths with the same folder name get different keys;
  - archiving, the cap of 20, and pruning after 30 days;
  - two simultaneous `updateTurn` calls (`Promise.all` over child processes) both land;
  - a stale lock older than 10 s is broken.
- **Behaviour delta:** a new folder under `~/.treejev/sessions/`. Nothing is written into the project. Hook outputs are unchanged.
- **Verification:**
  - jest;
  - `test_hook_cli.test.ts` still passes unchanged;
  - a real prompt in SequenceTree writes `~/.treejev/sessions/SequenceTree-<hash>/<session>/turn.json`, matching the decision file;
  - `git status` in SequenceTree shows nothing new.
- **Rollback:** revert. Nothing reads the record yet.

### Step 4 — Action effects

- **Goal:** actions declare what they do.
- **Changes:**
  - Add `ActionEffects` to `types/tree.ts`, plus `effects?` on `ActionDecisionConfig`.
  - Add `runBy?: 'walker' | 'agent'` on `EncapsulatorDecisionConfig`. The walker doesn't read it until step 6.
  - `evaluateNode` (`evaluator.ts:223`) copies `effects` into `ActionEvaluationResult`, interpolating `block` and `report` with `{{var}}` the way `directive` is. `runner.ts:114` carries them into `terminalAction`.
  - `validateTree` (`tree-invariants.ts:22`) checks the shape of `effects`.
  - UI: add `EffectsEditor` under `ActionConfigEditor`, and a `runBy` toggle in `EncapsulatorConfigEditor`.
- **Tests first:** extend `test_action_directive.test.ts` so that an action with `effects` reaches `terminalAction.effects` interpolated, and an action without `effects` gives `undefined`.
- **Behaviour delta:** none for existing trees, since none has `effects`. `formatEvaluationReport` is unchanged.
- **Verification:** jest. Open `change_area` in the editor, add a `block`, save, and reload.
- **Rollback:** revert. The field is optional.

### Step 5 — The lane runner

- **Goal:** `treejev hook claude` runs a flow.
- **Changes:**
  - `src/flow/lane-runner.ts`: `runLane(event, payload, flow)` → `{ output, record }`. It handles:
    - conditions and inputs;
    - tree resolution (overrides, then the floor);
    - command execution (`child_process.spawn` with `bash -c`, the environment above including the `TREEJEV_PLUGIN_<NAME>` paths, the payload on stdin, and a per-step timeout);
    - output merging (§4) and the `owedChecks` port;
    - repair: every failure §4 lists writes the repair file, notifies the owner, and returns the repair directive for its event, with `halted` written to the turn record.
  - `src/flow/plugins.ts`: resolves plugin install paths from `~/.claude/plugins/installed_plugins.json`, preferring the project-scope entry for this project over the user-scope one. Today SequenceTree has both: project 0.13.0 and a stale user 0.7.0.
  - `src/flow/default-flow.ts`: `defaultFlowFor(projectConfig)` builds the compatibility flow (prompt tree plus an optional `treeReport`).
  - `src/flow/conditions.ts`: evaluates the closed grammar.
  - `bin/treejev-hook`: a POSIX `sh` wrapper with no Node dependency (§4). It runs `treejev hook claude`; turns a non-zero exit or non-JSON output into repair; skips the flow in repair mode; and at Stop runs `doctor --health` to resume, keep repairing, or stop after 3 attempts. `treejev init` copies it into `~/.treejev/bin/`.
  - `runHookCli` (`hook-cli.ts:252`) loads the active flow, or the default flow, and calls `runLane`. The old handlers' bodies move into the runner. The function names stay as thin exports only if tests still import them; otherwise the tests switch to `runLane`.
  - An explicit tree id on the command line (`treejev hook claude sequence_tree_turn`, as `claude-md-hook.sh` calls it) keeps its current meaning: it runs that tree as a one-step prompt lane. That is the bridge until step 8.
- **Tests first:** `tests/test_lane_runner.test.ts`, all in `--mock` mode:
  - the default flow gives byte-identical output to the existing `test_hook_cli` cases;
  - `when` skips a step;
  - an override beats the tree;
  - the floor turns `research` at 0.35 into `code` and adds the note;
  - each failure §4 lists (bad flow, missing tree, walk error, a walk that ends on no action, unwritable record, missing plugin) writes the repair file and returns, per event, the owner notice plus the repair directive: context on UserPromptSubmit, a deny on PreToolUse, a block on PostToolUse and Stop. Each names the lane and step;
  - a failing project command does **not** halt;
  - `treejev-hook` (run as a child process against `treejev` stubs): a crashing stub enters repair, with `printf`-built JSON on each event; repair mode skips the flow on tool events; at Stop, a healthy stub resumes with the original prompt and the prompt lane's output, a repair that drops step ids counts as still broken, a still-broken stub blocks and counts the attempt, and the fourth Stop returns `continue: false`;
  - each output-merge row in §4;
  - a command that prints a native hook object is merged;
  - `owedChecks` denies on PreToolUse;
  - `stop_hook_active` turns a block into a report;
  - a command times out.
- **Behaviour delta:**
  - Successful runs: none for projects without a flow, which includes SequenceTree until step 8.
  - **Intended change:** a TreeJev failure now stops the agent. Today `runClaudePromptHook` reports the error in the context and exits 0 (`hook-cli.ts:146-158`), and `claude-md-hook.sh:39-42` turns the missing label into `code`.
- **Hook safety:** every step has a timeout, and a step that times out counts as a failure of that step. The runner catches all exceptions per step and turns TreeJev's own into the halt.
- **Verification:** jest. Pipe SequenceTree's real prompt and Stop payloads through `treejev hook claude` before and after and compare the output.
- **Rollback:** revert. Hook entries are unchanged.

### Step 6 — Agent-called trees

- **Goal:** a flow or a tree can hand a tree to the agent, and the Stop hook checks that the agent ran it.
- **Changes:**
  - `lane-runner.ts`:
    - `agentTree` steps;
    - `delegate` effects;
    - built-in logging on PostToolUse for `mcp__treejev__.*` (`parseTreeRuns`), which marks delegations satisfied and applies the delegated tree's own effects from the tool result's report;
    - the delegation check at the top of the Stop lane.
  - `walker.ts`: `startEncapsulatorTraversal` (`:129`) completes on an encapsulator with `runBy: 'agent'` without stepping inside it.
  - `runner.ts`: such a walk produces a `terminalAction` labelled with the encapsulator, whose `effects.delegate` is `[{ treeId: treeRef }]`.
  - The MCP tool result has to carry the effects. `formatEvaluationReport` gains `Effects:` lines (block, report, checks) only when the action has `effects`, so existing output is unchanged.
- **Tests first:**
  - `test_encapsulator.test.ts`: a `runBy: agent` encapsulator ends the walk with a delegate effect.
  - `test_lane_runner.test.ts`:
    - an unsatisfied delegation halts with `continue: false` and a `stopReason` naming the tree and its instruction, and the turn record marks it missed;
    - a satisfied one passes;
    - the delegated tree's `checks` become owed.
- **Behaviour delta:** none for trees and flows that don't use the feature.
- **Verification:** jest. In a temp project, a flow whose prompt lane delegates `pr_code_review`: a real Claude Code session must call the tool before Stop passes.
- **Rollback:** revert.

### Step 7 — Generated agent instructions

- **Goal:** the agent's instructions come from the flow.
- **Changes:**
  - `src/flow/instructions.ts` `renderFlowInstructions(flow, trees)`.
  - `treejev flow sync` writes `TreeJevAgentProtocol.md` and the Cursor rule. `saveFlow` calls the same function.
  - `writeAgentProtocol` (`init-project.ts:137`) is replaced by it.
  - The default flow's SessionStart lane gets an instructions step, available to any flow that turns it on.
  - `registerTreeTool` (`server.ts:54`) reads the active flow for agent-called instructions and drops the hard-coded signifier sentence. `treejev_evaluate` (`server.ts:270`) drops it too.
- **Tests first:** `tests/test_flow_instructions.test.ts` has snapshot tests for the default flow and for a flow with each step kind. `test_mcp.test.ts` checks that the description includes the delegation instruction.
- **Behaviour delta:**
  - `TreeJevAgentProtocol.md` becomes generated, so hand edits to it are overwritten. SequenceTree's copy has none beyond what init wrote; diff it before the first sync.
  - The MCP descriptions get shorter.
- **Verification:** jest. Run `treejev flow sync` in SequenceTree and review the diff of `TreeJevAgentProtocol.md` before committing.
- **Rollback:** revert, and restore the protocol file from git.

### Step 8 — Port SequenceTree and shrink research-suite

- **Goal:** SequenceTree runs entirely from `treejev/flows/sequence_tree.json`, and research-suite stops sequencing.
- **The flow:**

| Lane | Steps in order |
|---|---|
The flow lists `"plugins": ["research-suite"]`, and each command below runs as `"$TREEJEV_PLUGIN_RESEARCH_SUITE/bin/<script>"`.

| Lane | Flow steps in order | Hooks research-suite keeps (shown as grey cards) |
|---|---|---|
| SessionStart | — (instructions step, step 7) | `session-rules.sh --hook` |
| UserPromptSubmit | `snapshot`: command `turn-scope.sh --snapshot "$TREEJEV_SESSION"` · `turn`: tree `sequence_tree_turn`, with the `-deep` override and floor 0.5 → `code` · `design-context`: command `design-rules.sh --turn-context`, when `turn.steps.turn.action ∈ code, deep` · `deep-rules`: command `session-rules.sh`, when `… = deep` | — |
| PreToolUse | `owed` (`owedChecks`, so `edit-tools` runs on Bash) | `refactor-gate.sh` (`Edit\|Write`) |
| PostToolUse | `owed` (`owedChecks`, matcher `Edit\|Write`) | `design-rules.sh --hook post-tool`, `readability.sh --hook post-tool` (`Edit\|Write`) |
| Stop | `design-rules.sh --hook stop` · `readability.sh --hook stop` · `source-lines.sh` · `change_surface` and `change_area`: tree, input command `systematic-check.sh --state "$TREEJEV_SESSION"`, when `turn = deep` · `owedChecks` · `treeReport` | — |

- **Changes:**
  - **Tree JSON:** each `sequence_tree_turn` action's `context` takes over the mode paragraph that `claude-md-hook.sh:89-158` writes now. The `${sources}` and `${deep_trigger}` values are written out literally, since a tree belongs to one project.
  - **`checks`:** every action keeps `edit-tools` as now.
  - **Change trees:** `change_surface` and `change_area` actions that flag get `effects.block` with their directive text.
  - **`design-rules.sh --turn-context`** (new mode): prints the design-rule check plus the readability bullets that `claude-md-hook.sh:140-147` prints now. It keeps the rule "after the first prompt in `code` mode, print nothing when clean", using the same `claude-md-hook-<session>` marker. `deep` always prints, plus the deep paragraph (`:149-152`).
  - **`lib/gates.sh`:**
    - `turn_mode_marker` and `route_checks_marker` are deleted;
    - `stop_gates_wanted` reads `jq -r '.steps.turn.action' "$TREEJEV_TURN_FILE"`;
    - the `[gates]` keys `turn_tree`, `turn_confidence_floor`, `fallback_route_checks`, `change_trees` and `route_checks` are no longer read.
  - **Deleted:** `bin/claude-md-hook.sh` and `bin/route-checks.sh`. `systematic-check.sh` keeps only `--state`.
  - **`hooks/hooks.json`** keeps only the four hooks no tree decides (the right-hand column above) and loses the other eight. It registers no `treejev` entry: TreeJev's entry points are TreeJev's setup (§9).
  - **SequenceTree:**
    - `treejev init claude report` re-registers the five `treejev-hook` entries in `.claude/settings.json`, replacing today's two `treejev hook claude` entries;
    - `treejev/config.json` gains `"activeFlow": "sequence_tree"`;
    - `refactor.toml [gates]` drops the five moved keys;
    - this file's *Refactoring Commands* and *Analysis* sections, and the TreeJev protocol, are updated to say the flow is the sequence.
  - Bump research-suite to 0.14.0 and run `claude plugin update research-suite@research-suite --scope project`. `doctor` then confirms that `TREEJEV_PLUGIN_RESEARCH_SUITE` resolves to the 0.14.0 path.
- **Golden comparison (the main verification):**
  - **Before the port,** capture `additionalContext` from the current `claude-md-hook.sh`, mock mode, for six fixed prompts: one per label, one with `-deep`, and one forced below the floor.
  - **After the port,** capture the flow's UserPromptSubmit output for the same prompts. The two must match apart from whitespace, and the implementation notes must name every difference.
  - **Stop:** the same comparison on two saved diffs, the `ConnectionOps.h` reorder (no flag) and the `connectsToOtherTreeRoot`-made-private worktree (surface flag).
- **Behaviour delta:**
  - Intended:
    - five TreeJev registrations plus four research-suite ones, instead of 12 plus 2;
    - gates read the turn record instead of markers;
    - a TreeJev failure enters repair instead of falling back to `code`.
  - Every other difference the golden comparison finds is a bug to fix before this step is called done.
  - Measure PreToolUse latency on Bash before and after. If the median rises by more than 150 ms, implement the long-running-process approach before continuing.
- **Verification:**
  - the golden comparison;
  - a real `-deep` turn in a restarted Claude Code that edits a scratch branch and blocks once on a flagged change;
  - a real `light` turn that runs no Stop gates;
  - a real turn with `sequence_tree_turn.json` temporarily renamed: you see the notice; the agent finds the missing file, restores it, the Stop health check passes, and the original request then runs in the same session with its normal turn decision; and a second run where the fix is deliberately withheld stops after 3 attempts with the `stopReason`;
  - `treejev flow trace` shows all three turns.
- **Rollback:** revert research-suite to 0.13.0, run `claude plugin update`, and revert SequenceTree's config commit. The flow file can stay, because nothing reads it without `activeFlow`.

### Step 9 — The lanes view

- **Goal:** see and edit the sequence.
- **Changes:**
  - `WorkspaceTab.kind`.
  - `src/components/flow/FlowCanvas.tsx`: React Flow with lane background nodes, step nodes laid out by lane and order, and data edges derived from `when` paths.
  - `FlowStepCard.tsx`.
  - `src/components/inspector/FlowStepEditor.tsx`, `ConditionEditor.tsx`, `InputEditor.tsx`, and `ChecksRegistryPanel.tsx`.
  - `src/models/flow-operations.ts` (add, move, delete and update step; pure functions in the style of `tree-operations.ts`).
  - `app/api/jev/flows/route.ts` (GET, PUT, validate).
  - `ProjectSidebar` lists flows above trees.
  - `page.tsx` renders `FlowCanvas` for flow tabs.
  - Undo and redo use the existing `TreeHistory` mechanism, made generic over the document type.
- **Tests first:**
  - `tests/test_flow_operations.test.ts` covers the pure operations.
  - `tests/test_flow_canvas.test.tsx` renders a fixture flow and checks five lanes, the step order, one data edge per `when` dependency, and that an invalid step shows its problem.
- **Behaviour delta:** UI only. Saving goes through `saveFlow` validation, then `flow sync` (step 7).
- **Verification:**
  - `npm run verify:ui`;
  - open SequenceTree's flow and confirm it matches the table in step 8;
  - move `owedChecks` in Stop, save, and confirm `treejev flow show` reflects the move.
- **Rollback:** revert. Flows stay editable as JSON.

### Step 10 — The last-turn trace

- **Goal:** watch what the flow did.
- **Changes:**
  - `app/api/jev/traces/route.ts`: lists projects and sessions under `~/.treejev/sessions/`, and returns one session's current turn and history.
  - A trace overlay mode in `FlowCanvas`.
  - A **Sessions** list in `ProjectSidebar`, so traces of every steered project can be opened from the app.
  - `treejev flow trace [session] [--turn n]`.
- **Tests first:** `tests/test_flow_trace.test.ts` maps a fixture turn record onto card states: ran, skipped (with its reason), blocked, halted (with its stop reason), and delegation satisfied or missed.
- **Verification:** run a real turn and watch the cards update while the toggle is on.
- **Rollback:** revert.

### Step 11 — `treejev init` and `doctor` for flows

- **Goal:** any project gets a flow on setup.
- **Changes:**
  - `initProjectTreeJev` (`init-project.ts:183`) writes `flows/default.json`: the prompt tree, an instructions step and `treeReport` when `report` is given. It sets `activeFlow`.
  - It always registers the five TreeJev entry points in `.claude/settings.json` through `setClaudeTreeJevHook`, each running the `treejev-hook` command with its `|| printf` stop for a missing wrapper (§4). It installs `~/.treejev/bin/treejev-hook`. These hooks make TreeJev work, so they belong to its setup and not to any plugin (§9).
  - It merges `config.json` instead of rewriting it, which fixes the known reset of `evaluationMode` to `mock`.
  - `research-init` calls `treejev init claude` and writes the research-suite flow template, which lists `"plugins": ["research-suite"]`.
  - `treejev doctor` reports:
    - a `treejev` entry registered anywhere other than TreeJev's own setup (a plugin's `hooks.json`, a hand edit);
    - a plugin hook that duplicates a flow step;
    - an invalid active flow, or a plugin it lists that isn't installed for this project;
    - a stale `dist`;
    - delegations to trees that don't exist.
- **Tests first:** extend `test_init_project.test.ts`:
  - init writes a valid default flow;
  - init writes exactly five TreeJev entries, each ending in the `|| printf` stop, and keeps other hooks on those events;
  - re-running init keeps `evaluationMode: live`;
  - doctor finds a planted `treejev` entry in a fake plugin `hooks.json`.
- **Verification:** jest. `treejev init claude report` in a scratch project outside every repository, then a real session there.
- **Rollback:** revert.

## Risks

- **One point of failure.** Every tree-decided gate now goes through TreeJev. That is accepted on purpose: a failure is announced and repaired at once, the wrapper catches crashes without Node, and `doctor` catches configuration problems ahead of time.
- **Repair runs without the tree-decided gates.** While the repair file exists the flow is skipped, so the repair edits themselves aren't checked by the flow; research-suite's own hooks (§9) still run. The agent could also "fix" TreeJev by weakening it. The step-id check catches removed steps, but not edited directives or conditions. So the resume notice lists every file the repair changed, for the owner to read.
- **Live Jev API outages trigger repair.** A tree that can't reach the API is a walk error, and the agent can't fix someone else's server. Its repair attempts will fail and hit the 3-attempt stop. If outages turn out to be frequent, the owner can switch `evaluationMode` to `mock`, which never makes network calls.
- **Latency on tool hooks.** Measured at about 300 ms on `tsx` today. Step 1 is required before step 8, and step 8 has a numeric threshold for continuing.
- **Races from parallel tool calls** writing the turn record. Mitigated by the lock and atomic rename, with a test in step 3.
- **Scope creep in conditions and effects.** Both are closed lists that `validateFlow` enforces. New needs go in commands.
- **Exact text parity in the port.** The golden comparison in step 8 is the gate. Mode paragraphs that move into tree JSON can drift later, but they are now edited in one visible place.
- **Generated protocol overwrites hand edits.** Step 7 diffs before the first sync.

## Out of Scope

- Hook events beyond the five (`SubagentStop`, `PreCompact`, `Notification`). The `FlowEvent` union can grow later.
- Lifecycle hooks for Cursor or Antigravity. They get generated static instructions only.
- Moving research-suite's checks themselves (design rules, readability, tests) into TreeJev. They stay project commands, by decision 1.
- Changes to `Source/`.

## Decisions Made (2026-10-09)

1. **Agent-run encapsulators:** all three forms are kept: the flow step, the `delegate` effect, and `runBy: agent` on encapsulators inside trees (§5, step 6).
2. **Hook registration:** TreeJev's entry points are registered by `treejev init`. research-suite keeps only its own hooks that no tree decides, and everything a tree decides is a flow step (§9, steps 8 and 11).
3. **TreeJev broken:** the owner is told at once and the task stops. The agent then repairs TreeJev on its own and resumes the original request, and stops for the owner only after 3 failed attempts (§4).
4. **Delegated tree never run:** reported to the owner, and the agent halts (§5).
5. **Traces:** stored with the TreeJev application under `~/.treejev/sessions/`, never in the project (§2, §8).

## Decisions for the Owner

Answered during implementation (2026-10-09):

1. **research-suite baseline:** commit research-suite's current state (including another session's work in progress) as one baseline commit, then edit. Done as 6bfd402; the plugin goes to 0.15.0, since 0.14.0 was already taken.
2. **Lanes view placement:** flow tabs in the workspace, as planned (`WorkspaceTab` gains a kind), with a separate path that saves a flow back to its project's `treejev/flows/`.

## Implementation Notes

All steps are built and tested. TreeJev ends with 362 jest tests passing and `tsc --noEmit` clean. Nothing after the step 0 baselines is committed.

- **Step 0:** TreeJev's initial commit is 2a388b8, and SequenceTree's ecd9950 commits the move from `.treejev/` to `treejev/`. research-suite's last commit was plugin 0.6.1, with everything since then uncommitted and another session editing it. On the owner's decision, 6bfd402 commits that state as a baseline, including the other session's work in progress.
- **Step 1:** `scripts/build-cli.js` bundles the CLI with esbuild 0.28.2 (pinned as a devDependency). `bin/treejev` runs the bundle unless a file under `src/` is newer. A no-op hook went from 0.30–0.38 s to 0.15 s.
  - **Unplanned incident:** the first run of the bundle executed `init-mcp.ts`'s `require.main === module` block, because every module in a bundle counts as the main module. The MCP auto-configurator then rewrote four files:
    - TreeJev's `.agents/mcp_config.json`, restored from git;
    - TreeJev's `.cursor/mcp.json`, newly created, deleted;
    - `~/Library/Application Support/Claude/claude_desktop_config.json` and `~/.gemini/config/mcp_config.json`. In both, the `treejev` entry is now `npx tsx <TreeJev>/src/mcp/index.ts` with `cwd` set to TreeJev. The previous `treejev` values can't be recovered; every other key was kept.
  - The self-run block is deleted, and `npm run mcp:init` now goes through `index.ts mcp init`. `test_bin_launcher.test.ts` fails if the old block comes back.
- **Step 2:** done (`types/flow.ts`, `models/flow-invariants.ts`, `server/flow-repository.ts`, `flow/plugins.ts`, `treejev flow list|show|validate`). Unknown check names are errors only when the flow has a `checks` registry, and warnings otherwise.
- **Step 3:** done (`flow/turn-record.ts`). Jest gives every test file its own `TREEJEV_GLOBAL_DIR` (`tests/setup-global-dir.js`).
- **Step 4:** done (effects, `validateTree` shape checks, `EffectsEditor`, the Run By toggle).
- **Step 5:** done. Differences from the plan:
  - An override still runs the tree, so the turn record keeps what it chose. The report header names the final action, and a note names the tree's own answer; `FlowOverride.reason` was added for that note.
  - The old hook functions and the `.runs.json` log are removed. The `$TMPDIR` decision file is still written for the first prompt tree.
  - A command's `suppressOutput` carries over only when every report in the merged output asked for it.
- **Step 6:** done. The in-tree form uses `runBy: 'agent'` plus an optional `instruction`. The `Effects:` line appears only in MCP tool results (`formatEvaluationReport(result, true)`), because in hook context it was noise on every turn.
- **Step 7:** done. The protocol also has an "Agents without hooks" section. SequenceTree's `TreeJevAgentProtocol.md` is now generated from `sequence_tree.json`.
- **Step 8:** done.
  - **Gates read the mode from `values.mode`:** each action in `sequence_tree_turn` and `sequence_tree_change` sets it with `set`, and research-suite reads it through `$TREEJEV_TURN_FILE`. That replaces reading a step id that exists only in SequenceTree's flow.
  - **Deep rules:** they stay inside `design-rules.sh --turn-context` (with the `---` separator) rather than being a separate step, which keeps the text identical.
  - **`systematic-check.sh --state`:** it now reads the session from `$2`. It used to read `$3`, so it always diffed against `HEAD`.
  - **Checks keep their old guards inside the command:** `tests` runs only when `turn-scope --files` is non-empty, and `repeat` only for files under `Source/`.
  - **`hooks.json`:** research-suite keeps four hooks.
  - **Versions:** the plugin went to 0.15.0 (0.14.0 was already taken), then to 0.15.1 for the empty-mode-means-`code` rule in `--turn-context`, which the `research-init` template relies on.
  - **Golden comparison:** six live prompts went through the old `claude-md-hook.sh` and through the flow, and every prompt reached the same mode. The text is identical apart from three differences:
    - blank lines;
    - the light and research paragraphs now arrive as the action's `Context:` line;
    - under `-deep`, the header and directive are deep's, with the note, where before they were the tree's raw `research`.
  - **Stop comparison:** for a deep session against the `ConnectionOps.h` diff, Stop has the same content, with the `systematic-check:` summary line replaced by the tree report lines.
  - **Latency:** a PreToolUse on Bash takes 0.20 s, against about 0.07 s for the old hook's parts (+130 ms, under the 150 ms limit).
- **Step 9:** done as flow tabs (`WorkspaceTab.kind`, `FlowTabState`), as the owner chose.
  - Flow tabs load from and save to `<project>/treejev/flows/` through `/api/jev/flows` and are left out of the patch autosave.
  - Undo and redo use a separate flow history (`models/flow-operations.ts`); `TreeHistory` was not made generic.
  - Double-clicking a tree card opens a copy of the project's tree in a patch tab, and the app says so.
  - The lanes layout is a pure `models/flow-layout.ts`, tested directly, because React Flow does not render statically.
  - **Unplanned:** the first request to the flows API ran `ensureTreesDirectory`, which copied three TreeJev trees into `SequenceTree/treejev/trees` (`agent_deployment_gate`, `code_procedures`, `project_preferences`). They were untracked and have been deleted. `withProjectDir` now sets `TREEJEV_NO_SEED`, and a test covers it.
- **Step 10:** done (`flow/trace.ts`, `/api/jev/traces`, `treejev flow trace`, the Last turn overlay with session and turn pickers). Sessions are listed in the Flows browser through the project picker rather than as a separate sidebar list.
- **Step 11:** done.
  - init writes the default flow only when `activeFlow` is `default`, keeps the config keys it does not own, registers five `treejev-hook` entries and installs the wrapper.
  - `research-init` runs `treejev init claude`, installs `flows/research-suite.json` as `research_suite` and syncs the protocol.
  - `treejev doctor` checks hook registration, plugins that register TreeJev or repeat a flow step, the active flow and a stale bundle.

**Manual checks still owed:**
- After restarting Claude Code (research-suite 0.15.1 and the new `settings.json` hooks), confirm a `-deep` turn that makes a flagged change blocks once.
- Rename `treejev/trees/sequence_tree_turn.json` and send a prompt: you should get the notice, the agent should repair and resume, and three failed repairs should stop it.
- Run a session against a flow that hands a tree to the agent.
- Check the lanes view in the TreeJev app: open SequenceTree's flow from the Flows section, drag a card, save, and turn on Last turn. The browser tool could not start to check this.
- Edit an action's effects in the inspector and reload.

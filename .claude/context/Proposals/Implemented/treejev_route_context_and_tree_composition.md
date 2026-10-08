# Route Context, Route Checks and Trees That Call Trees

> Status: Implemented
> Implemented 2026-10-08, uncommitted (TreeJev has no commits; research-suite and SequenceTree changes are in the working trees).
> Built from 46b407c; drift: `runner.ts` and `hook-cli.ts` had just gained `lowestConfidence`/`confidence` (treejev_fail_closed_turn_decisions.md step 2); lines moved, shape unchanged
> Written 2026-10-08 at 46b407c (SequenceTree), research-suite ecdd8e0 (plugin 0.10.1), TreeJev working tree (no commits). Sources: Reviewed/ResearchReports/treejev_systemic_steering_design_study.md (What Survives: P2 "encapsulators cannot reference another tree, and they rename the terminal label"; P3 "`contextPayload` … is a sound, small schema extension"; P3 "candidate tree structure"; ledger #6, #8, #10, #11, #12, #13; Questions 5–7)

## Summary

The owner wants the turn tree to steer which context a turn gets and which extra checks run, through a router that hands off to more specific trees such as a refactor tree and a style tree. Three things in TreeJev block that today:

- An action can carry only a directive.
- An encapsulator cannot refer to another tree file. It runs only an embedded `subgraph`.
- An encapsulator's terminal label comes back as `[patchName] code`, which `claude-md-hook.sh` rejects and silently turns into `code`.

The plan has five steps:

1. Make the terminal label the action's own label.
2. Add a `treeRef` to encapsulators, resolved when a tree is loaded.
3. Give actions a `context` text and a `checks` list, carried into the turn report and into `TurnDecision`.
4. Teach research-suite to run named route checks. The commands live in the project's `refactor.toml`, never in tree JSON, and run on PostToolUse or at Stop, so they check the code the turn writes.
5. Add a `sequence_tree_change` tree under the turn tree's `code` branch.

Every leaf keeps one of the five labels the hook accepts. The baseline gates stay unconditional: route checks only add. The change is medium-sized: about 120 lines of TypeScript and shell, one new script, one new tree, and no C++. **P3.**

## The Problem

### Mechanism

**1. A terminal action reached through an encapsulator is renamed.**

- `startEncapsulatorTraversal` (`TreeJev src/traversal/walker.ts:129`) and `stepActiveSubtraversal` (`:58`) record the outer step with `nodeLabel: \`[${patchName}] ${subStep.nodeLabel}\`` (`:205`, `:108`) and `result: subStep.result`.
- `runTreeToCompletion` takes the terminal label from `lastStep.nodeLabel` (`src/mcp/runner.ts:84`), so `finalAction` becomes `[Change] code`.
- `claude-md-hook.sh:30-33` accepts only `skip|light|research|code|deep` and maps anything else to `code`.
- The routing is lost silently. A sub-tree that chose `deep` arrives as `code`.

**2. An encapsulator cannot point at another tree.**

- `EncapsulatorDecisionConfig` (`src/types/tree.ts:40-48`) has `patchId`, `patchName`, `patchFile`, `subgraph`, `exitNodeId` and `outputBranches`.
- Only `subgraph` is executed (`walker.ts:136-160`).
- `patchFile` and `patchId` are written by the editor's loader (`src/components/inspector/EncapsulatorConfigEditor.tsx:47-56`) and read by nothing that runs a walk.
- With no `subgraph`, the walk completes on the encapsulator with no action (`walker.ts:139-159`). `finalAction` is then null and the hook falls back to `code`.
- Embedding a copy works, but the copy drifts from the tree it was copied from, and the MCP server already publishes every tree file as its own tool (`registerTreeTool`, `server.ts:51`).

**3. An action can only say one thing.**

- `ActionDecisionConfig` (`tree.ts:27-32`) has `description`, `directive` and `outcome`.
- There is nowhere to put route-specific context (for example, "read `REFACTORING.md`" on refactor turns, ledger #8), or to name an extra check the route should run.
- The report's `executeCommand` idea (ledger #6, #13) would put shell commands in tree JSON. That gives every tree file arbitrary execution through a runner the MCP server also exposes (review question 7), and it ran at prompt time, before the turn's edits existed.

**Concrete input.**

- The owner writes "lift the replay catch-up out of `continueReplay`". The turn tree classifies it `code`.
- Today that turn gets the generic `code` directive. Nothing points the model at `refactor.encap` or the walk-parity test, and no test runs at Stop.
- With the trees in step 5, the `code` branch enters `sequence_tree_change`, which picks `refactor` → `traversal`. The leaf is labelled `deep`. Its `context` names the extract command and the parity test, and its `checks: ["tests"]` makes the Stop hook build and run `SequenceTree_Tests` and `SequenceTree_GraphTests` once Source has changed.
- Under the current walker that same tree would report `[Change] deep`, and the hook would run the turn as `code`.

### Evidence

- The review's ledger #11 (Refuted: `patchName` does not load a tree; the prefixed label fails the hook) and #12 (only five labels are accepted), and *What Survives* items 3–5.
- Re-read today: `walker.ts:108,205`, `runner.ts:81-90`, `tree.ts:27-48`, `evaluator.ts:238-247` (the action result carries no label), `claude-md-hook.sh:30-33`. The lines are unchanged from the review.
- Not yet run. A TreeJev test with an encapsulator whose inner leaf is labelled `code` would show `finalAction === '[…] code'` on the current code. Step 1 adds that test and records it failing first.

### Why It Matters

This is what the owner's design study asked for: trees that steer per-route context and checks (Subject Coverage rows 3–5). Without step 1, any multi-tree design misroutes silently. Without steps 2–4, a route can only change wording.

Nothing is broken today, because the project runs one flat tree. Context injection can only add to what SessionStart and CLAUDE.md load (ledger #5, #8), so the value is material that is *not* already loaded, plus checks the baseline gates do not run. That is the design below. **P3.**

## Current Design

### API Surface

TreeJev:

- `TraversalStep.nodeLabel`, set in `stepTraversal` (`walker.ts:273`), `stepActiveSubtraversal` (`:108`) and `startEncapsulatorTraversal` (`:144`, `:205`). Read by `runTreeToCompletion` for `decisionPath` (`runner.ts:63-74`) and for `terminalAction.label` (`:85`), and by the editor's traversal panel through `useTreeTraversal` (`src/hooks/useTreeTraversal.ts:5`, browser, imports `walker` only).
- `evaluateNode(node, state, useMock, options)` (`evaluator.ts:223`) builds `ActionEvaluationResult` (`types/jev.ts:25-31`: `kind, description, directive, resolvedDirective, outcome`) at `:238-246`.
- `TraversalExecutionResult.terminalAction` (`runner.ts:12-18`). Consumers: `runClaudePromptHook` (`hook-cli.ts:103-104`), `formatEvaluationReport` (`server.ts:40-44`), `runOneUnit` (`unit-runner.ts:97-113`).
- `loadTree(identifier)` (`src/server/tree-repository.ts:235-274`). It resolves the file, parses it, runs `validateTree` (`src/models/tree-invariants.ts`), and returns `{graph, treeId}`. Callers: `runClaudePromptHook` (`hook-cli.ts:82`), `runRunCli` (`run-cli.ts:96`), MCP `registerTreeTool` (`server.ts:58`, `:94`), `treejev_get_tree` (`:210`), `treejev_evaluate` (`:278`). The editor UI does not use it (`src/storage/client-tree-storage.ts`).
- `TurnDecision` (`hook-cli.ts:17-26`) and `formatEvaluationReport` (`server.ts:36-46`), as in `treejev_fail_closed_turn_decisions.md`.

research-suite:

- `claude-md-hook.sh`, the per-prompt router.
- `lib/gates.sh`, the config bridge (`:27-…`), `turn_mode_marker` (`:84`) and `stop_gates_wanted` (`:88-97`).
- `hooks/hooks.json`. PostToolUse on `Edit|Write` runs `design-rules.sh --hook post-tool` and `readability.sh --hook post-tool`, each reporting through `hookSpecificOutput.additionalContext` (`design-rules.sh:467`, `readability.sh:459`). Stop runs `design-rules.sh`, `readability.sh`, `systematic-check.sh` and `source-lines.sh`, and blocks with `{decision: "block", reason}` (`design-rules.sh:478`).
- `bin/repeat-check.sh`, which runs `refactor.reporting.repeat_check`. It is an informational "these added lines repeat existing code" report with a `--hook post-tool` mode (`repeat_check.py:68-75`). It is **not wired into `hooks.json`** today, which makes it a ready-made route check.

SequenceTree:

- `.treejev/trees/sequence_tree_turn.json`. Root Choice `classify`, options `control→skip`, `simple→light`, `research→research`, `code→code`, `deep→deep`. Each option's `childId` is an action node with the same id.
- `.claude/refactor.toml` `[gates]`.

### Structure

- TreeJev owns the walk and what a decision record contains. research-suite owns what records mean for hooks and gates, and runs every command. SequenceTree owns its trees and its configuration, including which commands a named check means.
- Commands appear only in the project's committed `refactor.toml`, which research-suite already reads, and never in a tree file. That is the containment the review's question 7 asked for: a tree can *select* among checks the project has defined, and cannot *define* one.
- The walker is shared with the browser editor (`useTreeTraversal`), so it cannot read files. A tree reference must therefore be resolved where trees are read from disk, in `loadTree`, which is server-side only.

### Data Flow

```
prompt ─▶ claude-md-hook.sh ─▶ treejev hook claude sequence_tree_turn
            loadTree: parse → resolve treeRef (inline sequence_tree_change as subgraph) → validate
            runTreeToCompletion: classify ─code─▶ [encapsulator change] ─▶ kind ─refactor─▶ scope ─traversal─▶ action "deep"
            terminalAction {label: "deep", directive, context, checks: ["tests"]}
            additionalContext: [TreeJev … | Final Action: deep] / Directive / Context
            TurnDecision {finalAction, …, context, checks} → $TMPDIR/treejev/<s>.json
          ├─ .finalAction → turn_mode_marker            (unchanged)
          └─ .checks      → route_checks_marker          (new)
Edit|Write ─▶ PostToolUse: design-rules, readability (unchanged) + route-checks.sh post-tool (marker ∩ post-tool checks)
Stop      ─▶ existing gates (unchanged) + route-checks.sh stop (marker ∩ stop checks, only if turn-scope --files non-empty)
```

## Approaches Considered

1. **Reference-by-id encapsulators resolved at load, action `context` and `checks`, and named checks defined in project config (recommended).** It keeps every leaf on the hook's five labels, so `claude-md-hook.sh`'s mode logic is untouched. It never puts a command in a tree. Checks run when the code exists (PostToolUse and Stop), which answers "while working". Cost: five steps across three repositories. Risk: the editor UI does not know `treeRef` (see *Risks*).
2. **Replace the five modes with richer labels (`refactor`, `style`, …) and rewrite `claude-md-hook.sh`'s `case`** (the report's master router). Every gate script keyed on the mode marker (`stop_gates_wanted`, `systematic-check.sh`'s deep handling) would need the new vocabulary, and every label would need a gate policy of its own. It is a larger rewrite for the same effect, because "refactor" is a *kind* of `code` turn, not a different gate regime. Rejected.
3. **Action `executeCommand` run by TreeJev** (the report's design). It runs at prompt time, before the edits it should check. It duplicates the prompt-time `design-rules.sh` run (`claude-md-hook.sh:91`), and it gives tree JSON shell execution through MCP. Rejected (ledger #6, #13).
4. **Flatten instead: add refactor, style and traversal leaves straight into `sequence_tree_turn`.** This needs no TreeJev change beyond `context`/`checks`. But one Choice with 8–9 options degrades classification: the five-way root already returns 27–42% confidence on some prompts. It also leaves the tree without the composition the owner asked to explore. It is a fallback if step 2 is declined.

**Recommendation: 1.** It is the smallest design that delivers per-route context and per-route checks without weakening any gate or putting execution in tree files.

## Plan

Prerequisite: `treejev_fail_closed_turn_decisions.md` step 0 (the initial TreeJev commit). Steps 1–3 below also go better after that proposal's steps 1–2. A sub-tree adds Choice calls, and each one is another place where a malformed or low-confidence answer could route silently. The record's lowest confidence covers sub-tree steps too, because the outer history steps carry the inner results (`walker.ts:109`, `:206`).

### Step 1 — The terminal label is the action's own label

- **Goal:** a walk that ends inside an encapsulator reports the inner action's label unchanged.
- **Changes:**
  - `src/types/jev.ts`: add `label: string` to `ActionEvaluationResult`.
  - `src/traversal/evaluator.ts` `evaluateNode`: set `label: node.label` in `actionResult` (`:240-246`).
  - `src/mcp/runner.ts:85`: `label: actRes.label`.
  - `decisionPath` keeps the prefixed `nodeLabel` (`:70`), so the path still shows which patch the action came from.
- **Tests first:** add a case to `tests/test_encapsulator.test.ts`. Use the existing parent-plus-subpatch fixture at `:196`, with the subpatch ending in an action labelled `code`, run through `runTreeToCompletion(…, true)`, and expect `terminalAction.label === 'code'`. Record it failing on the old code (`'[…] code'`).
- **Behaviour delta:**
  - Trees without encapsulators: identical, because `node.label === step.nodeLabel`.
  - Walks ending inside an encapsulator: the label loses its `[patch]` prefix in `finalAction`, in the report header and in unit verdicts (`unit-runner.ts:108`). Nothing in this project depends on the prefix.
- **Invariants:** none in SequenceTree.
- **Verification:** `npx jest` in TreeJev. `treejev run sequence_tree_turn --mock --state '{"prompt":"refactor this"}'` is unchanged.
- **Rollback:** `git revert` in TreeJev.

### Step 2 — Encapsulators that reference a tree

- **Goal:** `{"kind":"encapsulator","treeRef":"sequence_tree_change"}` runs that tree file as the sub-walk.
- **Changes:**
  - `src/types/tree.ts`: add `treeRef?: string` to `EncapsulatorDecisionConfig`.
  - `src/server/tree-repository.ts`: in `loadTree`, after `validateTree` passes (`:265-271`), walk `graph.nodes`. For each encapsulator with a `treeRef`:
    - If the node also has a `subgraph`, throw `Encapsulator "<id>" has both treeRef and an embedded subgraph`.
    - Otherwise call `loadTree(treeRef)` recursively, carrying the set of tree ids already open. A repeat throws `Tree reference cycle: a → b → a`.
    - Set `config.subgraph` to the loaded graph, and `config.patchName` to the referenced tree's `name`, falling back to its id when `patchName` is empty. Leave `exitNodeId` unset.
    - Write it as a loop inside `loadTree` over a local `pending` list, not as a new one-call helper. If it grows past readability, decision 4 is whether to extract a named `resolveTreeReferences`.
  - Leaving `exitNodeId` unset makes the walker fall back to `findDeepestNode` (`walker.ts:163`). Referenced trees must therefore end every branch in an action, so that reaching any leaf completes the sub-walk (`walker.ts:82-83`). The resolve loop enforces this: it throws `Referenced tree "<id>" has a non-action leaf "<node>"` when a node with no children is not an action.
- **Tests first** (new `tests/test_tree_reference.test.ts`, written against a temp `TREEJEV_TREES_DIR` the way `test_hook_cli.test.ts:55-64` does):
  - a parent whose `code` option leads to a `treeRef` encapsulator, and a child tree that ends in `deep`, gives `finalAction` `deep` (after step 1);
  - a missing reference throws `not found`;
  - a self-reference throws `cycle`;
  - `treeRef` together with `subgraph` throws.
- **Behaviour delta:** trees without `treeRef` are unchanged. `treejev_get_tree` counts top-level nodes only (`server.ts:211-214`), so its summary is unchanged.
- **Invariants:** the walker stays browser-safe, because resolution happens only in the Node-side loader.
- **Verification:** `npx jest`. `treejev run` on a scratch parent and child pair in a temp project (not in this repo), `--mock`.
- **Rollback:** `git revert` in TreeJev. No existing tree uses `treeRef`.

### Step 3 — Actions carry `context` and `checks`

- **Goal:** a route can add context the session has not already loaded, and name extra checks, and both reach the hook.
- **Changes:**
  - `src/types/tree.ts` `ActionDecisionConfig`: add `context?: string` (supports `{{var}}` like `directive`) and `checks?: string[]`.
  - `src/types/jev.ts` `ActionEvaluationResult`: add `context?: string` and `checks: string[]`.
  - `evaluator.ts` `evaluateNode`: set `context: interpolateTemplate(node.config.context || '', state)`, and set `checks` to `node.config.checks`, or `[]` when it is absent. Write both with `if` blocks rather than `??`/ternaries in new lines.
  - `runner.ts`: add `context?: string` and `checks: string[]` to `terminalAction` (`:12-18`) and fill them at `:83-89`.
  - `server.ts` `formatEvaluationReport`: after the `Directive:` line, append `\nContext: ${context}` when `context` is non-empty. The first line, which is the protocol's signifier, is unchanged.
  - `hook-cli.ts`: add `context: string | null` and `checks: string[]` to `TurnDecision`, and set them in the `decision` literal (`:99-108`).
- **Tests first:**
  - In `test_action_directive.test.ts`, an action with `context: 'Read {{doc}}'` and `checks: ['tests']` run with state `{doc: 'X'}` gives `terminalAction.context === 'Read X'` and `checks` `['tests']`, and the report contains `Context: Read X`.
  - In `test_hook_cli.test.ts`, the recorded decision has `checks`.
- **Behaviour delta:**
  - Actions without the fields: the report is unchanged, and the decision gains `context: null, checks: []`, which the hook ignores until step 4.
  - The `/api/jev/evaluate` route and the editor are untouched, but `ActionConfigEditor.tsx` must be checked so that saving an action from the UI keeps unknown fields (`:28`, `:91` build the new config). If it drops them, the UI would silently strip `context` and `checks`. Verify and, if needed, spread the existing config in those two updates.
- **Verification:** `npx jest`, plus the `ActionConfigEditor` check above (open a tree with `checks` in the editor, change the directive, save, and diff the JSON).
- **Rollback:** `git revert` in TreeJev.

### Step 4 — research-suite runs named route checks

- **Goal:** a route's `checks` run on the code the turn writes, with commands only from project config, and never in place of the baseline gates.
- **Changes** (research-suite):
  - `lib/gates.sh`:
    - Add `route_checks_marker()` beside `turn_mode_marker` (`:84`), giving `"${TMPDIR:-/tmp}/claude-route-checks-<session>"`.
    - Bridge `[gates.route_checks]` into a shell variable `route_checks`, one line per check: `name<US>event<US>command`, in the same `\x1f` encoding `project_checks` uses (`:59-…`).
    - Each entry is a table `{ event = "post-tool" | "stop", command = "…" }`.
  - `bin/claude-md-hook.sh`:
    - After `:27`, write `jq -r '(.checks // [])[]' "$decision"` to `$(route_checks_marker "$session")`. When the tree did not run, or `-deep` overrode it, write an empty marker. With `-deep`, the tree's checks still apply, because deep only adds.
    - For any name not defined in `route_checks`, add `Route check "<name>" is not defined in [gates.route_checks].` to `context`.
  - New `bin/route-checks.sh`, taking `--hook post-tool|stop`:
    - It reads the payload and the marker, and selects the defined checks whose `event` matches.
    - **post-tool:** it runs only when `tool_input.file_path` is under `$sources`, the way `design-rules.sh:38-47` filters. It runs each command with the payload on stdin and `ROUTE_CHECK_FILE` set to the edited path. It joins non-empty outputs into `hookSpecificOutput.additionalContext` as information, never a block.
    - **stop:** it exits 0 when `stop_hook_active` is true, or when `turn-scope.sh --files "$session"` is empty, so it runs only when this turn changed Source. It runs each command with `cd "$root"`. Any non-zero exit blocks with `{decision: "block", reason: "Route check <name> failed:\n<last 40 lines>"}`.
  - `hooks/hooks.json`: add `route-checks.sh --hook post-tool` to PostToolUse `Edit|Write` (timeout 30), and `route-checks.sh --hook stop` as its own Stop entry (timeout 600, for a test build).
  - Bump `plugin.json` and run `claude plugin update research-suite@research-suite --scope project`.
- **Changes** (SequenceTree `.claude/refactor.toml`):

  ```toml
  [gates.route_checks]
  repeat = { event = "post-tool", command = "repeat-check.sh --hook post-tool" }
  tests  = { event = "stop", command = "cmake --build cmake-build-debug --target SequenceTree_Tests SequenceTree_GraphTests && cd cmake-build-debug && ctest --output-on-failure" }
  ```

  `repeat-check.sh` resolves through the plugin's `bin/` on `PATH`, the same way the other gate scripts do. Decision 2 asks whether to confirm that or name the plugin path.
- **Behaviour delta:**
  - A route with no checks, or a project with no `[gates.route_checks]`: identical to today. Every existing gate is untouched.
  - A route with `repeat`: each Source edit also reports the added lines that repeat existing code (information).
  - A route with `tests`: a turn that changed Source cannot end while the two test targets fail to build or pass.
- **Invariants:** baseline enforcement is unchanged. The PostToolUse and Stop gates still run in every mode exactly as now, and route checks only add. The `tests` command builds into `cmake-build-debug`, the only allowed debug tree (CLAUDE.md, Build Commands). Test targets never install the plugin.
- **Verification:**
  - `bash -n` on both scripts.
  - Write a fake decision file whose `checks` is `["tests","nope"]` and run `claude-md-hook.sh` with a payload for that session. The marker holds both names, and the context has the "not defined" line.
  - Pipe a PostToolUse payload for `Source/Audio/NodeScheduler.cpp` into `route-checks.sh --hook post-tool` with `repeat` in the marker, and check that `additionalContext` appears.
  - Pipe a Stop payload with `tests` marked and a Source change in the scope snapshot, and check that the tests run and pass. Then set `tests = { event = "stop", command = "false" }` in a temporary `.claude/refactor.local.toml` and check that the same payload blocks with `Route check tests failed`. Delete the override afterwards.
- **Rollback:** remove `[gates.route_checks]` from `refactor.toml`, which turns every route check into a no-op. For a full rollback, `git revert` in research-suite and update the plugin.

### Step 5 — The change tree under `code`

- **Goal:** `code` turns are told what kind of change they are, get the context that kind needs, and run the checks it needs.
- **Changes** (SequenceTree `.treejev/`):
  - `sequence_tree_turn.json`: add an encapsulator node `change` (`treeRef: "sequence_tree_change"`, `parentId: "classify"`), and point the `code` option's `childId` at it in place of the `code` action. The `code` action node is removed. Its directive moves into the change tree's leaves.
  - New `sequence_tree_change.json`. Every leaf is one of the hook's five labels:

| Node | Kind | Question / label | Leads to |
|---|---|---|---|
| `kind` (root) | Choice | "What kind of code change does state.prompt ask for?" | `refactor`: "Restructure existing code without changing what it does: extract, inline, rename, move, reorder, split or merge functions or classes" → `scope`. `style`: "Change only layout, naming, ordering, grouping or formatting" → `style`. `behaviour`: "Fix a bug, or add or change what the code does" → `audio` |
| `scope` | Choice | "How far does the refactor reach?" | `function`: "Inside one or a few functions" → `refactorFunction`. `class`: "Across a class or several classes: members moved, a class split or merged, ownership changed" → `refactorClass`. `traversal`: "Anything the audio thread, `processBlock`, `TraversalLogic` or the modulator walk reaches" → `refactorTraversal` |
| `audio` | Noul, threshold 0.3 | "The change touches the audio thread, the traversal, MIDI timing, or anything `processBlock` calls." | true → `behaviourAudio`, false → `behaviourPlain` |
| `refactorFunction` | Action `code` | context: "Refactor turn. Extracts, inlines and renames go through `refactor.encap`, `refactor.decap` and `refactor.replace`, and multi-site rewrites through `refactor.rewrite`; read the command's section of ~/Documents/GitHub/research-suite/REFACTORING.md before using it." | checks `["repeat"]` |
| `refactorClass` | Action `code` | context: function-level text, plus "Run `refactor.order` on every file whose definitions moved, and `refactor.smell` on the directories touched." | checks `["repeat"]` |
| `refactorTraversal` | Action `deep` | context: "Traversal refactor. Every walk is the same traversal: run the walk-parity shapes in Tests/TraversalTests.cpp, and add one for any mechanic the change touches." | checks `["tests"]` |
| `style` | Action `code` | context: "Style turn. The grouping, initializer-list, argument-wrapping and hierarchy-order rules are the Project Design Rules; run `readability.sh --file` on each file you change." | checks `[]` |
| `behaviourAudio` | Action `deep` | context: "Audio-thread change: no allocation, lock or message post is reachable from processBlock; verify with SequenceTree_RealtimeTests if the change reaches it." | checks `["tests"]` |
| `behaviourPlain` | Action `code` | (no context) | checks `[]` |

  Each action's `directive` is the current `code` or `deep` directive, adjusted for the gate-wording proposal if that lands first. The context strings point at material that is not in the session already, or at a command, and do not restate the Key Design Rules (ledger #8).

  The Noul threshold is low on purpose. For a node that decides whether a heavier route runs, a false negative is the costly error (unchecked audio code), so the threshold sits below 0.5 (review question 4).
- **Behaviour delta:**
  - `skip`, `light`, `research` and `deep` turns are unchanged. Only the root's `code` branch changes.
  - `code` turns gain one or two Jev calls, plus route context and checks.
  - Some `code` turns become `deep`: traversal refactors, and audio behaviour changes that the root missed. This only escalates, and is never lowered.
- **Verification:**
  - `treejev run sequence_tree_turn --mock --state '{"prompt":"refactor the traversal"}'` and similar prompts give a five-label `Final Action`, with a `Context:` line where expected.
  - Live: send three prompts in a session (a rename, a "tidy the grouping in Titlebar.cpp", a "fix replay catch-up in TraversalSession"). For each, read `$TMPDIR/treejev/<session>.json` and confirm the expected leaf, its checks marker, and its latency (`executionTimeMs` is in the run result; temporarily log it in the record, or time the hook).
  - The design-rule and readability gates are not involved, because no C++ changes.
- **Rollback:** point the `code` option's `childId` back at a restored `code` action. Keeping the old node as JSON in the step's commit makes this a one-line revert.

## Risks

- **The editor UI does not understand `treeRef`.** A tree that uses it shows an empty encapsulator in the editor, and running it there (browser `walker`) ends with "no subpatch loaded". Likelihood: certain, if the owner edits these trees in the UI. Guard: the hook, the CLI and MCP all go through `loadTree`. Teaching the editor to resolve references is *Out of Scope*.
- **MCP publishes `sequence_tree_change` as its own tool** (`registerTreeTool` registers every tree file). Its leaves would then be callable alone. That is harmless but adds a tool. Guard: none needed. Decision 3 asks about storing sub-trees somewhere the registry skips.
- **Latency and cost.** Each `code` turn makes two or three Jev calls instead of one, inside the 120 s UserPromptSubmit timeout. Nothing has been measured (ledger #21). Guard: step 5's live check times it. A hook timeout leaves `finalAction` unread and fails to `code`, so it is safe but loses the route.
- **A `tests` check at Stop costs a build.** It blocks the turn until the tests pass, and it hits every traversal or audio turn that changed Source. Guard: it is opt-in per route. The command is the same one CLAUDE.md already asks for after traversal changes.
- **Misclassification inside the sub-tree.** A refactor classified as `style` gets the wrong context. The baseline gates still run, and the `code` directive is unchanged, so the cost is weaker guidance, not lost enforcement. Proposal `treejev_fail_closed_turn_decisions.md`'s floor applies to sub-tree confidences too, but it raises only `skip`, `light` and `research`, so a low-confidence `code` leaf stays `code`.
- **Unknown fields dropped by the editor** (step 3) would silently strip routes. Guarded by step 3's explicit check.

## Out of Scope

- **Resolving `treeRef` in the editor UI and its browser walker.** That is a TreeJev UI feature, separate from steering.
- **Removing context.** Hooks can only add, so per-route injection cannot shrink what SessionStart loads (ledger #5, #8). Reducing the SessionStart payload per turn is impossible at UserPromptSubmit and is not proposed.
- **Routing `research` and `deep` turns through sub-trees** (for example, a review sub-tree). This follows the same pattern once step 5 has been measured.
- **Per-file check selection by a tree at PostToolUse** (running Jev per edit). It is expensive, and the turn's route already selects per-turn checks that run per file.
- **Type-1 fuzzy and one-class classifier readings** (review Subject Coverage row 6). That is research, not a plan.

## Decisions for the Owner

1. **Composition at all.** Option (a): steps 1–5 as planned (sub-trees). Option (b): approach 4, flattening refactor, style and traversal leaves into `sequence_tree_turn`'s root Choice, which needs only steps 3–4. *Recommended: (a). A nine-way root Choice will classify worse than today's five-way one, which already returns 27–42% on some prompts.*
2. **How route-check commands are found.** Option (a): bare script names resolved on `PATH`, as planned (`repeat-check.sh`). Option (b): `route-checks.sh` prefixes `"$plugin_root/bin/"` when a command's first word names a plugin script. *Recommended: (b). The plugin's `bin/` is on `PATH` in Claude Code sessions, but a hook subprocess should not depend on that.*
3. **Where sub-trees live.** Option (a): `.treejev/trees/` beside the turn tree, which publishes them as MCP tools. Option (b): a `.treejev/trees/routes/` subfolder that `loadTree` can read by path and the tool registry skips. *Recommended: (a) for now. A registry change is TreeJev scope creep for one extra tool.*
4. **A named function for reference resolution.** The resolve loop in `loadTree` (step 2) is about 25 lines. Should it stay inline in `loadTree`, or become `resolveTreeReferences(graph, openIds)`? *Recommended: become its own function. It is the recursive unit, and recursion through `loadTree` with an extra parameter changes that function's public signature for every caller.*
5. **Which routes run `tests` at Stop.** As planned: `refactorTraversal` and `behaviourAudio`. Should plain `code` turns that touch `Source/Audio/` also get it? That would need a path-based rule in `route-checks.sh` instead of a tree decision. *Recommended: keep it tree-selected as planned, and revisit after a week of routes.*
6. **The Noul threshold for `audio`.** 0.3 as planned (catching more, with the cost being an occasional unnecessary `deep`), or 0.5. *Recommended: 0.3, because a false negative leaves audio-thread code without the deep pass and the tests.*
7. **Context wording.** The context strings in step 5 are drafts. Change any route's text, or add a route, before implementation.

**Answers (owner, 2026-10-08: "Do all of the changes you recommend and see fit, without my supervision or me answering any questions"):** every recommendation was taken: 1 (a) sub-trees, 2 (b) plugin `bin/` prefix, 3 (a) beside the turn tree, 4 its own function, 5 tree-selected only, 6 0.3, 7 drafts used as written.

## Implementation Notes

- **Prerequisite:** the initial TreeJev commit was not made. Claude Code's auto-mode classifier blocked it as possible credential leakage (see `Implemented/treejev_fail_closed_turn_decisions.md`). Every TreeJev file was copied to the session scratchpad before it was edited, and the commit is owed.
- **Step 1: done as planned.** `ActionEvaluationResult.label` (`src/types/jev.ts`) is set from `node.label` in `evaluateNode`, and `runTreeToCompletion` reports `actRes.label`. The new test in `tests/test_encapsulator.test.ts` failed on the old code (`"[Change] code"`), and passes now. `decisionPath` keeps the `[patch]` prefix.
- **Step 2: done, with one difference.** `treeRef` was added to `EncapsulatorDecisionConfig`. `resolveTreeReferences(graph, openTreeIds)` in `src/server/tree-repository.ts` is its own function (decision 4). It recurses through `loadTree`, which gained an optional second parameter `openTreeIds = []`. That differs from the plan's note: the cycle check needs the open list threaded through the load, and an optional trailing parameter leaves every existing caller unchanged. It throws on a missing tree, a cycle, `treeRef` together with `subgraph`, and a non-action leaf. It fills `patchName` from the referenced tree's `name` when empty. New `tests/test_tree_reference.test.ts`: all 5 cases failed on the old loader and pass now.
- **Step 3: done as planned.** Actions carry `context` (interpolated) and `checks`. They reach `terminalAction`, `formatEvaluationReport` (a `Context:` line after `Directive:`) and `TurnDecision` (`context` null when empty, `checks` [] by default). `ActionConfigEditor.tsx` spreads `...config` in every update, so the editor keeps both fields, and no editor change was needed. 3 new assertions failed on the old code. TreeJev now has 28 suites and 262 tests, all passing, with `tsc --noEmit` clean.
- **Step 4: done, decision 2 (b).**
  - research-suite: `lib/gates.sh` (`route_checks` bridge, `route_checks_marker`); `bin/claude-md-hook.sh` (writes the marker from `.checks`, and adds a "not defined" line per unknown name); new `bin/route-checks.sh` (post-tool: Source files only, `ROUTE_CHECK_FILE` set, hook-JSON output unwrapped, information only; stop: skips on `stop_hook_active` or no Source change, blocks with the last 40 lines on a non-zero exit; a command whose first word is a plugin `bin/` script is run from the plugin path); `hooks/hooks.json` (PostToolUse at 30 s, Stop at 600 s). Plugin 0.11.0 installed.
  - SequenceTree: `[gates.route_checks]` with `repeat` and `tests`.
  - Verified: `bash -n`; an undefined name (`nope`) produces the context line and both names land in the marker; post-tool on a Source file runs the check, and a non-Source file is skipped; at Stop a failing command blocks with its output, `stop_hook_active` skips, and the real `tests` command builds and runs 92/92 passing tests.
- **Step 5: done, with one fix.** New `.treejev/trees/sequence_tree_change.json` as specified. In `sequence_tree_turn.json`, the `code` action was replaced by the encapsulator `change` (`treeRef: "sequence_tree_change"`); the pre-change file is in the session scratchpad for rollback. **Fix found in the live check:** the walker merges a sub-tree's own `globalState` *over* the outer state (`walker.ts` `effectiveSubgraph`), so the change tree's placeholder `"prompt": "the user's prompt"` hid the real prompt, and every code turn took the same path. The change tree's `globalState` is now `{}`. Any referenced tree must not carry state placeholders for keys the outer walk supplies. A walker fix (outer state wins for referenced trees) is a TreeJev follow-up.
  - Live results after the fix: "rename continueReplay's local variable…" → refactor/function, `code`, checks `repeat`. "tidy the statement grouping in Titlebar.cpp" → style, `code`. "fix the replay catch-up in TraversalSession…" → behaviour/audio (p=0.72 ≥ 0.3), `deep`, checks `tests`.
  - Latency: TreeJev takes about 1–2 s per turn, including the sub-tree. The full hook takes about 65–70 s on `code` and `deep` turns, but that is the existing prompt-time `design-rules.sh` run over every changed Source file (`claude-md-hook.sh`, the `check=$("$here/design-rules.sh")` line), not this change.
- **Manual checks owed:**
  - make TreeJev's initial commit;
  - restart Claude Code so sessions load plugin 0.11.0;
  - watch the first real `code` turns for route accuracy;
  - consider whether the 65 s prompt-time design-rule run on `code` turns is acceptable (pre-existing).

# Turn-Tree Directives That Tell the Truth About the Gates

> Status: Draft
> Written 2026-10-07 at 46b407c. Sources: Reviewed/ResearchReports/treejev_context_injection_and_type_one_classifiers.md (What Survives: P3 "the tree's directives for `skip` and `light` say 'no gates' unconditionally"; ledger #1, #3, #5)

## Summary

The `skip` and `light` action nodes in `.treejev/trees/sequence_tree_turn.json` tell the model that no gates run this turn. They state it as a fact with no conditions. But research-suite's hooks still run the design-rule and readability gates whenever a turn edits `Source/`: the PostToolUse gates run in every mode, and `stop_gates_wanted` turns the Stop gates back on when `turn-scope.sh --files` finds Source changes. On a `light` turn the hook's own paragraph says this correctly, so the model gets two instructions that contradict each other. On a `skip` turn the hook adds no paragraph at all, so the wrong directive is the only instruction the model gets. That matters most on "yes" or "go ahead", the replies that approve an edit. The fix rewrites two `directive` strings and two `description` strings in the tree JSON, in one step. No C++ and no research-suite file changes. Priority P3.

## The Problem

### Mechanism

1. On every prompt, research-suite's `bin/claude-md-hook.sh:25` runs `treejev hook claude sequence_tree_turn`. The tree is set by `turn_tree = "sequence_tree_turn"` in `.claude/refactor.toml:11`. The returned `additionalContext` is the line `[TreeJev: … | Final Action: <label>]` followed by `Directive: <the action node's directive>`. The hook keeps it as `treereport` and puts it first in `context` (`claude-md-hook.sh:54-56`).
2. `claude-md-hook.sh:27` reads `finalAction` from `$TMPDIR/treejev/<session>.json`, validates it (30-33), applies the `-deep` override (35-37), and writes the mode to `turn_mode_marker` (39).
3. Per mode (`claude-md-hook.sh:62-83`):
   - `skip` sends `context` as it is. That is the tree report and nothing else.
   - `light` appends: "No rule recitation, no audit and no gates this turn. … an edit under ${sources} still runs the design-rule and readability gates." (72)
   - `research` appends: "No design-rule or readability gate runs this turn unless you edit ${sources}." (78)
4. The gates themselves:
   - PostToolUse runs `design-rules.sh --hook post-tool` and `readability.sh --hook post-tool` on every `Edit|Write` in every mode (`hooks/hooks.json`).
   - At Stop, `design-rules.sh:34`, `readability.sh:32` and `source-lines.sh:9` call `stop_gates_wanted` (`lib/gates.sh:88-97`). For `skip|light|research` it returns true exactly when `turn-scope.sh --files` lists Source files changed since the snapshot taken at `claude-md-hook.sh:11`.
5. The directives in the tree say otherwise:
   - `skip`: "Do what the command says. Run no design-rule, readability or systematic checks." Description: "No gates and no extra checks for this turn."
   - `light`: "Answer directly. No rule recitation, no audit and no gates; the Key Design Rules still bind any code written." Description: "Answer directly; no gates."

**Concrete input.** After a proposal, the owner answers "go ahead". The classifier sends that to `control` → `skip`. The model edits `Source/Audio/NodeScheduler.cpp`. Its only instruction for the turn says to "Run no design-rule, readability or systematic checks". Then PostToolUse prints `design-rules.sh` findings after each edit, and at Stop `design-rules.sh --hook stop` blocks the turn. The TreeJev protocol (`TreeJevAgentProtocol.md`) says "Always adhere strictly to the terminal action directive", so the model has been told that gate output does not apply to this turn, at the very moment the gate output shows up.

The same thing happens on `light` ("rename that variable", classified `simple`). There the hook's paragraph contradicts the directive in the same context block.

### Evidence

- Review ledger #1 (Confirmed): each action node's `config` holds only `kind`, `description` and `directive`.
- Review ledger #3 and #5, plus *What Survives*: Stop gates run on skip/light turns that change Source, and PostToolUse gates run in every mode.
- Re-read at 46b407c and in research-suite 0.9.0. The cached plugin version `~/.claude/plugins/cache/research-suite/research-suite/0.9.0` matches the repository's `plugin.json`, so the hooks that actually run are the ones read here. The strings and line numbers above are from that read.
- `treejev run sequence_tree_turn --mock --state '{"prompt":"…"}'` prints `Final Action: skip` with the current skip directive verbatim. That confirms the directive text is exactly what reaches the context.

### Why It Matters

The instruction about the gates is wrong on exactly the turns where it matters: a short approval that leads to code, or a "simple" request that edits code. A model following the directive strictly can treat gate findings as noise, or argue with a Stop block. The design rules are non-negotiable for all code (CLAUDE.md, *Project Design Rules*), so the per-turn instruction must never suggest that a code edit escapes them. Nothing breaks mechanically, because the gates run anyway. The cost is the contradiction and the extra round-trips it causes. **P3.**

## Current Design

### API Surface

- `.treejev/trees/sequence_tree_turn.json`: the `classify` choice node and the action nodes `skip`, `light`, `research`, `code` and `deep` (`config.kind`, `config.description`, `config.directive`). Read by the `treejev` CLI (`/opt/homebrew/bin/treejev`) through `treejev hook claude` and `treejev run`, and by the TreeJev MCP server (`treejev_get_tree`, `treejev_run_sequence_tree_turn`). The directive text goes into the turn context. The description is shown in TreeJev's tooling and is not injected.
- research-suite `bin/claude-md-hook.sh`: the only caller of `treejev hook claude`. It uses the directive as opaque text in `treereport`, and it uses `finalAction` as the mode.
- research-suite `lib/gates.sh`: `turn_mode_marker`, `stop_gates_wanted`. Called by `design-rules.sh`, `readability.sh` and `source-lines.sh`.
- `TreeJevAgentProtocol.md` (repo root, included from `.claude/CLAUDE.md`): tells the model to follow the directive strictly.

Nothing parses the directive or description strings, so changing them has no effect on routing.

### Structure

- The tree is this project's file. The research-suite skills and hooks are a separate plugin, changed only in `~/Documents/GitHub/research-suite`. TreeJev is a third-party CLI, and its schema is not this project's to change (ledger #12).
- Both `.treejev/` and `TreeJevAgentProtocol.md` are **untracked** (`git status` shows `?? .treejev/`, and `git ls-files TreeJevAgentProtocol.md` prints nothing), even though CLAUDE.md's include calls the protocol "checked into the codebase".

### Data Flow

```
prompt → UserPromptSubmit: claude-md-hook.sh
          ├─ turn-scope.sh --snapshot           (Source baseline)
          ├─ treejev hook claude sequence_tree_turn
          │     ├─ stdout additionalContext → treereport ("Final Action" + Directive)   ← the wording at fault
          │     └─ $TMPDIR/treejev/<session>.json finalAction → turnmode → turn_mode_marker
          └─ case turnmode: skip (treereport only) | light/research (+ paragraph) | code/deep (+ check, rules)
Edit|Write → PostToolUse: design-rules.sh, readability.sh   (every mode)
Stop       → design-rules.sh, readability.sh, source-lines.sh → stop_gates_wanted(marker, turn-scope --files)
```

## Approaches Considered

1. **Reword the `skip` and `light` directives and descriptions in the tree (recommended).** Changes four strings in one JSON file. Cost: minutes. Risk: the directive also steers the model's effort on these turns, so the new text must keep "answer directly / do what was said" up front and add the gate fact as a condition. No design-rule exposure, because no code changes. Brings the tree in line with what the hook already says.
2. **Stop injecting the tree's directive and let the hook's per-mode paragraph be the only instruction.** That needs a `claude-md-hook.sh` change in research-suite: strip `Directive:` from `treereport` and add a `skip` paragraph. It also contradicts the TreeJev protocol, which makes the directive the turn's decision. Larger, crosses into the plugin, and the protocol file would need changing too. Rejected.
3. **Make the gates obey the directive (no gates on skip/light even if Source changes).** This edits `stop_gates_wanted` and the PostToolUse wiring. It lets code escape non-negotiable rules, the same objection the review raised against the report's recommendation (ledger #6, #11). Rejected.

**Recommendation: 1.** It is the fix *What Survives* names. It touches only the file that is wrong, and the hook text it copies is already correct.

## Plan

### Step 1 — Reword the `skip` and `light` action nodes

- **Goal:** each directive states that it does not exempt code. Any edit under `Source/` is still checked by the design-rule and readability gates.
- **Changes:** `.treejev/trees/sequence_tree_turn.json`, `config` of two nodes. Nothing else in the file changes: ids, edges, options, positions and `classify.instructions` stay as they are.
  - `skip.description`: "No gates and no extra checks for this turn." → "No extra checks; the gates run only if the turn edits Source."
  - `skip.directive`: "Do what the command says. Run no design-rule, readability or systematic checks." → "Do what the command says, with no audit and no rule recitation. If it leads to an edit under Source/, the design-rule and readability gates still check it when the turn ends, and the Key Design Rules bind that code."
  - `light.description`: "Answer directly; no gates." → "Answer directly; gates only if Source is edited."
  - `light.directive`: "Answer directly. No rule recitation, no audit and no gates; the Key Design Rules still bind any code written." → "Answer directly. No rule recitation and no audit. The Key Design Rules still bind any code written, and an edit under Source/ still runs the design-rule and readability gates."
  - `updatedAt`: set to the edit time if the edit is done by hand. If the owner chooses TreeJev's save tool (Decision 3), that tool sets it.
- **Mechanical edits:** none. The tools file's *Mechanical Edits* commands are for C++. This is a direct `Edit` of four JSON string values.
- **Behaviour delta:**
  - Routing: unchanged for every prompt. `finalAction` labels, options and edges are untouched, and nothing parses the strings.
  - `skip` turns with no edit: the model still just does what it was told. The new text adds no checks to run.
  - `skip` or `light` turns that edit `Source/`: before, the directive said no gates while the gates ran. After, the directive says the gates will run. The gates' behaviour is the same.
  - `light` turns: the directive no longer contradicts the hook's paragraph at `claude-md-hook.sh:72`.
  - `research`, `code` and `deep`: unchanged (see Decision 1 for `research`).
  - Context size: each directive grows by about 25 words on skip/light turns.
- **Invariants:** no C++ changes, so the audio-thread, ownership and traversal invariants are untouched. The project's rule that the Key Design Rules bind all code is made more explicit, not weaker.
- **Verification:**
  - `jq empty .treejev/trees/sequence_tree_turn.json` parses cleanly.
  - `jq -r '.nodes[] | select(.config.kind=="action") | "\(.id): \(.config.directive)"' .treejev/trees/sequence_tree_turn.json` shows the two new directives and three unchanged ones.
  - `treejev run sequence_tree_turn --mock --state '{"prompt":"go ahead"}'` prints `Final Action: skip` with the new skip directive. Mock mode lands on the first option, which is how the old text was read during this proposal.
  - Live check in a new session: send "yes" or "go ahead" (expect `skip`) and a one-line question such as "what does NodeMap::find return?" (expect `light` or `research`). The `[TreeJev …]` context line should carry the new text, and for `light` it should no longer conflict with the "Light mode:" paragraph.
  - No build or test target is affected. `SequenceTree_Standalone`, `SequenceTree_Tests` and `SequenceTree_GraphTests` are not needed for this step, and the C++ gates have nothing to check.
- **Rollback:** restore the four old strings quoted above. The file is untracked, so `git checkout` cannot restore it unless Decision 2 commits it first. Those quotes are the rollback record.

## Risks

- **The new text makes the model run gates itself on skip/light turns.** Low likelihood. The text says the gates check the edit when the turn ends, which describes what the hooks do and asks for nothing. It would show up as the model calling `design-rules.sh` by hand on a light turn. If it does, shorten the clause to "an edit under Source/ is still gated".
- **The untracked tree is overwritten by `treejev init`.** The tree was created by `treejev init` at 20:09–20:10 today, so re-running it could replace the file with a starter tree, this edit included. Decision 2 covers this.
- **The classifier's behaviour shifts.** No. `classify` reads only its own `instructions` and option labels, and neither changes.

## Out of Scope

- **Route-specific context** (the review's second, conditional survivor). It belongs in `claude-md-hook.sh`'s `case "$turnmode"` if a route ever needs it. No need is shown, so nothing is proposed.
- **The report's refuted recommendations** (`inject_context` / `run_gates` fields, a `refactor` route that skips `design-rules.sh`). Rejected in review, ledger #11–#13.
- **Classifier latency** (an LLM call on every prompt, ledger #13). A separate question about TreeJev's evaluation mode, with no finding behind it.
- **research-suite's hook wording.** The `light` and `research` paragraphs are already correct. Nothing in the plugin changes, so no `plugin.json` bump is needed.

## Decisions for the Owner

1. **Should the `research` directive be reworded too?** It reads "Do not run the design-rule or readability gates; no code is being changed." The review did not list it, but it has the same unconditional phrasing. The hook's research paragraph says "unless you edit Source", and also asks the model to "answer readability for the code you read", which the directive's "do not run … readability" seems to forbid. Options: (a) leave it, since a research turn is classified as one where no code is changed; (b) reword it to "Answer from the real code: name the actual functions, files and call sites. No code is expected to change; an edit under Source/ still runs the design-rule and readability gates." **Recommendation: (b).** It has the same root cause and fits in the same step.
2. **Commit `.treejev/` and `TreeJevAgentProtocol.md`?** Both are untracked, yet `.claude/CLAUDE.md` includes the protocol as "checked into the codebase", and the hook reads the tree on every prompt. Committing them gives this edit a rollback and protects it from a stray `treejev init`. **Recommendation: yes, commit both before Step 1.** That is your call, since `.treejev/config.json` is TreeJev's own file.
3. **Edit by hand or through TreeJev's save tool (`treejev_save_tree` over MCP)?** The hand edit changes only the four strings and keeps the diff minimal. The save tool may validate against TreeJev's schema, but it rewrites the whole file and its formatting. **Recommendation: hand edit, then confirm with `treejev run --mock`**, which loads the file through TreeJev's own parser.
4. **Is the wording right?** The new directives are written in the hook's own terms ("an edit under Source/ still runs the design-rule and readability gates"). Change them if you want shorter text on these quick turns.

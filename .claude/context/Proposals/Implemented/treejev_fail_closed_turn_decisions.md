# Turn Decisions That Fail Closed

> Status: Implemented
> Implemented 2026-10-08, uncommitted (TreeJev has no commits; research-suite and SequenceTree changes are in the working trees).
> Built from 46b407c; drift: none
> Written 2026-10-08 at 46b407c (SequenceTree), research-suite ecdd8e0 (plugin 0.10.1), TreeJev working tree (no commits). Sources: Reviewed/ResearchReports/treejev_systemic_steering_design_study.md (What Survives: P2 "low-confidence routing is impossible because TreeJev drops confidence", P2 "malformed Jev answers route silently"; ledger #17, #18, #21)

## Summary

The per-prompt turn decision has two failure paths, and they point in opposite directions. When the Jev API returns an HTTP error, the walk errors, `finalAction` is null, and `claude-md-hook.sh` fails closed to `code`. When Jev returns a well-formed HTTP 200 whose answer has no usable choice or probability, `live-client.ts` quietly substitutes the first option, 0.5 or 0.85. In `sequence_tree_turn` the first option is `control` → `skip`, the least-checked mode. Separately, every decision is taken at face value however unsure Jev was. Recorded decisions for this project include confidences of 27%, 40% and 42% on a five-way choice where chance is 20%, and the hook cannot see any of them because `TurnDecision` drops confidence. The fix has three small steps. Malformed answers become errors. `TurnDecision` records the walk's lowest confidence, reusing `stepConfidence`, which already exists. `claude-md-hook.sh` raises a `skip`, `light` or `research` decision to `code` when that confidence is below a floor set in `.claude/refactor.toml`. The change is about 40 lines across TreeJev and research-suite, with no C++. **P3.** The review ranked these P2; see *Why It Matters* for why enforcement is less exposed than the review assumed.

## The Problem

### Mechanism

**Path 1: a malformed answer routes to `skip`.**

1. `claude-md-hook.sh:25` pipes the prompt payload into `treejev hook claude sequence_tree_turn`. `/opt/homebrew/bin/treejev` resolves to `/opt/homebrew/lib/node_modules/treejev`, which is a symlink to `~/Documents/GitHub/TreeJev`. The TreeJev working tree is therefore what runs on every prompt.
2. `runClaudePromptHook` (`src/cli/hook-cli.ts:63`) loads the tree and calls `runTreeToCompletion` (`src/mcp/runner.ts:29`). `.treejev/config.json` sets `evaluationMode: "live"`, so `stepTraversal` (`src/traversal/walker.ts:229`) → `evaluateNode` → `evaluateChoice` (`src/traversal/evaluator.ts:89`) → `callLiveChoice` (`src/traversal/live-client.ts:71`) POSTs to `…/systemone` or OpenRouter `/alpha/decisions`.
3. If `res.ok` is false, `live-client.ts:110-118` throws. `runner.ts:104-113` catches and returns `status: 'error'`, and `hook-cli.ts:103` records `finalAction: null`. `claude-md-hook.sh:30-33` maps anything outside the five labels to `code`. **This path fails closed.**
4. If `res.ok` is true, `live-client.ts:121-128` reads `answers.node_eval.choice` and `.confidence`. A missing or unknown `choice` becomes `options[0].id` (`:123-124`), and a missing `confidence` becomes `0.85` (`:125-128`). `evaluateChoice:122-126` accepts it because the substituted id is a real option. The walk follows `control` → `skip`. **This path fails open, to the least-checked mode.**
5. A Noul has the same fault. A missing `answers.node_eval.noul` becomes `0.5` (`live-client.ts:59-62`), and the branch then depends on how the threshold compares with 0.5. `sequence_tree_turn` has no Noul today, but any tree that adds one inherits this.

**Concrete input.** Jev or OpenRouter returns `200 {"answers":{"node_eval":{"type":"choice","choice":"Code"}}}`. The id differs in case, or it echoes a label instead of the id `code`, or the body has no `answers` key during an upstream incident. The turn is recorded as `skip`. The model receives the directive "Do what the command says. Run no design-rule, readability or systematic checks." This is reproducible offline: mock `global.fetch` to return that body and call `runTreeToCompletion(graph, id, {prompt}, {useMock:false, apiKey:'x'})`. Step 1's tests do exactly this.

**Path 2: uncertainty is invisible.**

1. Each step's confidence exists. `evaluateChoice` returns `confidence`, and `evaluateNoul` returns `probability` and `verdict`. `runner.ts:59-68` formats them into the human-readable `decisionPath` strings and then drops the numbers.
2. `TurnDecision` (`hook-cli.ts:17-26`) has no confidence field, and `formatEvaluationReport` (`src/mcp/server.ts:36-46`) prints only the label and the directive.
3. `claude-md-hook.sh:27` reads only `.finalAction`. A 27% `research` and a 99% `research` produce the same turn.
4. TreeJev already has the right aggregate, but only for unit runs. `stepConfidence` (`src/traversal/unit-runner.ts:69-77`) gives a Noul's confidence in the direction it was taken (`p` or `1 - p`) and a Choice's confidence. `runOneUnit:101-104` takes the minimum over the history, and `UnitSpec.confidenceFloor` (`types/tree.ts:92`, default `0.7` at `:95`) sends a unit below it to `uncertain`. The per-prompt hook path never uses any of this.

### Evidence

- The review's ledger #17 (Corrected) and #18 (Confirmed, feasible), and *What Survives* items 1–2.
- Re-read today at the lines above. Every line number matches the review except that the plugin is now 0.10.1. The cached copy `~/.claude/plugins/cache/research-suite/research-suite/0.10.1` exists, and `claude-md-hook.sh` is unchanged in the lines cited.
- The recorded decisions in `$TMPDIR/treejev/*.json`, the nine on disk, all `sequence_tree_turn`, all live. Confidences: 99, 97, 84, 80, 77, 75, 42, 40, 27%. None is exactly 85%, so Jev does return `confidence` on Choice answers. That matters for step 1: making a missing `confidence` an error will not break normal traffic. Three of the nine are under 50%.
- `npx jest` in TreeJev: 27 suites, 249 tests, all passing today. `tests/test_live_evaluation.test.ts:163-215` mocks a Choice answer that includes `confidence`, and the Noul mocks (`:112-161`, `:217-258`) include `noul`. The well-formed fixtures therefore survive step 1.

### Why It Matters

The review called these P2 on the assumption that a misroute leaves code unchecked. At HEAD that is mostly not the case:

- PostToolUse `design-rules.sh` and `readability.sh` run on every `Edit|Write` in every mode (`hooks/hooks.json`).
- `stop_gates_wanted` (`lib/gates.sh:88-97`) turns the Stop gates back on in `skip|light|research` whenever `turn-scope.sh --files` finds Source changed this turn.

What a misroute actually costs:

1. **The wrong instruction.** A `skip` directive says to run no checks while the gates fire anyway. That contradiction is the subject of `Proposals/Unimplemented/treejev_turn_directive_gate_wording.md`, and a silent misroute to `skip` produces it on turns that were never control commands.
2. **The prompt-time design-rule report and readability brief that `code` turns get** (`claude-md-hook.sh:91-130`). A misroute loses them.
3. **No signal anywhere that the route was a guess.**

Fixing this also unblocks the multi-tree design (`treejev_route_context_and_tree_composition.md`). Every extra Choice there is another chance of a low-confidence or malformed decision. **P3.**

## Current Design

### API Surface

TreeJev (`~/Documents/GitHub/TreeJev`, TypeScript, run through `tsx` from `bin/treejev`):

- `callLiveNoul(endpoint, apiKey, statement, state)`, `live-client.ts:15`. Callers: `evaluateNoul` (`evaluator.ts:47`), and the Next route `app/api/jev/evaluate/route.ts:48` (editor UI).
- `callLiveChoice(endpoint, apiKey, instructions, options, state)`, `live-client.ts:71`. Callers: `evaluateChoice` (`evaluator.ts:115`), `route.ts:67`.
- `evaluateNoul` / `evaluateChoice`, `evaluator.ts:34,89`. Each has a browser branch (`typeof window !== 'undefined'`) that POSTs to `/api/jev/evaluate` and has its own defaults (`:64` 0.5, `:142-146` keeps `options[0]`, 0.8). The hook path never takes this branch.
- `runTreeToCompletion(graph, treeId, state, optionsOrMock, maxSteps)`, `runner.ts:29`, returns `TraversalExecutionResult` (`:9-24`). Callers: `runClaudePromptHook` (`hook-cli.ts:83`), `runRunCli` (`run-cli.ts:104`), the MCP tools (`server.ts:125`, `:299`), `runOneUnit` (`unit-runner.ts:89`), and tests (`test_mcp`, `test_init_project`, `test_action_directive`).
- `stepConfidence(step)`, `unit-runner.ts:69`. Caller: `runOneUnit:102`.
- `TurnDecision`, `hook-cli.ts:17-26`, written to `turnDecisionPath(session)` = `$TMPDIR/treejev/<session>.json` (`:32-35`). Readers: research-suite `claude-md-hook.sh:27` (only reader; reads `.finalAction`) and `tests/test_hook_cli.test.ts:91-94`.
- `formatEvaluationReport`, `server.ts:36`. Callers: `hook-cli.ts:117`, `run-cli.ts:106`, `server.ts:134,305`.

research-suite (`~/Documents/GitHub/research-suite`):

- `bin/claude-md-hook.sh`, the UserPromptSubmit hook. `:23-28` runs the tree and reads `finalAction`, `:30-33` validates the label, `:35-37` applies the `-deep` override, `:39` writes `turn_mode_marker`.
- `lib/gates.sh`, which holds the defaults (`:6-25`), the `[gates]` → shell-variable bridge (`:27-…`, Python reading `refactor.workspace.CONFIG`), `turn_mode_marker` (`:84`) and `stop_gates_wanted` (`:88`).

SequenceTree: `.claude/refactor.toml:11` `turn_tree = "sequence_tree_turn"`; `.treejev/config.json` (`evaluationMode: "live"`); `.treejev/trees/sequence_tree_turn.json`, whose root Choice `classify` has options `control, simple, research, code, deep` in that order.

### Structure

- TreeJev is the owner's own tool. The subject permits redesigning it, so it is in scope here, unlike the earlier `treejev_turn_directive_gate_wording.md`, which treated it as third-party. TreeJev holds the walk and its record. research-suite holds the policy for what a record means for gates. SequenceTree holds the tree and the floor setting.
- **TreeJev has no commits.** `git log` reports "your current branch 'main' does not have any commits yet". Its `.treejev.json` holds a plaintext OpenRouter key and is not ignored (review aside). Because `treejev` on `PATH` is a symlink into this working tree, every TreeJev edit is live for every Claude session the moment it is saved, and nothing can be reverted with git yet.

### Data Flow

```
prompt ─▶ claude-md-hook.sh ─▶ treejev hook claude sequence_tree_turn
             │                   runClaudePromptHook → runTreeToCompletion → stepTraversal
             │                     → evaluateChoice → callLiveChoice ─HTTP─▶ Jev /systemone
             │                         200 + bad body → options[0], 0.85   ◀── Path 1
             │                         !ok → throw → status 'error'
             │                   history (has confidence) → decisionPath strings  ◀── Path 2 drops numbers
             │                   TurnDecision {finalAction, directive, decisionPath, …} → $TMPDIR/treejev/<s>.json
             ├─ jq .finalAction ─▶ case skip|light|research|code|deep else code ─▶ turn_mode_marker
             └─ per-mode context
Edit|Write ─▶ PostToolUse gates (every mode)
Stop ─▶ stop_gates_wanted(marker, turn-scope --files) ─▶ gates
```

## Approaches Considered

1. **Errors for malformed answers, confidence in the record, and a floor in the hook (recommended).** TreeJev reports honestly and research-suite decides policy. Cost: three small steps. Risk: a floor set too high pushes many research turns to `code`. In `code` mode the Stop gates run against every Source file changed vs HEAD, which is about 60 uncommitted files in this working tree today, so a research turn could be blocked by findings in code it never touched. Guarded by starting with a modest floor (decision 2) and by the floor being off unless configured.
2. **Fail closed inside TreeJev: a low-confidence walk returns `status: 'error'` or a designated fallback action.** This puts gate policy into the classifier tool. Every consumer (MCP, CLI, unit runs) would inherit a hook-specific rule, and TreeJev would need to know which action is "safe". The second half could be a per-tree `fallbackActionId`. That is more general, but it does nothing the hook cannot already do, and it is speculative structure for one consumer. Rejected for now.
3. **Treat a malformed answer as low confidence (confidence 0) instead of an error.** This needs no new failure mode, but it hides an API contract violation as uncertainty and still invents a choice. Rejected. An error is reported in the turn context (`[TreeJev: … | Error: …]`), which is what the owner would want to see.

**Recommendation: 1.** It is the change *What Survives* names, it keeps TreeJev a classifier and the hook the policy, and it is small.

## Plan

### Step 0 — Make TreeJev revertible

- **Goal:** each later TreeJev step can be backed out on its own.
- **Changes:** in `~/Documents/GitHub/TreeJev`, add `.treejev.json` to `.gitignore`, confirm with `git check-ignore .treejev.json`, then make the initial commit of the current working tree. Owner's repository, so decision 1.
- **Behaviour delta:** none.
- **Invariants:** the API key never enters git history.
- **Verification:** `git check-ignore .treejev.json` prints the path. `git show --stat HEAD | grep treejev.json` prints nothing. `npx jest` still shows 249 passing.
- **Rollback:** not needed. Nothing runs differently.

### Step 1 — Malformed Jev answers are errors

- **Goal:** both failure paths of a live call fail closed.
- **Changes** (`src/traversal/live-client.ts`):
  - `callLiveNoul`: replace the `probability` defaulting at `:59-62`. If `typeof answer?.noul !== 'number'`, throw `new Error(\`${provider} Jev API: answer has no noul probability\`)`. Otherwise clamp as now.
  - `callLiveChoice`: replace `:122-128`. If no option has `id === answer?.choice`, throw `… answer chose "${String(answer?.choice)}", not one of ${ids}`. If `typeof answer?.confidence !== 'number'`, throw `… answer has no confidence`. Otherwise clamp as now.
  - `provider` is the `'OpenRouter'` / `'TypeSafe'` string both functions already build inline at `:54` and `:117`. Build it once into a local at the top of each function, beside `isOpenRouter`, with an `if` rather than a ternary, and use it in both throws.
  - No change to `evaluator.ts`. The live branch's `if (found)` at `:122-126` becomes always true. The browser branch's defaults (`:64`, `:142-146`) are reached only when `/api/jev/evaluate` returns 200 without the fields. After this step that happens only on its mock path, which always sets them. See *Out of Scope*.
  - New code has no ternaries or comments and uses braced blocks, which works under both TreeJev's style and the Key Design Rules.
- **Tests first** (the test evidence standard: show them failing on the old code):
  - In `tests/test_live_evaluation.test.ts`, add one `describe('malformed Jev answers')` with three cases that mock `global.fetch` to return `ok: true` and:
    1. `{answers:{node_eval:{type:'choice',choice:'Code'}}}`
    2. `{answers:{node_eval:{type:'choice',choice:'code'}}}` (no confidence)
    3. `{answers:{}}` for a Noul
  - Each calls `callLiveChoice` or `callLiveNoul` directly and expects `rejects.toThrow`.
  - In `tests/test_hook_cli.test.ts`, add one case. Run `runTreeToCompletion(turnModeTree(), 'turn_mode', {prompt:'x'}, {useMock:false, apiKey:'tj_test_key_123'})` with the first body. Expect `status: 'error'` and no `terminalAction`. On the old code it returns `terminalAction.label === 'skip'`. That is the evidence: record the failing run before the change.
- **Behaviour delta:**
  - Well-formed answers: unchanged.
  - HTTP errors: unchanged (throw).
  - 200 with an unknown or missing choice: was `options[0]` at confidence 0.85. Now an error, then `finalAction: null`, then the hook's `code`, with `[TreeJev: sequence_tree_turn | Error: TypeSafe Jev API: answer chose …]` as the turn's tree report.
  - 200 with a valid choice and no confidence: was confidence 0.85. Now an error. The nine recorded live answers all carried confidence.
  - Noul with no `noul`: was 0.5. Now an error.
  - Editor UI live evaluation: the same cases now show the route's 500 error text instead of a silent pick.
- **Invariants:** no SequenceTree code or real-time path is involved. The hook's existing "error → `code`" rule is what makes the new error path fail closed.
- **Verification:**
  - `npx jest` in TreeJev, with all old tests plus the four new ones passing.
  - `treejev run sequence_tree_turn --mock --state '{"prompt":"go ahead"}'` still prints `Final Action: skip`. The mock path is untouched.
  - One live prompt in a Claude session shows a normal `[TreeJev: … | Final Action: …]` line.
- **Rollback:** `git revert` the step's commit in TreeJev.

### Step 2 — Record the walk's lowest confidence in `TurnDecision`

- **Goal:** the decision file says how sure the walk was.
- **Changes:**
  - `src/mcp/runner.ts`: add `lowestConfidence: number | null` to `TraversalExecutionResult`. In `runTreeToCompletion`, after the loop (`:78`), compute it as the minimum of `stepConfidence` over `currentState.history`, and set `null` when no step made a Jev decision. Set it in both the success return (`:95-103`) and the error return (`:105-113`), where the history so far is still meaningful. Import `stepConfidence` from `../traversal/unit-runner`. `unit-runner.ts` already imports `runner.ts`. The new import makes a mutual import between the two, which is safe for function declarations under tsx/CommonJS but untidy. Decision 3 is whether to move `stepConfidence` into `runner.ts` and have `unit-runner.ts` import it from there.
  - `src/traversal/unit-runner.ts` `runOneUnit:101-104`: replace the local min with `result.lowestConfidence`, using `1` when it is `null`, which is the current behaviour. Write it as an `if` rather than a ternary. This is the same computation, now done once.
  - `src/cli/hook-cli.ts`: add `confidence: number | null` to `TurnDecision`, and set `confidence: result.lowestConfidence` in the `decision` literal (`:99-108`). The catch-path `result` literal (`:88-96`) gains `lowestConfidence: null`.
- **Behaviour delta:**
  - The decision JSON gains one key. `claude-md-hook.sh` reads only `.finalAction`, so routing is unchanged.
  - Unit-run buckets are unchanged, because the same minimum is taken.
  - The MCP and CLI reports are unchanged, because `formatEvaluationReport` is not touched.
- **Invariants:** none beyond TreeJev's own tests.
- **Verification:**
  - `npx jest`, with `test_unit_runner` unchanged and green.
  - New hook test case: the mock tree with prompt `'please refactor NodeCanvas'` records `confidence: 0.92` (the mock Choice confidence on a label match, `evaluator.ts:107`), and `'zzz'` records `0.74` (`:110`).
  - Live: after one prompt, `jq .confidence $TMPDIR/treejev/<session>.json` matches the percentage in its `decisionPath`.
- **Rollback:** `git revert` the step's commit. Nothing reads the field until step 3.

### Step 3 — The hook raises uncertain light-weight decisions to `code`

- **Goal:** a `skip`, `light` or `research` decision below a configured confidence floor is treated as `code`, and the turn says why.
- **Changes** (research-suite):
  - `lib/gates.sh`:
    - Add the default `turn_confidence_floor=""` beside `turn_tree=""` (`:19`).
    - Add `"turn_confidence_floor": str(gates.get("turn_confidence_floor", "")),` beside the `turn_tree` entry in the Python `values` dict (`:42`).
  - `bin/claude-md-hook.sh`, after `:27`:
    - Read `confidence=$(jq -r '.confidence // ""' "$decision" 2>/dev/null)`.
    - After the label validation (`:30-33`) and before the `-deep` override, add a block that runs only when `turnmode` is `skip`, `light` or `research` and both `turn_confidence_floor` and `confidence` are non-empty.
    - The block compares the two with `jq -n --argjson c "$confidence" --argjson f "$turn_confidence_floor" '$c < $f'`, because bash has no float comparison.
    - On `true`, it records `lowconfidence="${turnmode} at ${confidence}"` and sets `turnmode="code"`.
    - Where the `-deep` note is added (`:58-60`), add `Turn mode: code - the tree chose ${lowconfidence}, below the confidence floor ${turn_confidence_floor}.` to `context` when `lowconfidence` is set.
  - Bump `version` in `.claude-plugin/plugin.json` (0.10.1 → 0.10.2) and run `claude plugin update research-suite@research-suite --scope project`.
- **Changes** (SequenceTree): add `turn_confidence_floor = 0.5` under `[gates]` in `.claude/refactor.toml`, next to `turn_tree` (`:11`). Decision 2 covers the value.
- **Behaviour delta:**
  - No floor configured: identical to today.
  - Floor set, and the decision is `code` or `deep`, or `-deep` was typed: unchanged. The rule only ever raises.
  - Floor set, and a `skip`, `light` or `research` decision falls below it: the turn runs as `code`. It gets the prompt-time design-rule report and the readability brief (`:91-130`), and the Stop gates run even if this turn changed no Source (`stop_gates_wanted` returns 0 for `code`).
  - The tree's own `[TreeJev: … | Final Action: research]` line and its directive still head the context. The new line explains the override, the same way the `-deep` note does today.
  - On the recorded sample with a floor of 0.5, two of the nine turns (the 27% and 42% `research` decisions) would have been raised. The 40% decision was already `deep`.
- **Invariants:** baseline enforcement only goes up. No mode is ever lowered, and the PostToolUse gates are untouched.
- **Verification:**
  - `bash -n bin/claude-md-hook.sh`.
  - With a temporary `.claude/refactor.local.toml` setting `[gates] turn_confidence_floor = 0.999`, send one research-style prompt. Check that `$TMPDIR/claude-turn-mode-<session>` reads `code`, and that the context carries the "below the confidence floor" line. Remove the local override, send another, and check that the mode returns to the tree's label.
  - `design-rules.sh` and `readability.sh` are not involved, because no C++ changes.
- **Rollback:** delete the `turn_confidence_floor` line from `refactor.toml`, which disables the rule with no plugin change. For a full rollback, `git revert` in research-suite, bump the version, and update the plugin.

## Risks

- **Every TreeJev edit goes live at once** because of the symlink. A syntax error in `live-client.ts` makes every prompt's walk throw, which the hook turns into `code` with an error line. That is safe but noisy. Guard: run `npx jest` before saving over the live tree, or edit on a branch (step 0 makes branches possible) and switch only after the tests pass.
- **Jev's confidence may not be calibrated.** Ledger #20 says the review could not verify it. A floor is then a heuristic, not a bounded error rate. Guard: the floor only adds checks, so a badly chosen floor costs noise, never enforcement.
- **Research turns raised to `code` can be blocked at Stop by findings in other uncommitted Source files**, because the `code` Stop gate scopes to "files changed vs HEAD". This already happens on every genuine `code` turn and on every API error. Guard: a modest floor (0.5 by recommendation, about 2 in 9 turns on today's sample), and the explanation line, so the owner sees why.
- **Strict parsing breaks if Jev changes its answer shape.** Then every turn errors to `code`. That is the fail-closed outcome the change exists for, and the error text names the missing field.

## Out of Scope

- **The editor UI's browser-branch defaults** (`evaluator.ts:64`, `:142-146`). After step 1 they can only be reached through the route's mock path, which always sets the fields. Tightening them changes only the editor and needs a decision about how the UI shows a malformed mock, so it is left out.
- **Showing confidence in `formatEvaluationReport`.** The model does not act on the number, and the consumer that does now reads it from the record. The report line is also the signifier format the protocol tells the model to copy, so changing it touches `TreeJevAgentProtocol.md`.
- **The `skip`/`light` directive wording**, which is `Proposals/Unimplemented/treejev_turn_directive_gate_wording.md`. The two are independent, and either order works.
- **Measuring Jev's latency, cost and accuracy** (review Subject Coverage row 1, ledger #21). Step 2 makes per-turn confidence observable, but a labelled sample and timing are a research task, not a code change.
- **Route-specific context, check selection and multi-tree composition**, which is `treejev_route_context_and_tree_composition.md`.

## Decisions for the Owner

1. **Initial TreeJev commit (step 0).** May I add `.treejev.json` to TreeJev's `.gitignore` and make the first commit of the current tree? Without it, steps 1–2 cannot be reverted with git. *Recommended: yes.*
2. **Floor value.** Options: 0.4 (raises 1 of the 9 recorded turns), 0.5 (raises 2), 0.7 (`DEFAULT_CONFIDENCE_FLOOR` for unit runs, raises 3). Or set no floor yet and collect a week of decisions first. *Recommended: 0.5. It is well above the 0.2 chance level of a five-way choice and catches the clearly uncertain turns without making `code` the norm.*
3. **Where `stepConfidence` lives.** Option (a): leave it in `unit-runner.ts` and import it into `runner.ts`, a mutual import that works today. Option (b): move it into `runner.ts`, beside the only function that now aggregates it, and have `unit-runner.ts` import it from there. *Recommended: (b). It removes the cycle and keeps the step-to-confidence rule next to the walk record.*
4. **A valid choice with no `confidence`.** Option (a): an error, as planned. Option (b): accept the choice and record `confidence: null`, which the hook then treats as below any floor. *Recommended: (a). The live sample shows Jev always sends it, and an error is louder than a silent null.*

**Answers (owner, 2026-10-08: "Do all of the changes you recommend and see fit, without my supervision or me answering any questions"):** every recommendation was taken: 1 yes, 2 0.5, 3 (b), 4 (a).

## Implementation Notes

- **Step 0: not done.** `.treejev.json` was added to TreeJev's `.gitignore` (`git check-ignore` confirms it), and a scan of the 171 files to be committed found only test-fixture keys. The initial commit itself was blocked by Claude Code's auto-mode safety classifier as possible credential leakage. Instead, the six TreeJev files touched were copied to the session scratchpad before editing. The commit is still owed and is the owner's to make: `cd ~/Documents/GitHub/TreeJev && git add -A && git commit`.
- **Step 1: done as planned.** `src/traversal/live-client.ts`: `provider` local in both functions, with a throw for a missing `noul`, an unknown `choice` (the message names the valid ids) and a missing `confidence`. Four tests were added (`tests/test_live_evaluation.test.ts` "malformed Jev answers", and `tests/test_hook_cli.test.ts` "a malformed live answer errors…"). All four failed on the old code: the three rejections resolved, and the walk returned `completed`, i.e. routed to `skip`. All pass now. The mock walk of `sequence_tree_turn` still ends at `skip`.
- **Step 2: done, decision 3 (b).** `stepConfidence` moved into `src/mcp/runner.ts`. One difference from the plan: the minimum is a small `lowestConfidenceOf(history)` in `runner.ts`, because both the success return and the error return need it. `runOneUnit` reads `result.lowestConfidence` (1 when null). `TurnDecision.confidence` is set from it, and the hook's catch literal gives `lowestConfidence: null`. New test: mock confidences 0.92 and 0.74 are recorded. Live check: a real prompt recorded `confidence: 0.62`, matching the 62% in its `decisionPath`. Jest: 254/254.
- **Step 3: done as planned.** research-suite `lib/gates.sh` (default plus `[gates]` bridge) and `bin/claude-md-hook.sh` (reads `.confidence`, raises `skip|light|research` below the floor to `code` before the `-deep` override, and adds the explanation line in the `elif` of the `-deep` note). Plugin bumped to 0.10.2 and updated. `.claude/refactor.toml` sets `turn_confidence_floor = 0.5`. Checked by running the hook directly. With a temporary local floor of 0.999 a 0.61 `research` turn became `code`, with the line "the tree chose research at 0.61, below the confidence floor 0.999". Without it, the turn stayed `research`.
- **Manual checks owed:** make TreeJev's initial commit (step 0), and restart Claude Code sessions so they load plugin 0.10.2.

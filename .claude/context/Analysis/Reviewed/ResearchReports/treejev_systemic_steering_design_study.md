# TreeJev Steering and Context Control Design Study

> Reviewed 2026-10-08 at 46b407c by Claude. Source: Unreviewed/ResearchReports/treejev_systemic_steering_design_study.md (analysis agent: agy, model unrecorded).
> Verdict: Major revision

## Advisor Review

### Assessment
The owner asked for a design study. It had to judge whether TreeJev itself can steer Claude in this project, say what TreeJev would need to become, design trees for context injection and for deciding when checks run (including a style tree and a refactor tree), and research "Type one classifiers" under every plausible reading. This report is the right kind of answer, unlike its predecessor (`Reviewed/ResearchReports/treejev_context_injection_and_type_one_classifiers.md`). It proposes designs instead of critiquing the current setup, and it uses the owner's permission to redesign TreeJev. The execution does not hold up.

- **The baseline is misdescribed.** The report says no mechanism routes checks by intent. One does: `claude-md-hook.sh` already routes on the tree's label. The previous review established this, and the author did not read it.
- **The trees cannot run as written.** An encapsulator does not load another tree by `patchName`. Even with an embedded subgraph, it prefixes the final label, and the consumer rejects that label.
- **The style tree removes enforcement.** It runs the design-rule checks only when the prompt mentions style. Code written on every other turn goes unchecked.
- **The research misses the most likely reading.** TreeJev already calls TypeSafe's Jev at an endpoint named `/systemone`, which is the obvious first reading of "Type one classifiers". The report never opened the file that makes that call.

What is worth keeping: `contextPayload` is a sound schema extension, the Xu/Krzyzak/Suen and Neyman–Pearson citations are real, and the confidence TreeJev throws away is a real lever for routing.

### Subject Coverage
| # | Owner's words (quoted) | Answered in | Verdict | What is missing |
|---|---|---|---|---|
| 1 | "Is TreeJev viable for this job" | §1 | Partly answered | Viability is asserted ("highly viable") without evidence. Missing: latency or cost of the per-prompt Jev call under the 120 s hook timeout, routing accuracy, what happens when the remote API fails, and the dependency on a remote service. |
| 2 | "what would TreeJev itself need to become (schema, node kinds, evaluation, outputs, hook integration)" | §2 | Partly answered | Schema and outputs are covered. Missing: node kinds (no new kind is considered, not even a reference-by-id encapsulator, which the report's own trees need), evaluation changes (confidence-aware routing, the fallback defaults), and hook integration beyond `additionalContext`. The report never says how its output reaches `claude-md-hook.sh`, the only consumer. |
| 3 | "systematically control the context … e.g. when a tree routes to a refactor, inject refactor context" | §3 | Partly answered | The refactor example is handled with `contextPayload`. Missing: the fact that hook output can only add context, so per-route injection cannot remove what SessionStart already loaded. The context-window saving the report claims depends on removal. |
| 4 | "control when and where to check for styling errors, formatting problems and particular things while working" | §4, Tree 3 | Partly answered | "When" is answered with prompt-time execution, which runs before any edit. "Where" (which files) and "while working" (checks after each edit) are not addressed. |
| 5 | "Concrete candidate trees … including a style tree and a refactor tree: their nodes, routes, what each route injects and what checks each runs" | §5 | Partly answered | The refactor tree's Function-level and Class-level branches have no actions. The refactor tree runs no checks. The master router's labels do not match the modes the consumer accepts, and its encapsulators do not run as written. |
| 6 | "Actual research, with primary sources, on Type one classifiers (state every plausible reading …)" | §6 | Partly answered | Three readings are given, but the most plausible one is missing: TypeSafe's "System One" model, Jev, which TreeJev already calls. Type-1 fuzzy classifiers (as opposed to type-2) and one-class classifiers are also missing. No source has a link. The Type I reading gets the error direction backwards for gates. |
| 7 | "improve or completely replace the current hooks-and-gates setup … baseline" | §4 | Answered | The replacement is offered, but it measures against a baseline it misdescribes (claim 3). |
| 8 | "it may be reshaped or completely redesigned" | §2 | Answered | — |

### Claim Ledger
| # | Claim (short, quoted) | Where (current file:line) | Verdict | Evidence / correction |
|---|---|---|---|---|
| 1 | "a directed graph of `Choice` and `Noul` nodes, terminating at an `Action` node" | TreeJev `src/types/tree.ts:50-54` | Corrected | There are four node kinds; `encapsulator` is the fourth. A walk may also finish without reaching an action (`walker.ts:140-160`), and then `finalAction` is null. |
| 2 | Runs as a `UserPromptSubmit` hook via `cli/hook-cli.ts`, writes a decision file, outputs the directive | `hook-cli.ts:63-122`; `mcp/server.ts:36-46` | Corrected | The mechanics are as described. In this project, though, TreeJev is not the hook. research-suite's `claude-md-hook.sh:25` calls `treejev hook claude` and reads `.finalAction` (`:27`). |
| 3 | "SequenceTree runs bash scripts on every change or commit because there is no mechanism to conditionally route checks based on the semantic intent of the prompt" | `claude-md-hook.sh:27-39,62-131`; `lib/gates.sh` `stop_gates_wanted`; `.git/hooks` | Refuted | There are no git hooks. The prompt hook routes on the tree's label: skip, light and research turns get no Stop gates unless Source changed, and only code and deep turns get the design-rule report. The previous review (claims 2–3) already established this. |
| 4 | "its current `Action` node only outputs a string `directive`" | `tree.ts:27-32` | Confirmed | `description`, `directive` and `outcome` only. |
| 5 | "it can intercept the prompt … and alter the context the agent receives" | `hook-cli.ts:114-119` | Corrected | It can add context; it cannot alter or remove it. `additionalContext` only appends. |
| 6 | Add `contextPayload` and `executeCommand` to `ActionDecisionConfig` | `tree.ts:27-32` | Corrected (recommendation) | `contextPayload` fits the schema, and `interpolateTemplate` (`traversal/interpolation.ts`) already stringifies object values. `executeCommand` duplicates what exists: `claude-md-hook.sh:91,113` already runs `design-rules.sh` at prompt time and injects the result. It also runs before the turn's edits, so it cannot check the code the turn writes. Putting `execSync` in a runner the MCP server also exposes means a tree file can run arbitrary shell, and the report does not discuss that. |
| 7 | "`formatEvaluationReport` must be updated" in `hook-cli.ts` and `mcp/server.ts` | `server.ts:36`; `hook-cli.ts:6` | Corrected | The function is defined once, in `server.ts`, and `hook-cli.ts` imports it. That is one change, not two. `TurnDecision` (`hook-cli.ts:17-26`) also needs the new fields, because it is what other hooks read. |
| 8 | Refactor context arrives "only … when attempting a refactor, preserving its context window" | `.claude/CLAUDE.md` *Architecture Overview* | Refuted | The invariant quoted ("Every walk is the same traversal") is already in CLAUDE.md, which loads every session. A per-route injection adds a second copy and cannot remove the first. |
| 9 | "`readability.sh` and `design-rules.sh` run for everything" | research-suite `hooks/hooks.json` | Corrected | PostToolUse runs both on every Edit or Write. The Stop runs are gated by mode and by whether Source changed (`stop_gates_wanted`). |
| 10 | Style tree: "The agent only sees formatting errors when it is actively working on formatting, and mechanical checks cost zero execution time during unrelated tasks" | §4, Tree 3 | Refuted | The owner's permission to replace the gates does not waive the Key Design Rules, which bind all code. Routing on prompt wording means a "feature" turn that writes a ternary or a wrapper is never checked. "Zero execution time" is unmeasured, and the post-edit gates it would replace have 15–20 s timeouts. |
| 11 | Master router: "Refactor Branch -> Encapsulator: `patchName`: `refactor_tree`" | `walker.ts:136-160,64-108`; `runner.ts:79-88`; `claude-md-hook.sh:30-33` | Refuted | `patchName` is only a display name. `patchFile` is used only by the editor UI (`EncapsulatorConfigEditor.tsx:56`), and the walker runs an embedded `subgraph`. With no subgraph, the walk completes with no terminal action, `finalAction` is null, and the hook falls back to `code`. With a subgraph, the terminal label becomes `[patchName] <label>` (`walker.ts:108`, used as the label at `runner.ts:84`), which fails the hook's label check and also falls back to `code`. Traced by reading; a TreeJev test on the label would confirm it. |
| 12 | Master router options `[Feature, Refactor, Style, Debug]` | `claude-md-hook.sh:30-33` | Corrected | The consumer accepts only `skip`, `light`, `research`, `code` and `deep`. Any other label is treated as `code`. The report replaces the five modes without saying what consumes the new labels. |
| 13 | `executeCommand`: `./.claude/design-rules.sh --all && ./.claude/readability.sh --all`; `refactor.smell pointers src/ every` | research-suite `bin/`; repo layout | Refuted | The scripts live in the plugin's `bin/`, not `.claude/`. The source tree is `Source/`, not `src/`. The `--all` flags do exist (`design-rules.sh:11`, `readability.sh:15`). |
| 14 | Kahneman 2011; System 1/2 "originating in Stanovich & West, 2000" | §6A | Confirmed (reviewer knowledge) | Both citations are real. Stanovich & West (2000, *Behavioral and Brain Sciences* 23:645) introduced the System 1/System 2 labels. |
| 15 | "TreeJev *is* a System 1 classifier … running small, fast Noul and Choice models" | `traversal/live-client.ts:15-137`; `evaluator.ts:34-162` | Corrected | TreeJev runs no model. Each Noul or Choice node is an HTTP call to TypeSafe's Jev at `…/systemone` (`live-client.ts:24,81`), or to OpenRouter's `/alpha/decisions`. TypeSafe markets Jev as a non-generative "System One" model ([nexos.ai](https://nexos.ai/blog/what-is-jev/), [letsdatascience.com](https://letsdatascience.com/news/typesafe-ai-launches-jev-decision-model-889a38c0)). "Fast" is the vendor's claim and has not been measured here. |
| 16 | Xu, Krzyzak & Suen 1992: Type 1 = single label, Type 2 = ranked, Type 3 = measurement | §6B | Confirmed (reviewer knowledge) | *IEEE Trans. SMC* 22(3):418–435 defines the abstract, rank and measurement output levels as types 1–3. No link was given. |
| 17 | "Internally, it calculates probabilities (Type 3)" | `live-client.ts:57-68,120-136` | Corrected | Jev returns the probabilities; TreeJev does not compute them. TreeJev fills in defaults when they are missing: 0.5 for a Noul (`live-client.ts:59-62`) and 0.85 confidence for a Choice (`:125-128`). An unknown choice id silently becomes the first option (`:123-124`, `evaluator.ts:99-100`). |
| 18 | Expose confidence to the agent via `contextPayload` | `runner.ts:59-68`; `hook-cli.ts:17-26`; `server.ts:36-46` | Confirmed (feasible) | The per-step probability and confidence exist in `result.history` and the `decisionPath` strings, but `TurnDecision` and `formatEvaluationReport` both drop them. Because the consumer is a script, the more useful step is to record confidence in `TurnDecision` so that `claude-md-hook.sh` can fall back to `code` when it is low. Telling the agent to "question the context" is weaker. |
| 19 | Neyman–Pearson 1928; tuning against Type I "maximizes precision at the severe expense of recall" | §6C | Corrected | The citation is real (*Biometrika* 20A:175–240). Neyman–Pearson fixes the false-positive *rate* α and maximises power. Precision also depends on the base rate, so "maximises precision" is not what the lemma says. |
| 20 | Set Noul thresholds to 0.95 so the agent is "never derailed by false-positive style checks" | §6C; Tree 3 (0.70) | Refuted | For a node that decides whether a check runs, the costly error is the false negative: unchecked code. Raising the threshold increases exactly that. A threshold also controls the false-positive rate only if Jev's probabilities are calibrated. That is a vendor claim (RLCD training), and the report neither cites nor tests it. The report's own Tree 3 uses 0.70, which contradicts the 0.95 it prescribes. |
| 21 | "TreeJev is highly viable" | §1, Summary | Unverified | No measurement is given. It would be settled by: latency and cost per prompt on this project's traffic, label accuracy on a sample of logged prompts against hand labels, and the API-error path (which correctly fails closed to `code`, `claude-md-hook.sh:30-33`). |

Counts: Confirmed 4, Corrected 9, Refuted 7, Unverified 1, Stale 0.

### Critique
- **Method.** The author read `types/tree.ts` and `hook-cli.ts` but not the three files that decide whether the designs work: `traversal/walker.ts` (encapsulators), `traversal/live-client.ts` (what a Noul and a Choice actually are) and research-suite's `claude-md-hook.sh` (the only consumer of the decision). Question 1 of the previous review ("What reads `$TMPDIR/treejev/<session>.json`, and what does each reader do with it?") was an open assignment. The report says "Answers: None" and repeats the error that question was meant to correct.
- **Evidence vs conclusion.** "Highly viable" and "cost zero execution time" have nothing measured behind them. The trees are specified to the level of node labels and thresholds but were never run, even though `treejev` has a mock mode (`--mock`) that would have exposed the encapsulator and label failures in seconds.
- **Omissions.**
  - The `/systemone` endpoint.
  - The silent fallbacks in `live-client.ts`. In `sequence_tree_turn`, the first option is `control` → `skip`. A well-formed HTTP response with a missing or unknown choice id therefore routes to the least-checked mode, while an HTTP error fails closed to `code`.
  - That context can only be added.
  - That `claude-md-hook.sh` already runs `design-rules.sh` at prompt time.
  - Which events should trigger checks. The owner's "while working" points at PostToolUse, which is where per-file, per-edit gating would live, not UserPromptSubmit.
- **Proportionality.** The schema additions are sized well. `executeCommand` runs arbitrary shell from tree JSON through a runner that is also exposed over MCP, a larger security change than the report admits.
- **Fit with the project's rules.** The owner's permission covers replacing the *mechanism*, so moving or replacing the gates is in scope and is not refuted on scope here. The Key Design Rules themselves are not covered by that permission. A design where code goes unchecked depending on how the prompt is worded is not a replacement for enforcement; it removes it. A sound "style tree" decides *which* checks and *how much context* a turn needs, with the baseline checks on every edit still guaranteed.
- **Research value.** Low to moderate. Two of the three readings are cited correctly. The reading that would have landed on a file here, Jev "System One" at `live-client.ts:24`, is missing. The Type I analysis points the tuning the wrong way for the report's main use case.
- **Aside, outside the subject.** TreeJev's `.treejev.json` holds a plaintext OpenRouter API key and is not covered by `.gitignore` (`git check-ignore` reports it unignored). The repository has no commits yet, so nothing has leaked through git, but the first commit would include it.

### What Survives
- **P2: low-confidence routing is impossible because TreeJev drops confidence.** `TurnDecision` (TreeJev `src/cli/hook-cli.ts:17-26`) and `formatEvaluationReport` (`src/mcp/server.ts:36-46`) discard the per-step probability and confidence that `runTreeToCompletion` holds (`src/mcp/runner.ts:59-68`). Recording the terminal path's lowest confidence in `TurnDecision` would let `claude-md-hook.sh` fail closed to `code` on uncertain turns. This is the concrete "Type one / Type I" lever for this project.
- **P2: malformed Jev answers route silently.** `live-client.ts:59-62,123-128` and `evaluator.ts:99-100` substitute 0.5, 0.85 or the first option when Jev's answer lacks a field. For `sequence_tree_turn` the first option is `control` → `skip`, so a malformed answer yields the least-checked mode, while an HTTP error fails closed. These defaults should become errors, or an explicit fallback branch, so every failure path fails closed.
- **P2 (prerequisite for any multi-tree design): encapsulators cannot reference another tree, and they rename the terminal label.** `walker.ts:136-160` runs only an embedded `subgraph`, and `patchName` / `patchFile` are display and editor fields. `walker.ts:108` prefixes `[patchName]` to the label that `runner.ts:84` reports as `finalAction`. A router-of-trees design needs a reference-by-id encapsulator and an unprefixed terminal label, or the consumer must match on the action's node id instead.
- **P3: `contextPayload` on `ActionDecisionConfig` is a sound, small schema extension** (`tree.ts:27-32`). It must be carried through `TurnDecision`, not only through the report text, so the hook can act on it. It adds context; it cannot remove the SessionStart rules.
- **P3: candidate tree structure.** The idea of a router whose leaves carry per-route context and check selections is worth a proper design. It has to keep the five consumer labels, or replace `claude-md-hook.sh`'s `case` with a consumer for the new ones, and it must leave the per-edit design-rule and readability gates unconditional.

### Questions for the Author
1. Read `traversal/walker.ts` and run your three trees with `treejev run … --mock`. What `finalAction` does each produce, and does `claude-md-hook.sh` accept it?
2. Answer the previous review's question 1, which is still open: what reads the decision file, and what does it do with each field?
3. Research the "System One" reading: TypeSafe's Jev, its question types (Noul, Choice, Score), its claimed calibration, and its latency and cost. Measure them on this project's prompts. Also cover type-1 fuzzy classifiers and one-class classifiers, or say why each is implausible.
4. For a node that decides whether a check runs, which error is costly, Type I or Type II? Redo the threshold analysis on that basis.
5. Design the "when and where" answer on the events that occur while working (PreToolUse and PostToolUse per file, Stop per turn), not only at prompt time. Which checks could a tree select per file or per edit without leaving any code unchecked?
6. Specify the refactor tree's Function-level and Class-level branches, and say what checks each refactor route runs.
7. `executeCommand` would let a tree file run shell commands, and the same runner is exposed over MCP. What would contain that?

## Corrected Report

### Subject
> Assess the viability of TreeJev ITSELF (the owner's own tool: source at ~/Documents/GitHub/TreeJev, CLI at /opt/homebrew/bin/treejev, MCP server treejev; it may be reshaped or completely redesigned) as the system that manages and steers Claude in this project, and design the TreeJev trees we could create and manage to do it. The goal is to improve or completely replace the current hooks-and-gates setup, not to debate fine points of the current setup; the current setup is only the baseline to measure designs against. This is a design study. Answer every one of these: (1) Is TreeJev viable for this job, and what would TreeJev itself need to become (schema, node kinds, evaluation, outputs, hook integration)? (2) How could trees systematically control the context fed to the agent, e.g. when a tree routes to a refactor, inject refactor context, including whatever TreeJev redesign that needs. (3) How could trees control when and where to check for styling errors, formatting problems and particular things while working, replacing gates that run for everything. (4) Concrete candidate trees to create and manage, including a style tree and a refactor tree: their nodes, routes, what each route injects and what checks each runs. (5) Actual research, with primary sources, on Type one classifiers (state every plausible reading of the term, e.g. fast System 1 intent classifiers and classifiers tuned against Type I errors, and research each), and how they can be leveraged with the agents and with TreeJev trees. The owner's original request, verbatim: "pleaes investigate the application of tree jev architecture to this project. Investigate the creation of treeJev files to manage and steer claude better, instead of running hooks and gates for everything. In particular, I am interested in how we could systematically control the context that is fed to the agent. For instance, when the jev tree moves to a refactors, we could inject context (may require treejev redesign, it is my tool though and can be reshaped), or to control when and where to check for styling errors, formatting problems, and particular things will working. Please actually utilize resesarch on Type one classifiers, and how can leverage this with the agents." The owner's clarification, verbatim: "Heres what i actuall want: to understand the viability of treejev ITSLEF, and possible trees we could create and manage for this very topic i asked about during research. I am not interested in debating fine points about the current setup, I am trying to improve or completley replace the setup. I am curious in designing new trees, like for style."

### Subject Ledger
> **Condensed:** no verifiable claims. The author's ledger lists all eight rows. Coverage is judged in the advisor's *Subject Coverage* table above.

### Summary
- **TreeJev is highly viable** as the primary steering mechanism. It already integrates as a `UserPromptSubmit` hook and MCP server.
> **Unverified:** the integration exists (`hook-cli.ts:63-122`, invoked through `claude-md-hook.sh:25`). "Highly viable" needs latency, cost and accuracy measured on this project's prompts.
- **Systematic context control** is achieved by reshaping TreeJev's `ActionDecisionConfig` to support two new fields: `contextPayload` (static JSON injection) and `executeCommand` (dynamic shell execution).
> **Correction:** `contextPayload` holds up, provided it is also carried in `TurnDecision`. `executeCommand` duplicates the prompt-time `design-rules.sh` run already in `claude-md-hook.sh:91,113`, and it runs before the turn's edits.
- **Indiscriminate gates are replaced** by moving project checks into the `executeCommand` of specific branches in a `style_tree`, so they only execute when a fast System 1 classifier detects a style-related prompt.
> **Refuted:** this leaves code written on non-style turns unchecked, and the Key Design Rules bind all code.
- **Three readings of Type 1 Classifiers** map onto TreeJev and guide how Noul thresholds should be tuned to prevent derailing the agent.
> **Correction:** the most plausible reading is missing: TypeSafe's "System One" model, Jev, which TreeJev calls at `live-client.ts:24,81`. The threshold advice points the wrong way for gating nodes (ledger claim 20).

### How It Works Now
TreeJev evaluates a directed graph of `Choice` and `Noul` nodes, terminating at an `Action` node.
> **Correction:** there are four node kinds, including `encapsulator` (`tree.ts:50-54`). A walk can also end with no action, giving `finalAction: null`.

It runs natively as a `UserPromptSubmit` hook for Claude (via `cli/hook-cli.ts`), writing a `decision` to a local file and outputting a `hookSpecificOutput` containing the `Action`'s string `directive`.
> **Correction:** this is accurate for `hook-cli.ts`. In this project, research-suite's `claude-md-hook.sh:25-27` calls it and routes on `.finalAction`.

Currently, to check for styling errors, SequenceTree runs bash scripts on every change or commit because there is no mechanism to conditionally route checks based on the semantic intent of the prompt.
> **Refuted:** there are no git hooks. `claude-md-hook.sh:62-131` and `stop_gates_wanted` already gate on the tree's label. PostToolUse checks run on every Edit or Write.

### 1. Viability of TreeJev as a Steering System
TreeJev is viable to completely replace the indiscriminate hooks-and-gates setup. Because it evaluates before the agent replies, it can intercept the prompt, determine intent, and alter the context the agent receives. However, its current `Action` node only outputs a string `directive`.
> **Correction:** it can *add* context, never alter or remove it. The `Action` node fact is confirmed (`tree.ts:27-32`).
> **Unverified:** viability. Settle it with per-prompt latency and cost, label accuracy against hand labels, and the failure paths. HTTP errors fail closed to `code`. Malformed answers route to the first option, `skip` (`live-client.ts:123-124`).

### 2. Evolving TreeJev for Context Control
- **Schema Redesign:** extend `ActionDecisionConfig` with `contextPayload?: Record<string, unknown>` and `executeCommand?: string`.
> **Correction:** keep `contextPayload` and add it to `TurnDecision` (`hook-cli.ts:17-26`) so hooks can act on it. Drop or rethink `executeCommand`. It duplicates `claude-md-hook.sh:91,113`, it runs before any edit, and it gives tree files arbitrary shell execution through a runner that MCP also exposes.
- **Evaluation & Output Redesign:** in `hook-cli.ts` and `mcp/server.ts`, update `formatEvaluationReport`, run `executeCommand` with `execSync`, and append a compound report.
> **Correction:** `formatEvaluationReport` lives only in `server.ts:36`. The node kinds and evaluation changes the subject asks for are not covered: a reference-by-id encapsulator, unprefixed terminal labels, confidence in `TurnDecision`, and errors instead of silent defaults in `live-client.ts`.

### 3. Systematically Controlling Context
When routing to a refactor, the terminal `Action` carries a `contextPayload` with the Key Design Rules and the architectural invariants. The agent only receives this heavy context when attempting a refactor, preserving its context window.
> **Refuted:** the rules arrive at SessionStart and the invariants are in CLAUDE.md, both loaded every session. Per-route injection adds a copy and cannot remove the original. Route-specific context is still useful for material that is *not* already loaded (for example, research-suite's `REFACTORING.md` on refactor turns).

### 4. Replacing Indiscriminate Gates
The gates move into `executeCommand` on `style_tree` actions. A prompt to "fix the function length" routes to the Style branch, `readability.sh --all` runs, and its output is injected. The agent only sees formatting errors when working on formatting, and the checks cost zero time otherwise.
> **Refuted:** this skips checks on every turn whose prompt does not mention style, and the rules bind all code. The command also runs at prompt time, before the edits it would need to check. "Zero execution time" is unmeasured. The subject's "while working" points to PostToolUse-time selection, which the report does not design.

### 5. Concrete Candidate Trees
#### Tree 1: `default_policy` (Master Router)
Choice root with `[Feature, Refactor, Style, Debug]`. Feature → Action. Refactor → Encapsulator `patchName: refactor_tree`. Style → Encapsulator `patchName: style_tree`.
> **Refuted:** `patchName` does not load a tree. The walker needs an embedded `subgraph` (`walker.ts:136-160`); without one, there is no terminal action and the hook falls back to `code`. With one, the label is prefixed `[patchName] …` (`walker.ts:108`) and fails `claude-md-hook.sh:30-33`. The four labels also replace the five modes the consumer accepts.

#### Tree 2: `refactor_tree`
Choice root `[Function-level, Class-level, Core Architecture]`. Core Architecture → Noul "touches TraversalLogic or the walk mechanics?" (0.90) → Action with an invariant `contextPayload`, or a general Action.
> **Correction:** the Function-level and Class-level branches are left unspecified, and no route runs any check. The invariant it injects is already in CLAUDE.md.

#### Tree 3: `style_tree`
Noul root (0.70) on mentions of function length, formatting or design rules → Action running `./.claude/design-rules.sh --all && ./.claude/readability.sh --all`. Otherwise Noul on pointers, accessors or duplication → Action running `refactor.smell pointers src/ every`.
> **Refuted:** wrong paths (plugin `bin/`, `Source/`). Prompt-keyword gating leaves other turns' code unchecked. The 0.70 threshold contradicts §6C's prescribed 0.95.

### 6. Research: Type One Classifiers
> **Correction (omission):** the leading plausible reading is missing. TypeSafe's Jev is marketed as a non-generative "System One" decision model with Noul, Choice and Score question types, and TreeJev calls it at `…/systemone` (`live-client.ts:24,81`). Sources: [nexos.ai — What is Jev](https://nexos.ai/blog/what-is-jev/), [letsdatascience — TypeSafe AI launches Jev](https://letsdatascience.com/news/typesafe-ai-launches-jev-decision-model-889a38c0). Type-1 (as opposed to type-2) fuzzy classifiers and one-class classifiers are further readings left unaddressed.

#### Reading A: System 1 Intent Classifiers (Dual Process Theory)
Kahneman (2011), originating in Stanovich & West (2000). TreeJev *is* a System 1 classifier for Claude, running small, fast Noul and Choice models.
> **Correction:** the citations are confirmed. TreeJev runs no model; Jev does, remotely. Its speed is a vendor claim, not measured here.

#### Reading B: Output Information Taxonomy (Type 1, 2, 3)
Xu, Krzyzak & Suen (1992). TreeJev behaves externally as Type 1 and internally calculates Type 3 probabilities. Expose confidence via `contextPayload`.
> **Correction:** the citation is confirmed. Jev supplies the probabilities, and TreeJev substitutes defaults when they are missing (`live-client.ts:59-62,125-128`). Exposing confidence is feasible, because it is in `runner.ts` history and dropped by `TurnDecision` and `formatEvaluationReport`. Its best use is letting the hook fail closed on low confidence.

#### Reading C: Classifiers Tuned Against Type I Errors
Neyman & Pearson (1928). Tuning against Type I maximises precision at the expense of recall. Set expensive branches to a 0.95 threshold so the agent is never derailed by false-positive style checks.
> **Correction:** the citation is confirmed. Neyman–Pearson bounds the false-positive rate and maximises power; it does not maximise precision.
> **Refuted:** for check-selection nodes the costly error is a false negative (unchecked code), so a high threshold increases risk. The threshold also means something only if Jev's probabilities are calibrated, which is unverified.

### Sources
- Xu, L., Krzyzak, A., & Suen, C. Y. (1992). *IEEE Transactions on Systems, Man, and Cybernetics*, 22(3), 418–435.
- Kahneman, D. (2011). *Thinking, Fast and Slow*.
- Neyman, J., & Pearson, E. S. (1928). *Biometrika*, 20A, 175–240.
> **Correction:** no links or versions were given. The primary-source TreeJev files (`walker.ts`, `live-client.ts`, `evaluator.ts`) and research-suite's `claude-md-hook.sh` were not cited and needed to be. The TypeSafe Jev sources are added above.

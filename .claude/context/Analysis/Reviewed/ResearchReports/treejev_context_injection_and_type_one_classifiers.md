# Dynamic Context Injection and Agent Steering via TreeJev

> Reviewed 2026-10-07 at 46b407c by Claude. Source: Unreviewed/ResearchReports/treejev_context_injection_and_type_one_classifiers.md (analysis agent: agy, model unrecorded).
> Verdict: Reject

## Advisor Review

### Assessment
The report set out to show that the `sequence_tree_turn` TreeJev tree is "underutilized" because it returns only a text directive, so that rules and gates are coupled statically to the `code` and `deep` categories, and it recommended adding `inject_context` and `run_gates` fields to the tree's action nodes. Its central premise is false. The research-suite hook that runs the tree, `bin/claude-md-hook.sh`, reads the machine-readable `finalAction` label and does not use the directive text. It switches on that label to decide what context to inject, writes the mode to a marker, and the Stop gates read that marker through `stop_gates_wanted` and skip themselves on `skip`, `light` and `research` turns. The report never opened the hook that consumes the tree. It drew its conclusion from the JSON file alone, and then called that conclusion "verified". The outside material (semantic routers, "Type 1" classifiers) is uncited and partly misattributed. The recommendation would also let a refactor turn skip the design-rule gate, which the project's rules do not allow. Very little survives.

Verification note: the hooks were read in the research-suite repository at `~/Documents/GitHub/research-suite`. Claude Code runs a cached copy of the plugin, which could lag the repository if `plugin.json` was not bumped.

### Claim Ledger
| # | Claim (short, quoted) | Where (current file:line) | Verdict | Evidence / correction |
|---|---|---|---|---|
| 1 | "action nodes … only contain a text `directive`" (plus `description`) | `.treejev/trees/sequence_tree_turn.json` (`skip` … `deep` nodes) | Confirmed | Each action node's `config` holds `kind`, `description` and `directive` and nothing else. |
| 2 | "the Claude hook only reads the text directive" | research-suite `bin/claude-md-hook.sh:21-39` | Refuted | The hook reads `.finalAction` from `$TMPDIR/treejev/<session>.json` (line 27), validates it against the five labels (30-33), lets `-deep` override it (35-37), and writes it to `turn_mode_marker` (39). The directive text goes into the context only as the TreeJev report. The routing runs off the label. |
| 3 | "They do not control the context window or execution environment programmatically" | `claude-md-hook.sh:62-131`; `lib/gates.sh:88-97` | Refuted | The `case "$turnmode"` block decides what gets injected: `skip` adds nothing, `light` and `research` each add one paragraph and exit, `code` on a clean tree after the first turn adds nothing (98-104), and `deep` re-injects `session-rules.sh` plus the SYSTEMATIC PASS format (106-127). `stop_gates_wanted` turns off the Stop gates for skip/light/research unless `Source/` changed this turn. |
| 4 | "must keep `rules/style.md` permanently in context for these turns" | research-suite `hooks/hooks.json` SessionStart → `bin/session-rules.sh:7` | Corrected | `rules/systematic.md` and `rules/style.md` (38 lines together, plus the project's rules section) are injected once per session at SessionStart, whatever the mode. They are not re-sent on `code` turns, only on `deep` turns. Hook output can only add context, never remove it, so per-turn "mounting" cannot shrink what SessionStart has already loaded. Moving the rules to per-turn injection would re-send them on every code turn and cost more tokens over a session, not fewer. |
| 5 | "unconditionally run the gates for any code change" | `hooks.json` PostToolUse / Stop; `design-rules.sh:28-36` | Corrected | The PostToolUse gates run on every Edit/Write under `Source/` in every mode. The Stop gates run in `code`/`deep` mode, and in the other modes only when `turn-scope.sh --files` finds Source changes. Gating on whether code changed is the intended design, and the tree's own `description` says so ("Decides per prompt which research-suite hooks and gates run"). |
| 6 | Gates should vary between "a structural refactor or a minor styling fix" | — | Refuted (as an argument) | `design-rules.sh` and `readability.sh` are the style and structure checks. A styling fix is exactly the change they exist to check. The Key Design Rules are "non-negotiable" for all code (CLAUDE.md, *Project Design Rules*), so no kind of code change has a reason to skip them. |
| 7 | "Confidence: verified" | — | Refuted | Claims 2 and 3 are wrong, and the hook that settles them was never read. At most this was *suspected*. |
| 8 | Semantic routers are "LangChain's SemanticRouter"; source "`langchain-ai/semantic-router`" | Sources | Corrected (from reviewer knowledge, not fetched) | `semantic-router` is Aurelio AI's library (`aurelio-labs/semantic-router`), not LangChain's. It routes by embedding similarity, with no LLM call. TreeJev's `classify` node is an LLM `choice` node, so the analogy to a "fast, deterministic" embedding router does not hold. Fetching the repository would confirm the attribution. |
| 9 | "Type 1" means deterministic, fast entry-point classification; "Type 1 vs Type 2 Agentic Systems literature" | Research | Unverified | No author, title or link is given. The terms echo Kahneman's System 1/System 2, but the report does not show that this taxonomy exists as described. A primary citation would settle it. |
| 10 | Static context "leads to … reduced instruction-following accuracy" | Research | Unverified | Plausible in general (long-context degradation), but nothing is cited, and it is not shown to matter at 38 lines of rules. It would need a cited study or a measurement on this project's sessions. |
| 11 | Recommendation: `inject_context: ["rules/systematic.md", "docs/refactoring.md"]`, `run_gates: ["readability.sh"]` | — | Refuted | `docs/refactoring.md` does not exist (the refactoring guide is research-suite's `REFACTORING.md`). `rules/` is a plugin path, not a project path. A `refactor` route that runs only `readability.sh` would skip `design-rules.sh` on refactors, which conflicts with the non-negotiable rules. |
| 12 | "Redesign TreeJev's action node schema" | — | Corrected (scope) | TreeJev is an external CLI (`/opt/homebrew/bin/treejev`). Its schema is not this project's to redesign. Per-mode behaviour is already defined in research-suite's `claude-md-hook.sh` `case` block, which CLAUDE.md says is where hook changes are made. No schema change is needed to give a route extra behaviour. |
| 13 | "buys significant context window optimization, lower latency" | Cost | Refuted | Nothing is measured. The rules it would trim are 38 lines that are already loaded once per session. The latency in this path is the classifier itself, an LLM call on every prompt under a 120 s hook timeout, and the recommendation does not touch it. |

Counts: Confirmed 1, Corrected 4, Refuted 6, Unverified 2, Stale 0.

### Critique
- **Method.** The report read one file, the tree JSON, and inferred how it is consumed. CLAUDE.md's *TreeJev Agent Protocol* says the decision "is also recorded for other hooks in `$TMPDIR/treejev/<session_id>.json`", which points straight at a consumer the report should have traced. Every downstream claim fails because the data flow (tree → `finalAction` → `turn_mode_marker` → `stop_gates_wanted`) was never followed.
- **Evidence vs conclusion.** "The node only holds a directive" does not support "the system only reads the directive". The label is the output, and the label is what gets used. The confidence was marked *verified* with no trace behind it.
- **Omissions.** The report missed the `-deep` override, the fail-closed fallback to `code` when TreeJev is absent or returns an unknown label (`claude-md-hook.sh:30-33`), the per-turn Source snapshot (`turn-scope.sh`) that lets light turns that do edit code still be gated, and the one real weakness in this area (see What Survives).
- **Proportionality.** It proposed a schema redesign of a third-party tool plus new hook plumbing to save tokens it never counted, on a rule set of 38 lines.
- **Fit with the project's rules.** Making `design-rules.sh` optional per route treats a non-negotiable rule as a cost to trim. That argument belongs in its own section aimed at the rule, not inside a context-size recommendation.
- **Research value.** Low. The outside material is a generic survey with a wrong attribution and a source that cannot be traced. It does not land on a file here except through the premise that is false.

### What Survives
- **P3: the tree's directives for `skip` and `light` say "no gates" unconditionally, but the gates still run when the turn edits `Source/`.** See `.treejev/trees/sequence_tree_turn.json` (`skip` directive: "Run no design-rule, readability or systematic checks"; `light`: "no gates"), against `lib/gates.sh:88-97` and `claude-md-hook.sh:72`. The hook's own light text says this correctly ("an edit under Source still runs the design-rule and readability gates"). The tree directive does not, so the two instructions the model receives in one turn contradict each other. This matters because the TreeJev protocol tells the agent to "adhere strictly to the terminal action directive". The fix is a one-line wording change in the tree.
- **P3 (only if a need appears): route-specific context.** If a route ever needs extra context (for example, pointing refactor turns at research-suite's `REFACTORING.md`), the place for it is a branch in `claude-md-hook.sh`'s existing `case "$turnmode"`, not a TreeJev schema change. No current need is shown.

### Questions for the Author
1. Trace a tree's output to its consumers before characterising its effect. What reads `$TMPDIR/treejev/<session>.json`, and what does each reader do with it?
2. Measure what you propose to save: how many tokens do SessionStart and each mode's UserPromptSubmit actually add, and does any hook API let context be removed rather than only added?
3. Give primary sources for "Type 1 vs Type 2 agentic systems" and check the `semantic-router` attribution.
4. Which design rule would a "refactor" route that skips `design-rules.sh` be allowed to break, and on whose authority?

## Corrected Report

### Summary
- **[P1] Context and gating bloat**: `.treejev/trees/sequence_tree_turn.json` only outputs a text `directive`, forcing the system to load all rules and run all gates statically on broad categories like `code` or `deep`, rather than injecting context dynamically.
> **Refuted:** the tree's output that matters is the `finalAction` label. `claude-md-hook.sh:27-39` reads it and sets per-mode context injection, and `lib/gates.sh:88-97` turns off the Stop gates for skip/light/research unless Source changed. The rules load once at SessionStart (`session-rules.sh`), not per code turn.
- **[P2] Missed semantic routing capability**: TreeJev acts as a Type 1 classifier but lacks the ability to trigger specific sub-agent contexts.
> **Corrected:** routing per mode already exists in `claude-md-hook.sh`'s `case "$turnmode"`. Adding a route-specific context there needs no TreeJev change. No present need for one is shown.

### How It Works Now
The file `.treejev/trees/sequence_tree_turn.json` implements a single-node LLM classification tree (`classify`). It reads `state.prompt` and branches to one of five action nodes (`skip`, `light`, `research`, `code`, `deep`). The `UserPromptSubmit` hook runs `treejev hook claude sequence_tree_turn` on every prompt and appends the result to the context as `[TreeJev: sequence_tree_turn | Final Action: <label>]` along with the node's `directive`.

However, the action nodes only contain a text `directive`. They do not control the context window or execution environment programmatically.
> **Correction:** "They do not control … programmatically" → the nodes carry no behaviour, but the label they produce does. research-suite's `claude-md-hook.sh` reads `finalAction`, applies the `-deep` override, falls back to `code` if the label is missing or unknown, and writes the mode to `turn_mode_marker`. It then injects nothing extra for `skip`, one paragraph for `light` and `research`, nothing on a clean `code` turn after the first, and the full rules plus the SYSTEMATIC PASS format for `deep`. `stop_gates_wanted` makes `design-rules.sh`, `readability.sh` and `source-lines.sh` skip their Stop runs on skip/light/research turns that changed no Source file.

The system must therefore keep `rules/style.md` permanently in context for these turns, and unconditionally run the gates for any code change, regardless of whether it's a structural refactor or a minor styling fix.
> **Correction:** `rules/style.md` and `rules/systematic.md` are injected once per session at SessionStart, in every mode. Hook output can only add context, so per-turn injection cannot remove them. Gates do run on every code change, by design: they are the checks for the non-negotiable Key Design Rules, and a styling fix is what they check.

### Research
#### Type 1 Classifiers and Semantic Routing
> **Unverified:** the "Type 1 vs Type 2 agentic systems" taxonomy is uncited. A primary source would settle it.
> **Correction:** semantic routers "such as LangChain's SemanticRouter" → `semantic-router` is Aurelio AI's library (`aurelio-labs/semantic-router`) and routes by embedding similarity without an LLM call. TreeJev's `classify` is an LLM choice node, so it is not the fast deterministic kind the section describes. (Reviewer knowledge, not fetched.)

#### Dynamic Context Injection and Guardrails
Static context management leads to context bloat, increased latency, and reduced instruction-following accuracy. Routers can mount route-specific context and run conditional guardrails.
> **Unverified:** no source given, and no measurement showing the effect at this project's scale (38 lines of injected rules).

#### What this means here
- **Where / Kind**: `.treejev/trees/sequence_tree_turn.json`, external comparison.
- **Confidence**: verified
> **Refuted:** the hook that consumes the tree was not read. At most this was *suspected*.
- **What happens**: The Claude hook only reads the text directive; all context and all post-turn gates are statically coupled to `code` or `deep`.
> **Refuted:** see claims 2, 3 and 5 in the ledger. `claude-md-hook.sh:27` and `lib/gates.sh:88-97`.
- **Why it matters**: unnecessary context for simple tasks, costly hooks run when not needed, no way to inject specialised context.
> **Refuted:** simple turns already get no extra context and no Stop gates. Specialised context can be added in the existing `case "$turnmode"`.
- **Recommendation**: add `inject_context` / `run_gates` arrays to action nodes, with a `refactor` route running only `readability.sh` and injecting `rules/systematic.md` and `docs/refactoring.md`.
> **Refuted:** `docs/refactoring.md` does not exist. `rules/` is a plugin path. Dropping `design-rules.sh` for refactors conflicts with the non-negotiable rules. TreeJev's schema is an external tool's, and the hook already routes on the label.
- **Cost**: "significant context window optimization, lower latency".
> **Refuted:** unmeasured. The latency in this path is the per-prompt LLM classifier, which the recommendation leaves unchanged.

### Sources
> **Corrected:** `langchain-ai/semantic-router` → `aurelio-labs/semantic-router`. "Type 1 vs Type 2 Agentic Systems literature" cannot be traced as cited.

# Developer Tooling Layering Audit

> Reviewed 2026-09-26 at c215f8e by Claude. Source: Unreviewed/ProgramAudits/developer_tooling_layering_audit.md (analysis agent `agy`, Antigravity on Gemini; model not recorded).
> External state reviewed: `refactor-tools` at 977942a plus uncommitted and untracked work (`bin/`, `hooks/`, `lib/`, `rules/` untracked); `research-suite` at 06e35a7 plus staged renames and deletions; plugin cache `refactor-tools/0.3.0`, `research-suite/0.6.0`. SequenceTree's `.claude/research.toml` is not in c215f8e, and `.claude/CLAUDE.md` and `.claude/refactor.toml` differ from it, so this review reads the working tree.
> Verdict: Major revision

## Advisor Review

### Assessment
The report audits how the project's tooling is laid out across the two plugin repositories, the plugin cache and the project's config files. Most of its code facts hold up: the plugin bundles several things, hooks run from the cache, config is duplicated, CLAUDE.md overlaps other docs, the rules are injected twice, and the project gates are regexes. The problem is what it builds on those facts. Its three P1s all depend on a consequence or a number that doesn't hold. The claim that user scope applies policies to every project is refuted, because the gates are opt-in per project. "205 s on Stop" adds up timeout caps for hooks that run in parallel; measured, the Stop hooks take about 6 s. "Bloating by ~38 KB" gives the size of the whole file, when the tooling sections are about 9 KB of it. It also misses the most important fact in its own subject: the gate engine that is running right now exists only as untracked files in `refactor-tools` and as a copy in the plugin cache. Once these are corrected, the report is worth a handful of P2 and P3 housekeeping items.

### Claim Ledger
| # | Claim (short, quoted) | Where (current file:line) | Verdict | Evidence / correction |
|---|---|---|---|---|
| 1 | "`refactor-tools` … contains a C++ … CLI, … style rules, and a … hook/gate engine" | `refactor-tools/{refactor,commands,rules,bin,hooks,lib}` | Confirmed | All present. `bin/`, `hooks/`, `lib/` and `rules/` are **untracked** in git, though (see #20). |
| 2 | "`refactor-tools` is installed at the `user` scope", research-suite at project scope | `~/.claude/plugins/installed_plugins.json` | Confirmed | `scope: user`, and `scope: project` with `projectPath` SequenceTree. |
| 3 | "The rules are entirely language-agnostic" | `refactor-tools/rules/style.md:15,17` | Refuted | The rules forbid `inline` and namespaces, both C++ keywords, and the gates that enforce the rules are clang-AST checks. The rules fit a C++ plugin. |
| 4 | User scope means "policies could inappropriately apply globally" | `refactor-tools/lib/project.sh:66-69` | Refuted | Every hook sources `project.sh`, which exits 0 with no output unless the project's `.claude/refactor.toml` has a `[gates]` table. Running `session-rules.sh --hook` with `CLAUDE_PROJECT_DIR=/tmp` printed only the "gates are off here" note to stderr. Scope safety comes from opt-in, not from the install scope. |
| 5 | "`refactor.*` tools run via an editable pip install from the source repository" | `refactor` package → `~/Documents/GitHub/refactor-tools/refactor` | Confirmed | `python3 -c 'import refactor'` resolves to the repo. `bin/` from the repo is also on `PATH` (`which design-rules.sh`), which the report does not mention. |
| 6 | Hooks run from the cache via `${CLAUDE_PLUGIN_ROOT}`; `PYTHONPATH="$plugin_root"` | `refactor-tools/hooks/hooks.json:8-100`, `lib/project.sh:4,25` | Confirmed | `plugin_root` is resolved relative to the script, so it is the cache for hooks and the repo for manual runs from `PATH`. |
| 7 | "Manual verdicts and automated hook verdicts will silently diverge" | cache vs repo | Corrected | This is a real mechanism, but divergence isn't happening now: `diff -rq` between the repo and `cache/.../0.3.0` shows only `.in_use`. It isn't silent either. CLAUDE.md's *Analysis, Reviews and Proposals* section records the bump-and-update step. P1 → P3. |
| 8 | "use `claude --plugin-dir`" | `claude --help` | Confirmed (exists) | The flag exists. Whether it conflicts with the same plugin also installed at user scope is **Unverified**. |
| 9 | refactor.toml L3-8 and research.toml L5-10 duplicate `build_dir`, `build_trees`, `build_target`, `[test_targets]` | `.claude/refactor.toml:3-8`, `.claude/research.toml:5-10` | Confirmed | The values are identical. |
| 10 | "parsed by two different loaders … one via `refactor.workspace`, the other via `tomllib`" | `research-suite/lib/project.sh:14-41` | Corrected | The research-suite loader reads `[pipeline]`, `analyst_profile`, `issues_file` and `tools_file` only. It never parses the build keys. Only the markdown skills read those keys, as instructions to the model. The duplication is between one parser and prose, not two parsers. |
| 11 | CLAUDE.md is "38 KB" | `.claude/CLAUDE.md` | Confirmed | 38,267 bytes in the working tree, 37,019 at c215f8e. |
| 12 | Refactoring Commands (L55-80) and Analysis… (L81-105) are "already covered in `REFACTORING.md` and `.claude/research-tools.md`" | `.claude/CLAUDE.md:55,81`; `.claude/research-tools.md:22-24` | Corrected | The line ranges are right. Only part of the content overlaps. The "start the threshold low" and "`shape` relaxes spelling" guidance (CLAUDE.md L72-78) repeats research-tools.md L23-24 almost word for word. The pipeline section overlaps the skills' own SKILL.md files more than research-tools.md, which has no pipeline description. |
| 13 | Removing it "drastically reduces context window bloat … by ~38 KB" | section sizes of `.claude/CLAUDE.md` | Refuted | Measured by section, the two tooling sections are about 9.1 KB (Refactoring Commands 2.2 + Reaching for them 1.9 + Analysis 5.0). Architecture is about 23 KB, and the report keeps it. Removing the tooling text saves at most about 24% of the file. |
| 14 | Project Design Rules are in CLAUDE.md (L193-202) *and* injected by `session-rules.sh` | `refactor-tools/bin/session-rules.sh:9-28`; `.claude/CLAUDE.md:193` | Confirmed | Both copies are in this review session's own context. The section is about 1.4 KB, so the cost is small. |
| 15 | Duplicated `commands/` vs `.claude/commands/`, `skills/` vs `.claude/skills/` in refactor-tools | `refactor-tools/commands`, `.claude/commands`, `skills/refactor-tools`, `.claude/skills/refactor-tools` | Confirmed | `diff -rq` reports both pairs as byte-identical, and both copies of `commands/` are tracked. |
| 16 | "`research-suite` has renamed `gemini` to `analyst` and deleted `bin/` scripts locally without committing" | `research-suite` `git status` | Confirmed | The renames and deletions are staged but not committed. |
| 17 | "Over-generalized modularization … massive friction" (Confidence: verified) | — | Unverified | This is an inference labelled *verified*. The evidence (a migration in progress on the day of the audit) is not evidence of lasting friction. It would be settled by a record of sync failures or time lost over several weeks. |
| 18 | Stop hooks total "up to 205 seconds" | `refactor-tools/hooks/hooks.json:65-105` | Refuted | The Stop timeouts are 20+20+150+10 = 200, not 205. They are caps, and Claude Code runs matching hooks in parallel. Measured with a probe payload: design-rules 6.35 s, readability 0.43 s, systematic-check 0.04 s, source-lines 0.04 s. The 150 s cap belongs to the Haiku judge (`systematic-check.sh:211-260`), which runs only on turns that opt in with `-deep`. |
| 19 | `[[gates.checks]]` regexes are brittle | `.claude/refactor.toml:32-38` | Confirmed (with better evidence) | I ran the allocation pattern on sample lines. It misses `auto v = std::vector<int>(n);`, `std::unordered_map<int,int> m;`, `std::vector<std::vector<int>> x;`, `juce::Array<int> a;` and `auto s = juce::String(x);`. It flags the non-allocating `std::vector<int> const& ref = member;`. The report asserted brittleness without showing a single case. |
| 20 | (Omitted) The gate engine is untracked | `refactor-tools` `git status`; `installed_plugins.json` `gitCommitSha 977942a` | Confirmed (reviewer) | `bin/`, `hooks/`, `lib/` and `rules/` are `??` in refactor-tools and staged-deleted in research-suite. The marketplace is a `directory` source, so the cache copied the working tree. Its recorded SHA 977942a does not contain the hooks it is running. |
| R1 | Split into three plugins plus a pip package, all project-scoped | — | Rejected | This solves the scope problem that #4 refutes, and adds a third plugin to a solo project. |
| R2 | Hook scripts as thin shims over pip entry points | — | Judged: mixed | This removes the cache/repo split, but the hooks would then run the live working tree, so a half-edited gate would fire in the middle of a session. The cache currently protects against that. |
| R3 | One shared `.claude/tooling.toml` and loader | — | Judged: disproportionate | The duplicated content is five lines. A cheaper fix is for research-suite's skills to read the build keys from `refactor.toml`, since research-tools.md already names refactor-tools as the tool provider. |
| R4 | Remove duplicated tooling docs; inject rules once | — | Judged: sound, small | The Measurement guidance can live in one place. Double injection costs about 1.4 KB. |
| R5 | Collapse the repos into one marketplace repo | — | Rejected | This reverses a split the owner made on 2026-09-26, without evidence of harm. |
| R6 | Replace regex gates with tree-sitter AST checks; move long hooks to CI | — | Corrected | An AST check still cannot see reserved capacity or what is reachable from `processBlock`. The authoritative check already exists: `SequenceTree_RealtimeTests` under `SEQUENCETREE_REALTIME_SANITIZER` (`CMakeLists.txt:5,173`). The project has no CI. |

**Counts:** Confirmed 11 (1, 2, 5, 6, 8, 9, 11, 14, 15, 16, 19) plus 1 reviewer-added (20) · Corrected 3 (7, 10, 12) · Refuted 4 (3, 4, 13, 18) · Unverified 1 (17) · Stale 0. The six recommendations are judged, not verified.

### Critique
- **Method.** The agent opened the right files (`hooks.json`, `lib/project.sh`, `installed_plugins.json`, both TOMLs) and stated the code facts correctly. Where it failed was the step from a fact to its consequence. It read `scope: user` without reading the `[gates]` opt-in at `lib/project.sh:66` that every hook passes through. It added up timeouts without running a hook. It gave a file size as the size of one part of the file. Each of these would have been settled by one command.
- **Evidence vs conclusion.** All six findings say *Confidence: verified*. Finding 5 is labelled an inference in its own *Kind* line, and findings 2, 4 and 6 draw consequences nobody measured. By the analysis skill's own definitions, those are *traced* at best. The false confidence is what turned three facts into three refutations.
- **Commit pinning.** The report pins everything at c215f8e, which is SequenceTree's commit. Almost everything it discusses lives in other repositories, and much of it is uncommitted. A tooling audit has to record each repository's SHA and its dirty state.
- **Omissions.** (a) The finding that matters most: the running gate engine is untracked in git (#20). (b) The gates are opt-in per project. (c) Hooks run in parallel, and the judge runs only on `-deep` turns. (d) The RealtimeSanitizer test target, which is the real enforcement of the allocation rule and makes the regex a cheap tripwire rather than the check itself. (e) CLAUDE.md already documents the bump-and-update step.
- **Proportionality.** The profile says solo project, no enterprise patterns. Three plugins, a shared config file with a shared loader, and moving checks to CI (the project has no CI) all go past that. Collapsing the repos reverses a decision made the same day.
- **Fit with the rules.** This is tooling, not C++, so the Key Design Rules don't apply. The report does keep to the brief's advisory role and suggests no edits under `.claude/`.
- **Scope.** The analyst brief and profile point at the codebase. The profile's "check whether each [tool] is used to its full extent" does allow a tooling audit, but the report's value to the project would have been higher if it had asked whether the gates catch real violations in `Source/`.

### What Survives
- **P2 — The gate engine is not in version control.** `refactor-tools` has `bin/`, `hooks/`, `lib/` and `rules/` untracked, and `research-suite` has their deletions staged. The plugin cache (`refactor-tools/0.3.0`, recorded SHA 977942a) is a copy of an uncommitted working tree. A `git clean` or re-clone loses every hook this project runs, and the recorded SHA does not describe what is installed. The fix is to commit both repositories.
- **P2 — The allocation gate has concrete false negatives.** `.claude/refactor.toml:38` misses `auto x = std::vector<T>(n)`, `unordered_map`, nested templates, `juce::Array` and temporary `juce::String`. It flags `std::vector<int> const&` references. It is a tripwire. The allocation rule is actually enforced by `SequenceTree_RealtimeTests` (`CMakeLists.txt:173`), which only runs in the out-of-tree RTSan build.
- **P3 — Project Design Rules are injected twice.** They come from `refactor-tools/bin/session-rules.sh:9-28` and from `.claude/CLAUDE.md:193` loading natively, about 1.4 KB per session.
- **P3 — Measurement guidance is duplicated.** `.claude/CLAUDE.md:72-78` and `.claude/research-tools.md:22-24` say the same thing, so they will drift.
- **P3 — Build config is duplicated.** `.claude/refactor.toml:3-8` and `.claude/research.toml:5-10` hold the same build keys. research-suite never parses them itself, so one copy could go.
- **P3 — `commands/` and `skills/refactor-tools` are duplicated byte for byte** under `refactor-tools/.claude/`.
- **P3 — Hooks run the cache, manual runs run the repo.** `lib/project.sh:4`. They are in sync today and the update step is documented. This only matters while the tools are being edited.

### Questions for the Author
1. For every hook, what is the path from the entry point to the first decision? Show the `[gates]` opt-in before claiming anything about scope.
2. Measure before quoting latency. Time each hook on an ordinary turn and on a `-deep` turn, and report wall time given that hooks run in parallel.
3. Pin every repository you audit to its own SHA and its dirty state.
4. Does the regex gate catch anything the RTSan test would miss, or the other way round? Run `design-rules.sh --all` and the RealtimeTests build and compare what each finds in `Source/Audio`.
5. Is the "friction" of the multi-repo layout measurable over time, for example as sync mistakes recorded in the ongoing-issues file? If not, drop the finding.

## Corrected Report

# Developer Tooling Layering Audit

> Type: Program audit
> Written 2026-09-26 at c215f8e. Scope: This project's tool bindings, the refactor-tools repository, the research-suite repository, plugin caches, and settings. Excludes the C++ plugin implementation itself.
> Answers: N/A

> **Correction:** "at c215f8e" → c215f8e is SequenceTree's commit. The subject was refactor-tools 977942a plus untracked files, and research-suite 06e35a7 plus staged changes. `.claude/research.toml` is not in c215f8e.

## Summary
- **[P1 → P3] Separation of concerns:** `refactor-tools` bundles a C++ CLI, a hook engine and C++ style rules in one plugin (`~/Documents/GitHub/refactor-tools/`).
  > **Correction:** "language-agnostic rules" → the rules name C++ keywords, and the bundle fits a C++ tool. The scope consequence is refuted (see below).
- **[P1 → P3] Execution divergence:** Tools execute from three different locations (pip install, repo `bin/`, versioned plugin cache), allowing divergence between manual and automated hook runs (`hooks/hooks.json`).
  > **Correction:** "silent" → the update step is documented in CLAUDE.md, and the cache matches the repo today.
- **[P2 → P3] Duplicated configuration:** The build and test target keys are identical in `.claude/refactor.toml` and `.claude/research.toml`.
  > **Correction:** "parsed by two different loaders" → only refactor-tools parses them. research-suite's markdown skills read them as prose.
- **[P2 → P3] Documentation drift:** `.claude/CLAUDE.md` repeats some tooling guidance that is also in `research-tools.md`.
  > **Correction:** "~38 KB" → 38 KB is the whole file. The tooling sections are about 9 KB.
- **[P2] Over-generalized modularization.**
  > **Unverified:** the claimed friction would need a record of sync failures over time.
- **[P1 → P2] Friction and fragility:** The audio-thread checks use regexes with demonstrable false negatives.
  > **Refuted (latency half):** the Stop hooks run in parallel and take about 6 s. The 150 s cap applies only to the opt-in `-deep` judge.

## How It Works Now
The developer tooling is split across two external repositories (`refactor-tools` and `research-suite`), installed as Claude plugins from `directory`-source marketplaces. They read project-local configuration (`.claude/refactor.toml` and `.claude/research.toml`).
- **`refactor-tools`** is the C++ refactoring engine (a Python CLI backed by clang and tree-sitter) and also holds the hook engine and the style rules. It is installed at `user` scope, but every hook is a no-op in a project without a `[gates]` table (`lib/project.sh:66-69`).
- **`research-suite`** holds the analysis and proposal pipeline, as markdown skills with no hooks. It is installed at `project` scope.
- **Data flow:** hooks run from the plugin cache. They read `.claude/refactor.toml` through `refactor.workspace`, evaluate the project's regex checks and the clang-AST checks, and extract the Project Design Rules from `.claude/CLAUDE.md`.

> **Correction:** the original said the hooks "evaluate regexes or invoke the python CLI". The built-in checks for braces, wrappers, accessors and small classes use clang ASTs (per the `design-rules.sh` hook output). Only the project's `[[gates.checks]]` are regexes.

## Findings

### [P1 → P3] refactor-tools bundles three separable products into one clang-dependent plugin
- Where: `~/Documents/GitHub/refactor-tools/` and `~/.claude/plugins/installed_plugins.json`
- What happens: The repository contains the refactoring CLI (`refactor/`, `commands/`), the style rules (`rules/style.md`, `rules/systematic.md`) and the hook/gate engine (`bin/`, `hooks/hooks.json`, `lib/`). It is installed at `user` scope.
> **Refuted:** "The rules are entirely language-agnostic" → `rules/style.md:15,17` forbid `inline` and namespaces, and the gates that enforce them are clang-based.
> **Refuted:** "these policies could inappropriately apply globally" → `lib/project.sh:66-69` exits silently unless the project's `refactor.toml` has `[gates]`. Tested: `session-rules.sh --hook` in `/tmp` produced no context.
- Recommendation (judged): Rejected. The opt-in already provides the scope safety the three-way split aims for.

### [P1 → P3] Execution divergence across three unsynced locations
- Where: `refactor-tools/hooks/hooks.json:8-100`, `lib/project.sh:4,25`
- What happens: The `refactor.*` CLI runs through an editable pip install from the repository, and `bin/` scripts on `PATH` run from the repository. Hooks run from the versioned plugin cache via `${CLAUDE_PLUGIN_ROOT}`, with `PYTHONPATH="$plugin_root"`.
- Why it matters: Edits to the gates reach manual runs immediately, and reach hooks only after a version bump and `claude plugin update`.
> **Correction:** "silently diverge" → CLAUDE.md documents the bump-and-update step, and `diff -rq` shows the cache identical to the repo today. The risk exists only while the tools are being edited.
- Recommendation (judged): `--plugin-dir` exists; whether it clashes with the user-scope install is **Unverified**. Pip shims would make the hooks run the live, possibly half-edited, working tree, so that is a trade-off, not a fix.

### [P2 → P3] Configuration duplication across tools
- Where: `.claude/refactor.toml:3-8`, `.claude/research.toml:5-10`
- What happens: `build_dir`, `build_trees`, `build_target` and `[test_targets]` are identical in both files.
> **Correction:** "Each plugin contains its own `lib/project.sh` to load these" → `research-suite/lib/project.sh:14-41` does not load the build keys. The review, propose and implement skills read them as instructions.
- Recommendation (judged): A shared file and loader is disproportionate for five lines. Pointing research-suite's skills at `refactor.toml` would do the same job.

### [P2 → P3] CLAUDE.md bloat and double-injection of rules
- Where: `.claude/CLAUDE.md:55,81,193`, `refactor-tools/bin/session-rules.sh:9-28`
- What happens: CLAUDE.md's Refactoring Commands (L55-80) and Analysis, Reviews and Proposals (L81-105) sections partly repeat material documented elsewhere. `session-rules.sh` extracts the Project Design Rules (L193 onward) from CLAUDE.md and injects them at SessionStart, although CLAUDE.md is already loaded.
> **Correction:** "already covered in `REFACTORING.md` and `.claude/research-tools.md`" → the verbatim overlap is the measurement guidance (CLAUDE.md L72-78 vs research-tools.md L23-24). The pipeline section overlaps the skills' SKILL.md files, not research-tools.md.
> **Refuted:** "drastically reduces … ~38 KB" → the tooling sections are about 9.1 KB of 38.3 KB. The double injection is about 1.4 KB.
- Recommendation: Keep the measurement guidance in one place, and inject the Project Design Rules one way only.

### [P2] Over-generalized modularization for a solo project
- What happens: Two external repositories. Duplicated `commands/` and `.claude/commands/`, and duplicated `skills/refactor-tools` and `.claude/skills/refactor-tools`, both byte-identical (confirmed). `research-suite` has staged, uncommitted renames (`gemini/` → `analyst/`) and deletions of `bin/`, `hooks/` and `rules/` (confirmed).
> **Unverified:** "massive friction" is an inference presented as verified. A migration in progress on the audit date is not evidence of lasting friction.
> **Correction (omission):** the other half of the migration is the serious part. `refactor-tools` has `bin/`, `hooks/`, `lib/` and `rules/` **untracked**, so the whole running gate engine exists only in a working tree and a cache copy whose recorded SHA (977942a) does not contain it.
- Recommendation (judged): Rejected (collapsing the repos). Commit both repositories instead.

### [P1 → P2] Extreme hook latency and brittle regex gates
- Where: `refactor-tools/hooks/hooks.json:65-105`, `.claude/refactor.toml:32-55`
> **Refuted:** "up to 205 seconds on `Stop`" → the caps are 20, 20, 150 and 10 (200 total). Hooks run in parallel. Measured: 6.35, 0.43, 0.04 and 0.04 s. The 150 s cap is the `-deep` Haiku judge (`systematic-check.sh:211-260`), which runs only when asked for.
- What happens (confirmed, with evidence the original lacked): the allocation pattern at `.claude/refactor.toml:38` misses `auto v = std::vector<int>(n);`, `std::unordered_map<…> m;`, `std::vector<std::vector<int>> x;`, `juce::Array<int> a;` and `auto s = juce::String(x);`. It flags `std::vector<int> const& ref = member;`.
> **Correction:** "Replace the regex gates with AST-based checks" → no static check sees reserved capacity or what is reachable from `processBlock`. The project already has the authoritative check, `SequenceTree_RealtimeTests` under `SEQUENCETREE_REALTIME_SANITIZER` (`CMakeLists.txt:5,173`). The project has no CI to move hooks to.

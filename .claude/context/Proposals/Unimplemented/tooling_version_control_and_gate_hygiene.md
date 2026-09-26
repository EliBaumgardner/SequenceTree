# Tooling in Version Control, a Sharper Allocation Gate, and One Copy of Each Rule

> Status: In progress
> Built from c215f8e; drift: none
> Written 2026-09-26 at c215f8e (SequenceTree, dirty working tree), refactor-tools 977942a plus untracked `bin/ hooks/ lib/ rules/`, research-suite 06e35a7 plus staged changes. Sources: Reviewed/ProgramAudits/developer_tooling_layering_audit.md (What Survives: P2 gate engine not in version control, P2 allocation gate false negatives, P3 rules injected twice, P3 measurement guidance duplicated, P3 build config duplicated, P3 duplicated `commands/` and `skills/`, P3 hooks run the cache)

## Summary
The gate engine every SequenceTree hook runs — `bin/`, `hooks/`, `lib/` and `rules/` in refactor-tools — exists only as untracked files and as a copy in the plugin cache, while research-suite has the old copies staged for deletion; SequenceTree's own pipeline config (`research.toml`, `research-tools.md`, `.analyst/`) is untracked too, although CLAUDE.md says both configs are committed. A `git clean`, a re-clone, or committing research-suite before refactor-tools would leave no committed copy of the running hooks. Separately, the project's audio-thread allocation regex misses five common allocating forms and flags a harmless reference, and three pieces of text (Project Design Rules, measurement guidance, refactor-tools' slash commands and skill) exist in two copies each. The fix is seven small steps: commit the three repositories in a safe order, replace one regex (validated here against the whole tree), inject the Project Design Rules once, keep the measurement guidance in one file, and delete the duplicate command and skill copies. No step touches `Source/`.

## The Problem

### Mechanism

**1. The running gate engine is not in any commit.**
- `~/.claude/plugins/installed_plugins.json` records `refactor-tools@refactor-tools` at `cache/refactor-tools/refactor-tools/0.3.0` with `gitCommitSha 977942a`. The marketplace is a `directory` source, so the cache is a copy of the working tree, not of that commit.
- `git status` in refactor-tools at HEAD 977942a: `?? bin/`, `?? hooks/`, `?? lib/`, `?? rules/`, `?? refactor/init_project.py`, `?? skills/init/`, plus ten modified tracked files (`refactor/workspace.py`, `refactor/cli.py`, `.claude-plugin/plugin.json`, …).
- `git status` in research-suite at 06e35a7: `D  bin/{claude-md-hook,design-rules,readability,refactor-gate,repeat-check,session-rules,source-lines,systematic-check,turn-scope}.sh`, `D  hooks/hooks.json`, `D  rules/{style,systematic}.md` staged, plus `gemini/ → analyst/` renames and unstaged skill edits.
- `hooks/hooks.json:8-100` in refactor-tools runs `${CLAUDE_PLUGIN_ROOT}/bin/*.sh`, each of which sources `lib/project.sh` and reads `rules/*.md`. None of those files is tracked in refactor-tools.
- Concrete input that exposes it: `git clean -fd` in refactor-tools, or a fresh clone on another machine followed by `claude plugin update refactor-tools@refactor-tools`, installs a plugin with no `hooks/` — every gate this project relies on silently stops. Committing research-suite first (its staged deletions) removes the last committed copy from a branch tip.
- SequenceTree itself: `git status` shows `?? .claude/research.toml`, `?? .claude/research-tools.md`, `?? .analyst/`, `?? .claude/context/Analysis/`, and the old `GeminiAnalysis/` files deleted but unstaged. `.claude/CLAUDE.md:97` states "Both configs are committed", which is false at c215f8e.

**2. The allocation gate misses allocating forms.**
- `.claude/refactor.toml:32-38` declares the check; refactor-tools `lib/project.sh:53-57` serialises it and `bin/design-rules.sh:398-420` applies it with `awk` line by line to files under `Source/Audio/`, only inside function bodies (`in_functions = true`), skipping `prepare|prepareToPlay|reserve|setup|releaseResources`, after stripping `//` comments.
- The container branch is `(^|[^A-Za-z0-9_:])(std::)?(string|vector|map|set)[[:space:]]*<[^>]*>[[:space:]]+[A-Za-z_]`. It requires whitespace and a letter after the first `>`, so:
  - `auto v = std::vector<int>(n);` — `>` is followed by `(` → missed.
  - `std::unordered_map<int,int> m;` — `map` is preceded by `_` → missed.
  - `std::vector<std::vector<int>> x;` — `[^>]*` stops at the inner `>`, which is followed by `>` → missed.
  - `juce::Array<int> a;` — `Array` is not in the list → missed.
  - `auto s = juce::String(x);` — the `juce::String` branch needs whitespace then a letter → missed.
  - `std::vector<int> const& ref = member;` — `const` satisfies `[A-Za-z_]` → flagged, although nothing allocates.
- Verified by running both patterns over the sample lines above with BSD `awk` (the engine's own matcher) on 2026-09-26.

**3. The Project Design Rules are injected twice.**
- Claude Code loads `.claude/CLAUDE.md` natively, which includes `### Project Design Rules` at `.claude/CLAUDE.md:193`.
- refactor-tools `bin/session-rules.sh:9-28` finds `rules_file` (`lib/project.sh:71-78` picks `.claude/CLAUDE.md`), extracts the section under `project_rules` (`### Project Design Rules`) with `awk`, and appends it to the SessionStart `additionalContext` (`session-rules.sh:31-33`). This session's own context holds both copies.

**4. Measurement guidance is written twice.** `.claude/CLAUDE.md:74-79` ("Duplication questions go to…", "Start the threshold low…", "`shape` relaxes spelling…") and `.claude/research-tools.md:22-24` say the same things in different words; CLAUDE.md's copy is the fuller one (it names `shape_of` in `refactor/reporting/find.py`).

**5. refactor-tools ships its commands and skill twice.** `commands/*.md` and `.claude/commands/*.md`, and `skills/refactor-tools/SKILL.md` and `.claude/skills/refactor-tools/SKILL.md`, are byte-identical (`diff -rq`, both empty) and all tracked. The plugin loads `commands/` and `skills/`; the `.claude/` copies are project-level duplicates that only apply when Claude works inside the refactor-tools repository — where the user-scope plugin already provides them.

### Evidence
- Review ledger #20 (engine untracked), #19 (regex false negatives), #14 (double injection), #12 (measurement overlap), #15 (duplicate copies), #9/#10 (build config), #6/#7 (cache vs repo).
- Re-read at HEAD today: every `git status` above, `installed_plugins.json`, `session-rules.sh`, `lib/project.sh`, `design-rules.sh:398-420`, `.claude/refactor.toml:32-38`.
- Replacement pattern tested through the real engine: a scratch copy of `Source/` with the new pattern in its `refactor.toml` (outside the repo, in `/tmp`, since deleted) gave **no** "Never allocate or free on the audio thread" hit on `design-rules.sh --all`, and a probe file under `Source/Audio/` gave hits on `auto v = std::vector<int>(4);` and `juce::Array<int> a;` and none on `const std::vector<int>& r = v;`.

### Why It Matters
- **P2** — Finding 1: the tooling that enforces the Key Design Rules on every edit can be lost by a routine git operation, and the recorded SHA misdescribes what is installed. Losing it is silent: `lib/project.sh` has nothing to exit on because the hooks never run at all.
- **P2** — Finding 2: the gate is the only on-edit guard for "Never allocate or free on the audio thread"; the authoritative guard, `SequenceTree_RealtimeTests` (`CMakeLists.txt:173`), runs only in the out-of-tree Homebrew-LLVM build, so allocations the regex misses reach the tree unannounced until someone runs RTSan.
- **P3** — Findings 3–5: duplicated text drifts, and the design-rule copy costs ~1.4 KB of context per session.

## Current Design

### API Surface
| Piece | Where | Who reads it |
|---|---|---|
| `[gates]`, `[[gates.checks]]` | `.claude/refactor.toml:10-55` | `refactor.workspace.CONFIG` via `lib/project.sh:25-64` |
| `project_checks` (rule, label, paths, in_functions, skip, pattern) | `lib/project.sh:53-57` | `design-rules.sh:398-420` |
| `rules_file`, `project_rules` | `lib/project.sh:10-11,48,59-60,71-78` | `session-rules.sh:9-28`, `claude-md-hook.sh` |
| `session-rules.sh --hook` | `hooks/hooks.json:8` (SessionStart) | Claude Code |
| `session-rules.sh` (no flag) | `.claude/research-tools.md:7` | non-Claude agents (the analyst) |
| Build keys `build_dir`, `build_trees`, `build_target`, `[test_targets]` | `.claude/refactor.toml:3-8`, `.claude/research.toml:5-10` | refactor-tools parses the first (`refactor/workspace.py`); research-suite's skills (`skills/implement/SKILL.md:38`, `review/SKILL.md:44`, `analyst/ANALYST.md:67`) read the second as prose; `lib/new_config.py:76-95` writes it |
| Measurement guidance | `.claude/CLAUDE.md:74-79`, `.claude/research-tools.md:22-24` | every Claude session; pipeline stages and the analyst |
| Plugin commands and skill | refactor-tools `commands/`, `skills/refactor-tools/` (+ `.claude/` copies) | plugin loader |

### Structure
- **refactor-tools** (user scope, opt-in per project through `[gates]`, `lib/project.sh:66-69`) owns the `refactor.*` CLI, the gates, the hooks and the style rules. CLAUDE.md's *Refactoring Commands* and *Analysis, Reviews and Proposals* sections assign all tool changes to that repository.
- **research-suite** (project scope) owns the markdown pipeline and `lib/project.sh` for `[pipeline]` keys only. It has no hooks since the migration that is still staged.
- **SequenceTree** owns only configuration: `refactor.toml`, `research.toml`, `research-tools.md`, `.analyst/profile.md`, and `CLAUDE.md`.

### Data Flow
```
refactor-tools working tree ──(claude plugin update)──▶ ~/.claude/plugins/cache/.../0.3.0 ──(${CLAUDE_PLUGIN_ROOT})──▶ hooks
        │                                                                                       │
        └──(bin/ on PATH, editable pip install)──▶ manual design-rules.sh / refactor.*          ▼
                                                        .claude/refactor.toml ─▶ lib/project.sh ─▶ design-rules.sh awk checks
                                                        .claude/CLAUDE.md ─────▶ session-rules.sh ─▶ SessionStart context
                                                        .claude/CLAUDE.md ─────▶ Claude Code (native load)
```
No audio-thread or message-thread code is involved anywhere in this proposal.

## Approaches Considered

**Finding 1 — version control**
- **A. Commit each repository as it stands, refactor-tools first.** Cheap; the order guarantees a committed copy always exists. Recommended.
- B. Move the gate engine back into research-suite and unstage its deletions. Reverses the owner's 2026-09-26 split. Rejected.
- C. Vendor the engine into SequenceTree. Contradicts CLAUDE.md ("changes to the tools are made there, never in this project"). Rejected.

**Finding 2 — allocation gate**
- **A. Replace the one regex** with the tested pattern below. One line in `refactor.toml`, no engine change. Recommended.
- B. An AST allocation check in refactor-tools (clang-query for `CXXNewExpr`, container construction). More precise on syntax but still blind to reserved capacity and to reachability from `processBlock` (review ledger R6). Disproportionate while RTSan exists.
- C. Leave it; rely on RTSan. RTSan runs only on demand in a separate tree, so the tripwire is worth a one-line fix.

**Finding 3 — double injection**
- **A. `session-rules.sh --hook` skips the project section when `rules_file` is a file Claude Code already loads** (`CLAUDE.md`, `.claude/CLAUDE.md`). Printing without `--hook` (for other agents) is unchanged. Recommended.
- B. A `[gates] inject_project_rules = false` key. A config switch for something the script can infer; one more key to document.
- C. Delete the section from CLAUDE.md and rely on the hook. Makes the rules depend on the plugin being installed. Rejected.

**Finding 4 — measurement guidance**
- **A. `.claude/research-tools.md` holds the full text** (it is the only one the analyst sees); CLAUDE.md keeps its command block and replaces L74-79 with one sentence pointing to research-tools.md's *Measurement* section. Recommended, but it is the owner's document — see Decisions.
- B. CLAUDE.md holds it, research-tools.md points. The analyst gets research-tools.md in its prompt but not CLAUDE.md, so it would lose the guidance.
- C. Leave both.

**Finding 5 — duplicate commands and skill**: **delete `.claude/commands/` and `.claude/skills/` in refactor-tools**; the plugin copies are the ones installed. The alternative (symlinks) keeps two paths for one file with no reader for the second.

**Build config duplication (P3)**: research-suite is designed to stand alone — `research-init`/`lib/new_config.py` write the build keys for projects that have no refactor-tools, and every skill names `research.toml`. Pointing it at `refactor.toml` couples the plugins for five lines. Recommended: leave it (see Decisions and Out of Scope).

## Plan

Steps 1–3 must run in order. Steps 4–7 are independent of each other.

### Step 1 — Commit refactor-tools, including the gate engine
- **Goal:** the hooks this project runs exist in a commit, and the installed plugin records that commit.
- **Changes:** in `~/Documents/GitHub/refactor-tools`: `git add bin hooks lib rules refactor/init_project.py skills/init` plus the ten modified tracked files; bump `.claude-plugin/plugin.json` `version` 0.3.0 → 0.3.1; one commit (message ends at its last line of content — no attribution trailer). Then `claude plugin update refactor-tools@refactor-tools`.
- **Mechanical edits:** none.
- **Behaviour delta:** none for any hook — the cache content is the same files; only the recorded SHA and version change.
- **Invariants:** no C++ touched. Commit order guarantees the engine is committed before research-suite drops its copy.
- **Verification:** `git status --short` in refactor-tools shows nothing under `bin/ hooks/ lib/ rules/`; `git ls-files hooks/hooks.json lib/project.sh` lists both; `installed_plugins.json` shows version 0.3.1 and the new SHA; `diff -rq ~/Documents/GitHub/refactor-tools ~/.claude/plugins/cache/refactor-tools/refactor-tools/0.3.1` reports only `.git`/`.in_use`-type noise; start a session in SequenceTree and see the "Loading design rules" and design-rule check output as today.
- **Rollback:** `git reset --soft HEAD~1` (files stay on disk); reinstall 0.3.0 is unnecessary since the content is identical.

### Step 2 — Commit research-suite's migration
- **Goal:** research-suite's committed state matches the installed 0.6.0 (no hooks, `analyst/` layout).
- **Changes:** in `~/Documents/GitHub/research-suite`: commit the staged renames and deletions together with the unstaged skill edits, `bin/research-init`, `lib/project.sh`, `skills/research/run-pipeline.sh` and `lib/new_config.py` (scope is a decision below). Confirm `.claude-plugin/plugin.json` already says 0.6.0 so the recorded SHA can be refreshed with `claude plugin update research-suite@research-suite --scope project`.
- **Behaviour delta:** none; the cache already holds this content.
- **Invariants:** runs only after Step 1, so `bin/session-rules.sh` et al. are committed somewhere at every moment.
- **Verification:** `git status --short` clean (or only the files the owner chose to hold back); `/research-suite:review` and `/research-suite:propose` still listed; `installed_plugins.json` records the new SHA.
- **Rollback:** `git reset --soft HEAD~1`.

### Step 3 — Commit SequenceTree's tooling config
- **Goal:** `.claude/CLAUDE.md:97` ("Both configs are committed") becomes true, and the pipeline's inputs are versioned with the code they describe.
- **Changes:** stage `.claude/research.toml`, `.claude/research-tools.md`, `.analyst/`, `.claude/context/Analysis/`, `.claude/context/Proposals/` (this file), the deletions under `.claude/context/GeminiAnalysis/` and `.claude/state/refactor-history.json`, `.claude/CLAUDE.md`, `.claude/refactor.toml`, `.gitignore`. **Do not** stage the `Source/UI/...` changes — they are separate work in progress (`PaintToolSettings.h`, `ValueEditor.*`, `ColourSelector.*`, `CustomLookAndFeel_Buttons.cpp`). One commit, no attribution trailer.
- **Behaviour delta:** none at runtime.
- **Invariants:** no `Source/` change, so no build effect; `.claude/*.local.toml` stay ignored (`.gitignore:25-26`, confirmed with `git check-ignore`).
- **Verification:** `git status --short` shows only the `Source/UI` files; `git ls-files .claude/research.toml .claude/research-tools.md .analyst/profile.md` lists all three.
- **Rollback:** `git reset --soft HEAD~1`.

### Step 4 — Replace the allocation pattern
- **Goal:** the tripwire catches temporaries, nested templates, `unordered_*`, JUCE containers and `juce::String` construction, and stops flagging references.
- **Changes:** `.claude/refactor.toml:38`, the `pattern` of the "Never allocate or free on the audio thread" check, becomes:
  ```
  pattern = '(^|[^A-Za-z0-9_])(new|delete)[[:space:]]|make_unique|make_shared|malloc\(|calloc\(|realloc\(|free\(|(^|[^A-Za-z0-9_:])(std::|juce::)?(string|vector|map|set|unordered_map|unordered_set|deque|list|function|Array|OwnedArray|HashMap|StringArray)[[:space:]]*<[^;=&*]*>[[:space:]]*([({]|[A-Za-z_][A-Za-z0-9_]*[[:space:]]*[;={(])|(juce::(String|StringArray|MemoryBlock)|std::string)[[:space:]]*([({]|[A-Za-z_][A-Za-z0-9_]*[[:space:]]*[;={(])'
  ```
  What changed and why: the template body is `[^;=&*]*` instead of `[^>]*`, so nested `>>` is crossed but a reference, pointer or assignment is not (an earlier draft with `[^;]*` falsely matched `const std::vector<TraversalKey>& disabled = connection->disabledTraversals;` at `Source/Audio/TraversalRule.cpp:45` by reaching the `>` of `->`); after the `>` it accepts either a temporary (`(` or `{`) or a declared name followed by `;`, `=`, `{` or `(`, which excludes `const&`; the name list adds `unordered_map`, `unordered_set`, `deque`, `list`, `function`, and JUCE's `Array`, `OwnedArray`, `HashMap`, `StringArray`; the string branch covers `juce::String`, `juce::StringArray`, `juce::MemoryBlock` and `std::string` as declarations or temporaries, and still ignores `juce::StringRef` and `std::string_view`.
- **Behaviour delta:**

  | Input line in a `Source/Audio/` function body | Before | After |
  |---|---|---|
  | `std::vector<int> x;`, `juce::String s;`, `new Foo`, `make_unique` | flagged | flagged |
  | `auto v = std::vector<int>(n);`, `auto w = std::vector<int>{1, 2};` | missed | flagged |
  | `std::unordered_map<int,int> m;`, `std::vector<std::vector<int>> x;`, `std::map<int, std::vector<int>> m;` | missed | flagged |
  | `juce::Array<int> a;`, `juce::StringArray names;`, `std::function<void()> f = g;`, `std::string s;` | missed | flagged |
  | `auto s = juce::String(x);` | missed | flagged |
  | `std::vector<int> const& ref = member;` | flagged | not flagged |
  | `const std::vector<int>& v`, `juce::StringRef`, `std::string_view`, `static_cast<std::size_t>` | not flagged | not flagged |
  | Still missed by both: `push_back` past capacity, implicit `juce::String` from a literal, `juce::String::formatted`, anything outside `Source/Audio/` (including `processBlock` in `Source/Plugin/PluginProcessor.cpp`) | — | — |

  Current tree: no hit before, no hit after (verified with `design-rules.sh --all` on a scratch copy).
- **Invariants:** configuration only; the audio thread is untouched. `std::function` is flagged because assigning one may allocate — a new hit on it in `Source/Audio/` deserves a look, not an automatic exemption.
- **Verification:** `design-rules.sh --all` from the project root shows no "Never allocate or free on the audio thread" section; then, in a scratch copy outside the repo (never a new file in `Source/`), a probe function under `Source/Audio/` containing the three probe lines gives exactly the two expected hits. `SequenceTree_Standalone`, `SequenceTree_Tests`, `SequenceTree_GraphTests` need no rebuild (no source change), but running them confirms nothing else moved.
- **Rollback:** restore the old line 38.

### Step 5 — Inject the Project Design Rules once
- **Goal:** a Claude session carries one copy of the Project Design Rules; other agents that print the rules still get them.
- **Changes:** refactor-tools `bin/session-rules.sh`, before the `if [ -n "$rules_file" ]` block at line 9:
  ```bash
  if [ "${1:-}" = "--hook" ]; then
      case "$rules_file" in
          CLAUDE.md|.claude/CLAUDE.md)
              rules_file=""
              ;;
      esac
  fi
  ```
  Bump `plugin.json` to 0.3.2, commit, `claude plugin update refactor-tools@refactor-tools`.
- **Behaviour delta:** `--hook` with the default `rules_file` (a CLAUDE.md Claude Code loads natively): project section no longer appended. `--hook` with a `[gates] rules_file` pointing anywhere else: unchanged. No flag (analyst, manual): unchanged. `rules/systematic.md` and `rules/style.md` are still injected in every case.
- **Invariants:** `.claude/CLAUDE.md:189-191` ("injected at the start of every session… The rules below are this project's own") stays accurate — the general rules are still injected, the project rules are loaded with CLAUDE.md. After compaction Claude Code reloads CLAUDE.md, so the project rules survive it.
- **Verification:** `CLAUDE_PROJECT_DIR=$PWD session-rules.sh --hook | jq -r .hookSpecificOutput.additionalContext | grep -c 'Project Design Rules'` → `0`; `CLAUDE_PROJECT_DIR=$PWD session-rules.sh | grep -c 'Project Design Rules'` → `1`; a new session's SessionStart context ends with the Key Design Rules.
- **Rollback:** revert the commit, reinstall 0.3.1.

### Step 6 — One home for the measurement guidance
- **Goal:** the `refactor.find` reading guidance exists once, where both Claude and the analyst read it.
- **Changes:** `.claude/research-tools.md:22-24` takes CLAUDE.md's fuller wording (including "Duplication questions go to `refactor.find repeating`, both likenesses, before reading files by hand", the `count >= 3` question, and the `shape_of` detail). `.claude/CLAUDE.md:74-79` is replaced by one sentence: duplication questions go to `refactor.find repeating`, both likenesses, and how to read its output is in the *Measurement* section of `.claude/research-tools.md`, read before answering one. The command block at L61-72 stays.
- **Behaviour delta:** an ordinary Claude session must open research-tools.md before a duplication question instead of having the detail in context; the analyst gains the fuller text.
- **Verification:** `grep -n "shape_of" .claude/CLAUDE.md .claude/research-tools.md` finds it only in research-tools.md.
- **Rollback:** `git checkout` both files.

### Step 7 — Delete refactor-tools' duplicate command and skill copies
- **Goal:** one copy of each slash command and of the skill.
- **Changes:** `git rm -r .claude/commands .claude/skills` in refactor-tools (keep `.claude/CLAUDE.md`). Bump to the next patch version, commit, `claude plugin update`.
- **Behaviour delta:** inside the refactor-tools repository, `/encapsulate` etc. no longer appear twice (project copy plus `refactor-tools:` plugin copy); everywhere else, nothing changes. `grep -rn "\.claude/commands\|\.claude/skills"` in refactor-tools found no reference to the copies.
- **Verification:** `diff -rq` no longer possible (copies gone); in a session in refactor-tools, `/refactor-tools:encapsulate` is listed and the unprefixed duplicate is not.
- **Rollback:** revert the commit.

## Risks
- **Committing half-finished work (Steps 1–2).** Both working trees carry edits beyond the migration (e.g. `refactor/workspace.py`, research-suite skill edits). They are what is installed and running today, so committing them records reality; the risk is only that a commit message describes them loosely. Guard: the owner picks the commit scope (Decision 2).
- **Pushing.** Committing is local; pushing to `github.com/EliBaumgardner/*` publishes. The plan does not push unless the owner says so (Decision 1).
- **New false positives from Step 4.** A future `Source/Audio/` function that legitimately builds a container off the audio thread (not named `prepare…`/`setup`/`reserve`) would be flagged. Only added lines are gated on edit (`only_added`), and `skip_functions` can be widened. Likelihood low; symptom is a gate message on an edit.
- **Step 5 on a project with the rules in a non-auto-loaded file** — handled: only the two auto-loaded names are skipped.
- **Step 6** moves guidance out of every-session context; if duplication answers get worse, revert.

## Out of Scope
- **Build config duplication** (`refactor.toml:3-8` vs `research.toml:5-10`): research-suite is meant to work without refactor-tools, and its init writes these keys. Five duplicated lines do not justify coupling the plugins. Revisit if they ever disagree.
- **Hooks run the cache, manual runs the repo** (`lib/project.sh:4`): the cache is what protects a session from a half-edited gate (review ledger R2), and the bump-and-update step is already in CLAUDE.md. Steps 1, 5 and 7 each end with that step.
- **Extending the allocation check to `processBlock`** in `Source/Plugin/PluginProcessor.cpp`: `paths` is a prefix regex, so `Source/(Audio/|Plugin/PluginProcessor)` would work, but that file also holds `createEditor` (`new`, L162), `createPluginFilter` (L266) and the parameter layout (`make_unique`, L35-41), which would each need a `skip_functions` entry. RTSan already covers `processBlock` end to end. Listed as Decision 5.
- **Pre-existing design-rule violations** that `design-rules.sh --all` reports today (ternaries in `ScriptEmitter.cpp:487`, `ScriptLexer.cpp:99`, `FileLine.cpp:112`, `FilePage.cpp:82`, `TraversalRulesWindow.cpp:65`, `ButtonPane.h:160`; brace-less loops in `ColourSelector.cpp:86,112` and an `if` in `TraversalRulesWindow.cpp:220`; ~20 forwarding-call wrappers). They are C++ design-rule work, not tooling, and belong in their own proposal.
- **Multi-repo layout itself** (review: Unverified "friction", recommendations R1/R5 rejected).

## Decisions for the Owner
1. **Push after committing?** Steps 1–3 and 5/7 commit locally. Recommend pushing refactor-tools and research-suite once Step 2 is done, so a re-clone gets the engine; SequenceTree is your call.
2. **Commit scope in the tool repos.** (a) Everything in each working tree, in one commit per repo (recommended — it is what is installed and running), or (b) only the migration files, leaving the other edits (`refactor/workspace.py`, `cli.py`, `init_project.py`, research-suite skill edits, `new_config.py`) for separate commits you write.
3. **Step 3 scope.** Confirm the `Source/UI` changes stay out of the tooling commit, and whether `.claude/context/Analysis/` (reports) and `.claude/context/Proposals/` should be committed at all or gitignored like `tracker.md`.
4. **Measurement guidance home (Step 6).** research-tools.md holds it and CLAUDE.md points to it (recommended, because the analyst only sees research-tools.md), or CLAUDE.md keeps it and research-tools.md is left as the short version (status quo, accepting drift).
5. **Allocation gate over `processBlock`?** Leave the check on `Source/Audio/` only and rely on RTSan for `Source/Plugin/` (recommended), or widen `paths` to `Source/(Audio/|Plugin/PluginProcessor)` with `skip_functions` extended by `createEditor|createPluginFilter|createParameterLayout|SequenceTreeAudioProcessor`.
6. **Flag `std::function` in Step 4?** Keep it in the pattern (recommended — assignment may allocate), or drop it if you use it in `Source/Audio/` deliberately.
7. **Build config duplication.** Leave as is (recommended), or change research-suite's skills to fall back to `refactor.toml` when `research.toml` has no build keys.
8. **Steps 5–7 are changes in refactor-tools, not this project.** CLAUDE.md says tool changes are made in that repository; confirm `/implement` may make them there from this session, or do them yourself.

### Owner's answers (2026-09-26)
1. Push all three repositories after committing.
2. Commit scope: everything in each tool repo's working tree, one commit per repo.
3. `Source/UI` stays out of the tooling commit; commit both `.claude/context/Analysis/` and `.claude/context/Proposals/`.
4–8. Recommendations accepted: measurement guidance lives in research-tools.md with CLAUDE.md pointing to it; allocation gate stays on `Source/Audio/`; `std::function` stays in the pattern; build keys stay duplicated; `/implement` may edit refactor-tools from this session.
Added by the owner: fix `run-pipeline.sh` so its git-status guards ignore the pipeline's own folders under `.claude/context/` (a new proposal tripped the propose guard on this proposal's own run; with `Analysis/` committed the analyze guard would trip on every report).

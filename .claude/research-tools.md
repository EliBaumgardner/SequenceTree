# Research Tools

The tools the research pipeline uses in SequenceTree. They come from the `refactor-tools` plugin and its `refactor.*` commands, and each stage reads the sections it needs. The analysis agent gets this whole file in its prompt.

## Design Rules

- The **Key Design Rules** and **How to Systematically Solve Problems** are refactor-tools' `rules/style.md` and `rules/systematic.md`. A Claude session already has them in context; any other agent prints them, together with the Project Design Rules, with `session-rules.sh`.
- In short, the Key Design Rules forbid comments, ternaries, wrapper functions, getters and setters, booleans or ints standing in for named states (use an enum), namespaces and `inline`, and require braces on every block. They are the owner's decisions, and every recommendation and every line of code must be writable within them.
- Three things are asked about before they are planned, never assumed: a new short function said to name a larger process, a new `core_purpose_api` entry under `[gates]` in `.claude/refactor.toml`, and keeping a class small enough to be flagged.
## Measurement

Read-only. Run these for evidence rather than reading everything by hand.

| Purpose | Command |
|---|---|
| Wrappers and accessors | `refactor.smell <path>` |
| Raw pointers | `refactor.smell pointers <path> [owning \| fields \| every]` |
| Long functions | `refactor.find function 'numlines > 80' <path>` |
| Duplication | `refactor.find repeating 'numlines > 3' <path>` and `refactor.find repeating shape 'numlines > 3' <path>` |
| Callers and references | clangd, or `refactor.callers` / `refactor.refs` |
| Command help | `refactor.help`, `refactor.help <command>` |

- Quote every condition, or the shell reads `>` as a redirect.
- Duplication questions go to `refactor.find repeating`, both likenesses, before reading files by hand — it reads the whole scope in about half a second and beats grepping for a remembered line.
- **Start the threshold low and read upward.** `numlines > 3` first; a high threshold silently hides the shorter half of a finding, and there is no indication that it did. `count >= 3` asks the other question — what has been written three times over.
- **`shape` relaxes spelling, not structure.** `shape_of` in refactor-tools' `refactor/reporting/find.py` spells every identifier as the one token `name`, so `spawnKey` is one token and `traversal.key` is three, and `obj.f(x)` and `f(x)` differ by a receiver. Both likenesses report *contiguous* runs, so two functions that do the same thing with different expressions plugged in come back as several short islands rather than one long finding. Read adjacent findings in the same pair of files as possibly one duplicate, and go read the sites before reporting a size.

## Verification

Run on every file a change touches, and on the files a report makes design-rule claims about:

- `design-rules.sh --file <path>` — the machine-checked Key Design Rules and this project's `[[gates.checks]]`. Its "NOT machine-checked" list is what is left to judge by hand.
- `readability.sh --file <path>` — function length, one class per `.cpp`, member order and access sections.
- `design-rules.sh --all` checks the whole tree.

## Mechanical Edits

Extracts, inlines, renames and multi-site rewrites go through these, never by hand. The analysis agent never runs any of them.

| Edit | Command |
|---|---|
| Lift a selection into its own function | `refactor.encap <file>:<start>-<end> <name>` |
| Dissolve a function into its call sites | `refactor.decap <Class>::<function>` |
| Rename variables | `refactor.replace '<the selection retyped with the new names>'` |
| Structural search and replace | `refactor.rewrite '<pattern>' '<template>' <scope>`, with `--dry-run` first |
| Apply `constexpr` | `refactor.const_exper <path>` |
| Put back the last edit | `refactor.undo` |

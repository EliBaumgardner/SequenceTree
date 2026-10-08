# Ongoing Issues

Problems noticed in passing while working on something else, kept to go back over later or in a new session. Each entry says where the problem is, what it is, how it was confirmed and its status. An entry comes out of this file once it is solved.

## `peekCrossTreeNode` is misnamed

- **Where:** `TraversalLogic::peekCrossTreeNode` in `Source/Audio/TraversalLogic.cpp`, called from `TraversalDispatcher::dispatchCrossTree` and from `Tests/TraversalTests.cpp`.
- **Problem:** it advances the `CrossTree` and `CrossTreeSwitch` counters, the only place they advance, but "peek" says it only inspects. Agreed new name: `advanceCrossTreeCounts`.
- **Blocked:** `refactor.replace` refuses because the rename reaches `Tests/`. `refactor/editing/replace.py:101` checks `SOURCES` where `rewrite.py` and `signature.py` check `EDIT_ROOTS`, so no rename can touch a test file.
- **Status:** open 2026-10-03, waiting on the `replace.py` fix in research-suite.

## Trails on dashed arrows may trace the dash outlines

- **Where:** `CustomLookAndFeel::drawArrow` in `Source/UI/Theme/CustomLookAndFeelArrows.cpp`.
- **Problem:** when `isDashed()` is true, `shaft` is replaced by `createDashedStroke`'s outline before `drawArrowProgress` reads it, so a trail is trimmed along the perimeter of every dash rather than the centre line. Arrows leaving a traversal flag are dashed.
- **Status:** noticed 2026-10-03 while measuring the trail trimming cost, from reading the code only. Check on the canvas whether trails play on dashed arrows and how they look.

## A referenced tree's own state hides the outer prompt

- **Where:** TreeJev `src/traversal/walker.ts`, the `effectiveSubgraph` built in `startEncapsulatorTraversal` and `stepActiveSubtraversal`: `{...graph.globalState, ...subgraph.globalState}`.
- **Problem:** a sub-tree's own `globalState` is merged over the outer walk's state, so a placeholder such as `"prompt": "the user's prompt"` in a referenced tree replaces the real prompt. Every Choice and Noul inside it then classifies the placeholder.
- **Confirmed:** 2026-10-08 live runs of `sequence_tree_turn` → `sequence_tree_change`. Three different code prompts all took the same path until the change tree's `globalState` was emptied (see `Proposals/Implemented/treejev_route_context_and_tree_composition.md`).
- **Status:** worked around in `.treejev/trees/sequence_tree_change.json`. Open in TreeJev: for `treeRef` sub-trees the outer state should win.

## Prompt-time design-rule run makes code turns take about a minute

- **Where:** research-suite `bin/claude-md-hook.sh`, `check=$("$here/design-rules.sh" 2>&1)` on every `code` and `deep` turn.
- **Problem:** it checks every Source file changed vs HEAD (about 95 uncommitted files today), so the UserPromptSubmit hook takes about 65–70 s on code turns against about 1–2 s for TreeJev, close to the hook's 120 s timeout.
- **Confirmed:** 2026-10-08, the full hook timed against `treejev hook claude` alone on the same prompts.
- **Status:** open. Committing the working tree shrinks it. Scoping the prompt-time run would be a research-suite change.

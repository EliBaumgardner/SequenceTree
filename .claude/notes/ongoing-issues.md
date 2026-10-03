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

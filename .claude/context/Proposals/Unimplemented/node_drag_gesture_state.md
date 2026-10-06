# Node Drag Gestures Held in `DragState`

> Status: Draft
> Written 2026-10-04 at 8082e3c. Sources: Reviewed/ProgramAudits/node_controller_and_ui_input.md (What Survives: P2 "Gesture state is held outside `DragState`"; ledger #1–#6)

## Summary

`NodeController` keeps three of its gestures out of its `DragState` enum. A plain node move, a Shift/Ctrl drag that creates a child, and the first move of an arrow-head drag are tracked by `bool isDragStart` together with `draggedNodeTree.isValid()`, and both are re-tested against the modifier keys on every drag event. The project's CLAUDE.md says that one `DragState` holds what a gesture means, and the Key Design Rule on named states asks for an enum here. The fix adds five enum states: `NodePressed`, `MovingNode`, `CreatingChild`, plus `ArrowHeadPressed` split out of today's `MovingArrowHead`. The first-move work (undo transaction, grid) moves onto the transition from pressed to moving. `isDragStart` is deleted, and `draggedNodeTree` stays only as data: which tree is being placed. Because `mouseDown` already resets `dragState` to `Idle`, every gesture now starts clean. The change touches `NodeController.h` and `.cpp` only, about 30 lines, in one step, plus a one-line choice about what a failed child creation leaves behind.

## The Problem

### Mechanism

All line numbers are `Source/Input/NodeController.cpp` at `8082e3c`. The working tree's only change to this file is at `:770` (`createEncapsulator`'s signature), and it touches none of these lines.

1. **Press.** `mouseDown` (`:678`) sets `dragState = Idle`. `handleNodeMouseDown` (`:922`) changes it only for a value edit (`:934`), an arrow-mode Shift-drag (`:948`) or a Shift-drag on a traversal flag (`:962`). Every other left press on a node, which covers both a plain move and a child-creation drag, stays `Idle`. Nothing records that a node was pressed. The gesture rests on `isDragStart` still being `true` from whichever reset ran last.
2. **First move past `dragThreshold`.** `handleNodeMouseDrag` (`:268`) decides what the gesture is from three things: the modifier keys of *this* event, `draggedNodeTree.isValid()` and `isDragStart`:
   - `:296`: no Shift, no Ctrl and an invalid tree → `handleNodeDrag` (`:228`). Its `if (isDragStart)` block (`:230-234`) clears the flag, opens the undo transaction and shows the grid.
   - `:301`: modifiers held and `isDragStart` → clear the flag, then `handleNodeDragStart` (`:339`), which opens the transaction (`:359`), shows the grid (`:361`) and creates the child into `draggedNodeTree` (`:371`).
   - `:307`: the tree is valid → place the child (`snapToGrid`, `checkRootNodeSnap`). Releasing Shift mid-placement still lands here, because `:296` fails on the valid tree.
3. **Arrow-head drag.** `handleCanvasMouseDown` sets `dragState = MovingArrowHead` and `isDragStart = true` on press (`:820-821`), before anything has moved. `handleCanvasMouseDrag` (`:197-211`) calls the same `handleNodeDrag`, so the same `isDragStart` block opens that gesture's transaction and grid. Today `MovingArrowHead` means "pressed or moving", and the bool supplies the difference.
4. **Release.** `mouseUp` (`:454`) tests `draggedNodeTree.isValid()` at `:497` to decide that a child was being placed. The flag is reset in three places: `endDrag` (`:670`), `finishArrowHeadDrag` (`:510`) and `finishDanglingArrowCreation` (`:641`). The last of these never sees the flag cleared, because `:284` returns before any code that clears it.

Taken together, the code holds an implicit state machine:

| Implicit state | Held as | Next drag event does |
|---|---|---|
| node pressed | `Idle`, `isDragStart`, tree invalid | no mods → move (transaction + grid); mods → create child |
| moving node | `Idle`, `!isDragStart`, tree invalid | no mods → move (no new transaction); mods → **nothing** (node freezes while Shift is held) |
| creating child | `Idle`, `!isDragStart`, tree valid | place child, whatever the mods |
| child creation failed (`:353` or `:373` returned) | `Idle`, `!isDragStart`, tree invalid | identical to "moving node": no mods → moves the **pressed** node with no transaction of its own |
| arrow head pressed | `MovingArrowHead`, `isDragStart` | move (transaction + grid) |
| arrow head moving | `MovingArrowHead`, `!isDragStart` | move |

**Concrete input:** select a node and Shift-drag it. The first move creates a child, and every later event places that child. The only record that this is a child-creation drag rather than a move is a `juce::ValueTree` handle being non-null. The reader has to reconstruct this table to know what any one drag event does.

### Evidence

- The review confirmed ledger #1 (the enum has no state for a node move or a child-creation drag) and #2 (branches at `:296`, `:301`, `:307`, `:497`). It corrected #3 and #5: there are three resets, and the arrow-head use at `:230` and `:821` changes the fix.
- Re-read at `8082e3c` for this proposal. Every line above matches. `isDragStart` and `draggedNodeTree` are private and appear nowhere outside `NodeController.cpp` (grep over `Source/`).
- `design-rules.sh --file` on `NodeController.h` and `.cpp` passes the machine checks. It lists "Whether several bools in one class are really one enum state" and "Whether a state named in neither the type nor the field name still has two or more named alternatives and so owes an enum" as NOT machine-checked. Those are exactly this finding.
- `readability.sh --file Source/Input/NodeController.cpp` reports `handleCanvasMouseDown` (`:773`) at 81 lines, limit 80. The plan removes one line from it.
- **Ledger #6 (suspected undo merge), still unverified.** `handleNodeDragStart` returns at `:353` when `canvas.nodeManager.find(parentNodeId)` is null. For a node that is not an encapsulator, `parentNodeId == nodeId` is the pressed component itself, so this needs a collapsed `Encapsulator` whose `memberNodeIds.back()` has no `Node` component. `memberNodeIds` is refreshed through `EncapsulationView::refreshMembership` when members are added or removed (`NodeManager.cpp:169`, `:243`), so I found no edit sequence that reaches it. The `:373` return needs `NodeCreationDispatcher::create` to return an invalid tree. Every `NodeFactory::create*` it calls returns the tree `GraphState` added, so I found no reachable input there either. Both are treated as defensive paths. The plan decides what they leave behind but does not claim to fix an observed bug.

### Why It Matters

- **Design:** CLAUDE.md says "What a gesture means is held in one `DragState` enum, not in a set of booleans; `mouseDown`/`mouseDrag`/`mouseUp` dispatch on it to small named handlers". Three gestures break that, and the named-states rule applies.
- **Robustness:** `dragState` is reset to `Idle` on every `mouseDown` (`:680`), but `isDragStart` is reset only on certain `mouseUp` paths. If a `mouseUp` is ever lost (for example, a component destroyed mid-gesture), the next node move starts with `isDragStart == false`: no undo transaction, so the move merges into the previous undo step, and no grid. Once the state lives in `dragState`, that cannot happen.
- **Readability:** each drag event re-derives the gesture from modifier keys and a tree handle. Once the gesture is named, each branch says what it is.
- Priority **P2**, as the review ranked it. There is no user-visible defect at the reachable inputs.

## Current Design

### API Surface

All of the following are private to `NodeController` unless marked.

| Member | Role | Readers / writers |
|---|---|---|
| `enum class DragState` (`.h:66`) | gesture kind | — |
| `DragState dragState` (`.h:121`) | current gesture | written: `mouseDown :680`, `handleCanvasMouseDown :804 :813 :820 :827`, `beginBoxSelection :914`, `handleNodeMouseDown :934 :948 :962`, `mouseUp :467 :491`, `finishArrowHeadDrag :509`, `finishBoxSelection :530`, `finishDanglingTipDrag :540`, `finishFlagConnection :595`, `finishDanglingArrowCreation :640`; read: `mouseDrag :108 :113`, `handleCanvasMouseDrag :197 :214 :219`, `handleNodeMouseDrag :284 :291`, `mouseUp :456–490`, `handleNodeMouseDown :969` |
| `bool isDragStart` (`.h:137`) | "first move not yet made" | read `:230`, `:301`; written `:231`, `:302`, `:510`, `:641`, `:670`, `:821` |
| `juce::ValueTree draggedNodeTree` (`.h:146`) | the child being placed | written `:371` (`handleNodeDragStart`), `:669` (`endDrag`); state tests `:296`, `:307`, `:397`, `:497`; data reads `:310`, `:377`, `:392`, `:434`, `:498`, `:657` |
| `handleNodeDrag` (public, `.h:53`) | move one node + descendants | callers `:210` (arrow head), `:297` (plain move); no caller outside the class |
| `handleNodeDragStart` (public, `.h:56`) | create the child on first Shift/Ctrl move | caller `:303` only |
| `checkRootNodeSnap` (public, `.h:57`) | ghost a cross-root arrow while placing | caller `:311` only |
| `handleNodeMouseDrag`, `handleCanvasMouseDrag`, `handleNodeMouseDown`, `handleCanvasMouseDown`, `mouseUp`, `endDrag`, `finishArrowHeadDrag`, `finishDanglingArrowCreation` | gesture handlers | inside `NodeController` |

`setArrowMode` (public) is called only from `Titlebar::configureModePane` (`Titlebar.cpp:117`), a mouse click, so arrow mode cannot change during a drag.

### Structure

`NodeController` (`Source/Input/`) is a `juce::MouseListener` that `NodeCanvas` attaches to itself and to every `Node`. It is owned by `PluginEditor` and reached through `ApplicationContext::nodeController`. It composes `SelectionOps` (public member) and `ConnectionOps` (private), and drives `NodeCanvas`'s parts directly: `nodeManager`, `arrowManager`, `hitTester`, `encapsulationView`, `showGrid`/`hideGrid`/`snapPointToGrid`. Graph writes go through `GraphState` and `NodeFactory`/`NodeCreationDispatcher` (`Source/Graph/`, `Source/Input/`) under the `juce::UndoManager` the processor owns. CLAUDE.md assigns `NodeController` "the gesture and nothing else". The gesture state belongs exactly here.

### Data Flow

Everything runs on the message thread. No audio-thread state is involved.

```
mouseDown ─ dragState = Idle ─┬─ canvas → handleCanvasMouseDown ─ (arrow head) MovingArrowHead + isDragStart
                              └─ node   → handleNodeMouseDown   ─ Idle | EditingValue | CreatingDanglingArrow | ConnectingFlag
mouseDrag ─┬─ canvas → handleCanvasMouseDrag ─ MovingArrowHead → handleNodeDrag ─ GraphState::setNodePosition (undo)
           └─ node   → handleNodeMouseDrag   ─ mods/tree/isDragStart → handleNodeDrag | handleNodeDragStart | snapToGrid
mouseUp   ─ finish* / connectDraggedNodeToRoot / triggerSnapForNode ─ endDrag (isDragStart = true, tree = {})
```

A move's `GraphState::setNodePosition` reaches `RTGraphBuilder`'s listener (`NodeMoved` → duration maps) and `NodeCanvasTreeListener`. Neither changes under this plan, because the same calls run in the same order with the same undo grouping.

### Invariants touched

- **Arrow geometry is data / pitch is owned:** moves still go through `GraphState::setNodePosition` inside the gesture's own transaction, so duration and pitch-binding writes still undo as one step. The plan must keep `beginNewTransaction` exactly where the first move happens.
- No real-time path, thread boundary or audio-boundary type is touched.

## Approaches Considered

**A. Pressed and moving states per gesture in `DragState` (recommended).** Add `NodePressed`, `MovingNode` and `CreatingChild`, and split `ArrowHeadPressed` out of `MovingArrowHead`. `handleNodeDrag` opens the transaction and shows the grid when it sees a pressed state, and each caller then sets its moving state. `CreatingChild` is set by `handleNodeDragStart` once the child exists. This is what the review recommended and what CLAUDE.md describes. Cost: about 30 lines in two files. Risk: undo grouping per gesture, checked by hand below. It fits the design rules: no new functions, no bools, no ternaries.

**B. Gesture kind in `DragState`, plus a second `enum class DragPhase { Pressed, Moving }`.** This is fewer enum values, and the phase is shared by every gesture. But it keeps two fields that together say what a gesture means, which contradicts "one `DragState` enum", and most `DragState` values (box select, dangling tip, flag connection) have no meaningful phase. Rejected.

**C. Only `NodeDragging` and `DraggingNewNode` (the original report's fix).** The review refuted it: `MovingArrowHead` loses its first-move transaction and grid, because `handleNodeDrag` can no longer tell its first call. Rejected.

## Plan

### Step 1 — Hold node and arrow-head drags in `DragState`

- **Goal:** every gesture `NodeController` handles is a named `DragState`. `isDragStart` is gone, and `draggedNodeTree` is no longer tested as state.

- **Changes:**
  - `NodeController.h`
    - Add to `DragState`, keeping each gesture's states next to each other: `NodePressed`, `MovingNode`, `CreatingChild` after `Idle`, and `ArrowHeadPressed` immediately before `MovingArrowHead`.
    - Delete `bool isDragStart = true;` (`:137`).
  - `NodeController.cpp`
    - `handleCanvasMouseDrag` (`:197-211`): the guard becomes `if (dragState == DragState::ArrowHeadPressed || dragState == DragState::MovingArrowHead)`. After `handleNodeDrag(...)`, add `dragState = DragState::MovingArrowHead;` as its own group.
    - `handleNodeDrag` (`:230-234`): the block becomes
      ```cpp
      if (dragState == DragState::NodePressed || dragState == DragState::ArrowHeadPressed) {
          undoManager->beginNewTransaction();
          canvas.showGrid();
      }
      ```
      The `isDragStart = false;` line goes. Its callers record the move (above and below).
    - `handleNodeMouseDrag` (`:296-312`): after the existing `CreatingDanglingArrow` and `ConnectingFlag` branches, the three branches become
      ```cpp
      if (dragState == DragState::CreatingChild) {
          const juce::Point<int> cursor { newPosition.xPosition, newPosition.yPosition };

          snapToGrid(undoManager, newPosition, draggedNodeTree);
          checkRootNodeSnap(cursor);
          return;
      }

      if (dragState != DragState::NodePressed && dragState != DragState::MovingNode) {
          return;
      }

      if (e.mods.isShiftDown() || e.mods.isCtrlDown()) {
          if (dragState == DragState::NodePressed) {
              dragState = DragState::Idle;
              handleNodeDragStart(undoManager, &node, nodeId, newPosition, e.mods);
          }
          return;
      }

      handleNodeDrag(undoManager, nodeId, newPosition);

      dragState = DragState::MovingNode;
      ```
      `DragState::Idle` in the Shift branch is the failure state. See *Decisions for the Owner* #1: the alternative is `MovingNode`.
    - `handleNodeDragStart` (`:373-375`): straight after the `if (! draggedNodeTree.isValid()) { return; }` guard, set `dragState = DragState::CreatingChild;` as its own group, before `applySelectedArrowInfo`. The later early returns (`:383-390`) are then already in `CreatingChild`, as the valid tree is today.
    - `checkRootNodeSnap` (`:397`): the guard becomes `if (snapSourceNodeId < 0) {`. Its only caller is now the `CreatingChild` branch, so the tree test is dead.
    - `mouseUp`: `:456` becomes `if (dragState == DragState::ArrowHeadPressed || dragState == DragState::MovingArrowHead) {`, and `:497` becomes `else if (dragState == DragState::CreatingChild) {`.
    - `finishArrowHeadDrag` (`:510`) and `finishDanglingArrowCreation` (`:641`): delete `isDragStart = true;`. The remaining assignments in each group stay aligned.
    - `endDrag` (`:667-676`): replace `isDragStart = true;` with `dragState = DragState::Idle;`. Today `MovingNode`, `CreatingChild` and `NodePressed` survive past `mouseUp` until the next `mouseDown`, while every other finished gesture already returns to `Idle`.
    - `handleCanvasMouseDown` (`:820-821`): `dragState = DragState::ArrowHeadPressed;`, and delete the `isDragStart` line. The function goes from 81 lines to 80, which clears the readability report.
    - `handleNodeMouseDown` (`:947-963`): add a final `else { dragState = DragState::NodePressed; }` to the `if` / `else if` chain. This covers every node press that is not a value edit, an arrow-mode Shift-drag or a flag Shift-drag. Right-button presses land here too, and that is harmless: `handleNodeMouseDrag` returns at `:280` without the left button, and `mouseUp` → `endDrag` returns to `Idle`.

- **Mechanical edits:** none. Every edit is a changed condition or assignment at a single site. No extraction, inlining or rename is involved, and `refactor.rewrite` gains nothing over hand edits at fewer than three similar sites. No function is added or moved, so the hierarchy order is unchanged.

- **Behaviour delta:**

  | Input | Before | After |
  |---|---|---|
  | Plain drag of a node | first move opens a transaction, shows the grid, moves; later moves join it | same |
  | Plain drag, then Shift held mid-move | node freezes while Shift is held, resumes on release | same (`MovingNode` + mods → return) |
  | Shift/Ctrl drag of a node | first move creates the child (transaction, grid); later moves place it; Shift released mid-placement keeps placing | same (`CreatingChild` ignores mods) |
  | Shift drag released over a root | `connectDraggedNodeToRoot` (undo the child, cross-root arrow) | same: `snapTargetRoot` is set only from `CreatingChild` |
  | Shift drag released elsewhere | `triggerSnapForNode` on the child | same, gated on `CreatingChild` |
  | Arrow-head press, release without moving | `finishArrowHeadDrag` (snap animation), no transaction | same (`ArrowHeadPressed` handled by `mouseUp`) |
  | Arrow-head drag | first move opens a transaction and shows the grid | same |
  | Child creation fails (`:353` / `:373`, believed unreachable) then Shift released | the pressed node moves with no transaction of its own; for `:353`, no grid either | **changes** with Decision #1 = `Idle`: nothing moves until release. With `MovingNode`: identical to before |
  | A `mouseUp` lost mid-gesture (not shown reachable) | the next node move has no undo transaction and no grid | **changes:** `mouseDown` resets `dragState`, so the next move opens its own transaction |
  | Value edit, arrow-mode Shift-drag, flag Shift-drag, box select, dangling tip, arrow selection, paint, span, quaver modes | — | unchanged: their states and branches run before any new test |

  Apart from Decision #1 and the lost-`mouseUp` case, no difference is intended.

- **Invariants:** message thread only. Undo grouping is unchanged: `beginNewTransaction` runs at exactly the first move of a node or arrow-head drag (`handleNodeDrag`), and at the child's creation (`handleNodeDragStart :359`). `GraphState::setNodePosition` is still the only position writer, so "arrow geometry is data" and the pitch-binding sync keep their single-transaction undo. No audio-thread type or path is touched.

- **Verification:**
  - Build `SequenceTree_Standalone` in `cmake-build-debug`, then build `SequenceTree_Tests SequenceTree_GraphTests` and run `ctest --output-on-failure`. They do not reach `NodeController`. They confirm the tree still builds and that nothing in graph building moved.
  - `design-rules.sh --file` on `Source/Input/NodeController.h` and `.cpp`; `readability.sh --file Source/Input/NodeController.cpp`. Expect `handleCanvasMouseDown` back at 80 lines and no new reports. Then `refactor.order Source/Input/NodeController.cpp --check`.
  - `grep -n "isDragStart\|draggedNodeTree.isValid" Source/Input/NodeController.cpp` should show only `:373`'s creation-result check.
  - No automated test: CLAUDE.md states that UI behaviour needs a host, and `SequenceTree_GraphTests` builds graphs without a canvas or `NodeController`. Manual check in the standalone, with Cmd+Z after each row to confirm the whole gesture undoes as **one** step and nothing before it:
    1. Drag a node: it moves with its descendants and the grid shows. Undo puts it back.
    2. Drag a node and press Shift mid-move: it freezes, then resumes on release.
    3. Shift-drag a node: a child is created and follows the cursor. Release Shift mid-drag: the child keeps following. Undo removes the child.
    4. Ctrl-drag: an alternative child is created. In Modulator and Traversal Flag modes, Shift-drag creates the matching node type.
    5. Shift-drag a child onto another root: the ghost arrow shows, and the release makes a cross-root arrow with no leftover child.
    6. Shift-drag from the exit member of an expanded encapsulator, and from a collapsed encapsulator: the child joins the encapsulation as before.
    7. Drag an arrow head: the head node moves, the grid shows, and undo restores it. Click an arrow head without moving: the snap animation plays and no undo step is added.
    8. Regression sweep: drag a value editor, Shift-drag in arrow mode (dangling and connected), Shift-drag from a traversal flag to a node, drag a dangling tip, Shift-drag a box selection on the empty canvas, pan the canvas.

- **Rollback:** `git checkout -- Source/Input/NodeController.h Source/Input/NodeController.cpp` (the working tree's `:770` `createEncapsulator` edit lives in the same file, so if it is still uncommitted, revert this step's hunks with `git restore -p` instead).

## Risks

- **A node press that does not land in `NodePressed`.** The node would no longer move. Every left press on a node reaches `handleNodeMouseDown`'s chain unless it returns earlier as a value edit (`:937`) or a right-click toggle (`:942`), and the paint, span and quaver modes return before `handleNodeMouseDown`. Likelihood: low. It would show up as a node that does not move, and manual check 1 catches it.
- **Undo grouping drift.** If a moving state were set before `handleNodeDrag` ran, the transaction would never open, and the move would merge into the previous undo step. The plan sets each moving state *after* the call. Manual checks 1, 3 and 7 undo each gesture.
- **The `NodePressed` → `Idle` assignment before `handleNodeDragStart`.** If `handleNodeDragStart` ever set `CreatingChild` before creating the child, a failed creation would leave `CreatingChild` with an invalid tree, and `snapToGrid` would write to an invalid tree. The plan places the assignment after the `isValid` guard.

## Out of Scope

- **`mouseMove` makes three passes over every arrow** (review P3). The impact is unmeasured. Profile `updateArrowHover` on a canvas with a few hundred arrows before proposing the `SafePointer<Arrow>` hovered-arrow change.
- **`bool arrowMode` next to `nodeControllerMode`** (review question). Arrow mode is "no mode button selected" (`Titlebar.cpp:117`), while `nodeControllerMode` keeps its last value, which Ctrl/Shift child-creation drags in arrow mode still use. Whether arrow mode is one more state of the creation-mode enum is a separate decision about what arrow mode means. It touches `Titlebar`, `NodeCreationMode` and `isNodeCreationModeActive`, so it belongs in its own proposal.
- **`handleCanvasMouseDown` length.** This step brings it to the limit (80). It does not split it.
- `SelectionOps::selectedNodeIds` and `ConnectionOps::resolveOwnership`: the review rejected and refuted these.

## Decisions for the Owner

1. **What a failed child creation leaves behind.** This is the `handleNodeDragStart` return at `:353` or `:373`. Both look unreachable today.
   - **`Idle` (recommended):** the gesture ends, and nothing moves until the mouse is released. This removes the suspected undo merge (ledger #6) by construction.
   - **`MovingNode`:** keeps today's behaviour exactly. Releasing Shift then moves the pressed node with no transaction of its own (and, for `:353`, no grid).
2. **State names.** `NodePressed`, `MovingNode`, `CreatingChild`, `ArrowHeadPressed` (with `MovingArrowHead` kept). They follow the enum's existing `-ing` style (`MovingDanglingTip`, `CreatingDanglingArrow`). Rename any you would call something else, for example `PlacingNewChild` for `CreatingChild`, since after its first move the gesture places the child rather than creates it.

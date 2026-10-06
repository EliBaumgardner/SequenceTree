# Node Controller and UI Input Refactoring

> Reviewed 2026-10-04 at 8082e3c by Claude. Source: Unreviewed/ProgramAudits/node_controller_and_ui_input.md (Antigravity, `agy`, model not recorded).
> Verdict: Major revision

## Advisor Review

### Assessment
The report audits `NodeController`, `SelectionOps` and `ConnectionOps` for design-rule adherence, dense-graph performance and state robustness. Its first finding is real: gesture state is split between `DragState` and two other variables. But it cites the wrong rule, misses a third place that uses the flag, and offers a fix that would break arrow-head dragging. The two performance findings describe the code correctly, but their impact claims were never measured, and in one case the code was misread. The `ConnectionOps` finding is refuted. The fallback it calls "blind" is deliberate: it handles alternative arrows, which are drawn from child to parent. Even in the malformed case it imagines, the result is a guarded no-op, not corrupted state. What the project can use is one P2 design-rule item, which needs a better plan, and one P3 performance note.

The working tree changes `NodeController.cpp` and `SelectionOps.cpp` (the `createEncapsulator` signature only). None of the lines under review are affected, so the verdicts at `8082e3c` also hold for the working tree.

### Claim Ledger
| # | Claim (short, quoted) | Where (current file:line) | Verdict | Evidence / correction |
|---|---|---|---|---|
| 1 | "The `DragState` enum lacks states for normal node dragging or dragging a newly created node" | `Source/Input/NodeController.h:66-75` | Confirmed | The enum's states are `Idle`, `EditingValue`, `MovingArrowHead`, `MovingDanglingTip`, `ArrowSelected`, `CreatingDanglingArrow`, `ConnectingFlag` and `BoxSelecting`. A plain node move runs while `dragState` is still `Idle`. |
| 2 | "`handleNodeMouseDrag` branches … on `isDragStart` … and `draggedNodeTree.isValid()`" | `NodeController.cpp:296`, `:301`, `:307` | Confirmed | The report also missed `:307` and the `mouseUp` branch at `:497`. |
| 3 | "`mouseDown` … does not reset `isDragStart`, relying on `endDrag()` in `mouseUp`" | `NodeController.cpp:678-686`, `:667-676` | Corrected | `isDragStart` is reset in three places, not one: `endDrag` (`:670`), `finishArrowHeadDrag` (`:510`) and `finishDanglingArrowCreation` (`:641`). It is also set at `:821`. Every `mouseUp` path that can follow a node drag reaches one of these resets. The report shows no concrete case where the flag goes out of sync. |
| 4 | "Violates the Key Design Rule 'What a gesture means is held in one DragState enum'" | `.claude/CLAUDE.md`, Directory Responsibilities, `NodeController` | Corrected | The quoted sentence is CLAUDE.md's description of `NodeController`. It is not a Key Design Rule. Two things actually apply: the code contradicts that stated design, and the Key Design Rule on named states ("Use an enum whenever a property has two or more named states") is broken. The substance holds. |
| 5 | "Drop the `isDragStart` boolean entirely" by adding `NodeDragging` / `DraggingNewNode" | `NodeController.cpp:228-245`, `:819-822` | Corrected (recommendation) | `handleNodeDrag` also reads `isDragStart` at `:230`. That is the shared path for both plain node moves and the `MovingArrowHead` drag (`:210`), where the flag means "the first move past the threshold has not happened yet": open the undo transaction and show the grid. With only the two proposed states, the arrow-head drag loses that first-move step. The plan also needs a pressed-but-not-yet-moved state for the arrow head, or a matching split of `MovingArrowHead`. |
| 6 | "Makes gesture routing fragile" | `NodeController.cpp:351-353`, `:296-299` | Unverified | One suspected path, from reading only: `handleNodeDragStart` returns at `:351` when `parentNode == nullptr`. It does so before `beginNewTransaction`, so `isDragStart` is already `false` and `draggedNodeTree` is invalid. If Shift is then released mid-drag, `handleNodeDrag` skips `beginNewTransaction` and the move merges into the previous undo transaction. This needs a node with no `Node` component (a collapsed encapsulator whose last member is missing from `nodeManager`). To settle it, show that this state can be reached on the canvas. |
| 7 | "`updateArrowHover` iterates over all arrows twice" | `NodeController.cpp:64-92`, `Source/UI/Canvas/CanvasHitTester.cpp:56-64` | Corrected | It makes three passes: the flag-proximity loop (`:66`), `CanvasHitTester::arrowNear`, which runs `nearest` over `arrowManager.all()` (`CanvasHitTester.cpp:58`), and the hover loop (`:86`). The report cited `:85`, but the second loop starts at `:86`. |
| 8 | "…call `repaint()` on each arrow" | `NodeController.cpp:86-91` | Refuted | `repaint()` runs only when `hovered` changes (`:87-90`). On a typical move, at most two arrows repaint. |
| 9 | "This will cause UI stutter on dense graphs" | — | Unverified | No measurement was given. Each pass does a pointer comparison per arrow. `distanceToSegment` runs only for arrows that start at a traversal flag and inside `arrowNear`. To settle it, profile `mouseMove` on a canvas with a few hundred arrows. |
| 10 | Store a `hoveredArrow` and repaint just two arrows | — | Judged: valid, P3 | Correct in principle. It must be a `juce::Component::SafePointer<Arrow>`, because `ArrowManager` owns arrows in a `juce::OwnedArray` (`ArrowManager.h:21`) and deletes them. It removes one pass of three; the proximity loop and `arrowNear` stay. The payoff is small unless claim 9 is measured. |
| 11 | "`selectedNodeIds` … iterates over all nodes … checking `node->isSelected`" | `Source/Input/SelectionOps.cpp:87-98` | Confirmed | It iterates `nodeManager.all()`, an `std::unordered_map<int, std::unique_ptr<Node>>` (`NodeManager.h:25`). |
| 12 | "called by `hasSelection`, `deleteSelection`, and `copySelection`, which are frequent operations" | `SelectionOps.cpp:72`, `:194`, `:753` | Corrected | `copySelection` reaches it through `selectionWithEncapsulatedMembers` (`:72`). Every caller is a user command: the selection context menu (`NodeController.cpp:897-902`) and the keyboard delete and copy (`PluginEditor.cpp:149`, `:164`). That means one scan per user action, which is not frequent. |
| 13 | "Maintain a `std::set<int> selectedIds` in `SelectionOps`" | `Source/UI/Node/Node.cpp:134-150` | Rejected (recommendation) | Selection state is `Node::isSelected`. `Node::setSelectVisual` writes it, and is called from `NodeController.cpp:525` and `:985` as well as from `SelectionOps`. A set would be a second source of truth that every writer must keep in step, and node removal and undo would leave stale ids in it. "O(1) selection lookup" is not what any caller needs; each one wants the whole list. |
| 14 | "returns `{ endId, startId }` … without actually querying `endNode`'s children" | `Source/Input/ConnectionOps.cpp:29-43` | Confirmed | It is accurate as a description of the code. |
| 15 | "If … neither node owns the other … will cause `disconnectNodes` to attempt a disconnection on a non-existent child, or corrupt the graph state" | `Source/UI/Canvas/ArrowManager.cpp:96-105`, `Source/Graph/GraphState.cpp:277-292` | Refuted | The fallback is the deliberate case for alternatives. `connectParentToChild` draws an arrow to an `AlternativeNodeData` / `AlternativeModulatorData` from the child to the parent, so in that case the end node is the owner. Every non-dangling arrow is built from a `NodeChildrenIds` entry, and dangling arrows are filtered out before this point (`ConnectionOps.cpp:51`, `NodeController.cpp:782`). Flag-to-target arrows are also stored in `NodeChildrenIds` (`commitFlagConnection` → `connectWithSnapAnimation`). An arrow that neither node owns therefore cannot be built. If one existed, `disconnectNodes` checks `childId.isValid()` (`:289`) and does nothing. |
| 16 | "`GraphState` … handles undo transactions and notifies `RTGraphBuilder`" | `NodeController.cpp:229-232`, `ConnectionOps.cpp:19` | Corrected | The callers open transactions (`beginNewTransaction`), and `GraphState` only takes the `UndoManager*`. `RTGraphBuilder` is its own `ValueTree::Listener` on `nodeMap`, and `GraphState` notifies nobody. |
| 17 | "`SelectionOps` … identifying selected nodes by iterating over `NodeCanvas`'s `nodeManager`" | `SelectionOps.cpp:87-98` | Confirmed | — |

Counts: 5 Confirmed, 6 Corrected (one of them a recommendation), 2 Refuted, 2 Unverified, and 2 recommendations judged (one valid at P3, one rejected).

### Critique
- **Method.** The `NodeController` reading is first-hand: line numbers are close and the branches are named correctly. The `ConnectionOps` finding is pattern matching. It saw a fallback without a check and never asked why the fallback exists. One look at how arrows are built (`ArrowManager::connectParentToChild`) answers that, and the report itself listed it as "Settles it". `disconnectNodes` was never opened, even though the "corruption" claim depends on what it does.
- **Evidence vs conclusion.** Two performance findings are labelled "verified", but only the loops were verified. "Will cause UI stutter" and "frequent operations" are inferences with no measurement and no caller trace. The rubric calls these "suspected", and the impact needed a settling step. "Calls `repaint()` on each arrow" misreads the guarded code at `:87-90`.
- **Omissions.** The arrow-head use of `isDragStart` (`:230`, `:821`) belongs to the report's own subject and changes its recommendation. The report also did not run the tools file's Verification checks on the files it analysed. `readability.sh --file Source/Input/NodeController.cpp` reports `handleCanvasMouseDown` (`:773`) at 81 lines, one over the limit, and `design-rules.sh` lists "several bools that are really one enum state" as not machine-checked, which is exactly the judgement finding 1 needed. See the questions below for two neighbouring state questions in the same class.
- **Proportionality.** The project is a single-developer canvas editor. Sets and cached pointers to save a few hundred pointer comparisons per mouse move are not worth their invariants without a measurement. Ranking the `ConnectionOps` item P1 ("broken invariants") was not supported.
- **Fit with the project's rules.** The report treats a line from CLAUDE.md's architecture description as a Key Design Rule. It should cite the description as stated design and the enum rule as the rule. The selection-set recommendation contradicts the existing design, in which selection state lives on `Node`.

### What Survives
- **P2 — Gesture state is held outside `DragState`.** `NodeController.h:137` (`isDragStart`) and `:146` (`draggedNodeTree` validity used as "creating a child"). They are read and written at `NodeController.cpp:230`, `:296`, `:301-302`, `:307`, `:497`, `:510`, `:641`, `:670` and `:821`. A plain node move, a Shift/Ctrl child-creation drag and the first move of an arrow-head drag are all named states that live in a bool and a tree handle rather than in the enum. CLAUDE.md states that one `DragState` holds what a gesture means, and the Key Design Rule on named states applies. The fix must cover both users of `handleNodeDrag`: plain node moves and `MovingArrowHead`. One undo-merge path is suspected but not shown reachable (ledger #6).
- **P3 — `mouseMove` makes three passes over every arrow.** `NodeController.cpp:66`, `CanvasHitTester.cpp:58`, `NodeController.cpp:86`. It is only worth acting on if profiling a dense canvas shows a cost. Holding the hovered arrow in a `SafePointer` removes one pass.

### Questions for the Author
- Re-derive the `DragState` plan to cover both `handleNodeDrag` callers. Name the full state set, each transition with its line, and where the undo transaction and `showGrid` move to.
- Show whether `handleNodeDragStart` can reach `:351` with `parentNode == nullptr` from a real edit sequence (ledger #6).
- Measure claim 9 before re-raising it, with a canvas size, a timing and a tool.
- Before calling a fallback unchecked, open the code that creates its inputs and the function it falls through to.
- Run `design-rules.sh` and `readability.sh` on every file a design-rule finding names, and report their output with the finding.
- `NodeController` holds `bool arrowMode` (`NodeController.h:123`) next to the `nodeControllerMode` enum (`:43`), and `isNodeCreationModeActive` is `!isArrowMode()` (`NodeController.cpp:909`). Investigate whether arrow mode is one more state of the creation-mode enum, and present the result for the owner to decide rather than as a defect.

## Corrected Report

# Node Controller and UI Input Refactoring

> Type: Program audit
> Written 2026-10-04 at 8082e3c. Scope: Analyzes the UI gesture routing in `NodeController` and the selection and connection logic in `Source/Input/`, specifically checking for design rule adherence, performance on dense graphs, and state robustness.
> Answers: None

## Summary
- [P2] `isDragStart` and `draggedNodeTree` hold gesture state outside the `DragState` enum: `Source/Input/NodeController.h:137`, `:146`
- [P3] `updateArrowHover` makes three passes over all arrows on every `mouseMove`: `Source/Input/NodeController.cpp:64`
- ~~[P2] `SelectionOps::selectedNodeIds` uses O(N) iteration~~ — downgraded: one scan per user command, recommendation rejected
- ~~[P1] `ConnectionOps::resolveOwnership` blindly falls back~~ — refuted

> **Correction:** priorities P1/P2/P2/P1 → P2/P3/none/none. See the claim ledger.

## How It Works Now
- **NodeController** handles all canvas mouse events on the message thread. It dispatches gesture logic based on a `DragState` enum, though it also relies on `isDragStart` and `draggedNodeTree.isValid()` to infer normal dragging, node creation and the first move of an arrow-head drag.
- **SelectionOps** manages the copy/paste/delete logic, identifying selected nodes by iterating over `NodeCanvas`'s `nodeManager` to check `node->isSelected`.
- **ConnectionOps** manages connecting and disconnecting nodes, resolving which node owns an arrow by inspecting the `GraphState`'s `ValueTree`. An arrow is owned by its start node, except for an arrow to an alternative, which `ArrowManager::connectParentToChild` draws from the child to the parent.
- All mutation happens on the message thread. Callers open the undo transaction and pass the `UndoManager` to `GraphState`, which writes the tree. `RTGraphBuilder` hears the change through its own `ValueTree::Listener` and publishes through `AudioSnapshotPublisher`.

> **Correction:** "`GraphState` … handles undo transactions and notifies `RTGraphBuilder`" → callers open transactions, and `RTGraphBuilder` listens to the tree itself. The alternative-arrow direction was added because finding 4 depends on it.

## Findings

### [P2] `isDragStart` and `draggedNodeTree` hold gesture state outside `DragState`
- Where: `Source/Input/NodeController.h:137`, `:146`; `NodeController.cpp:230`, `:296`, `:301`, `:307`, `:497`, `:510`, `:641`, `:670`, `:821` (at 8082e3c)
- Kind: code fact
- Confidence: verified
- What happens: `DragState` has no state for moving a node or for dragging a newly created child. A plain move runs under `Idle`. `handleNodeMouseDrag` treats `draggedNodeTree.isValid()` as "a child-creation drag is in progress" and `isDragStart` as "the first move past the threshold has not happened yet". `handleNodeDrag` (`:230`) reads the same flag for both a plain move and a `MovingArrowHead` drag, to open the undo transaction and show the grid once. The flag is reset by `endDrag` (`:670`), `finishArrowHeadDrag` (`:510`) and `finishDanglingArrowCreation` (`:641`), and set at `:821`.
- Why it matters: CLAUDE.md describes `NodeController` as holding what a gesture means in one `DragState` enum, and the Key Design Rule on named states asks for an enum here.

> **Correction:** "Violates the Key Design Rule 'What a gesture means is held in one DragState enum'" → that sentence is CLAUDE.md's stated design for `NodeController`, not a Key Design Rule; the rule that applies is the enum rule for named states. "Relying on `endDrag()`" → three resets, listed above. The arrow-head use at `:230`/`:821` was missing.

- Evidence: as listed under Where.
- Settles it: N/A for the code fact.

> **Unverified:** "fragile". The one suspected desync is `handleNodeDragStart` returning at `:351` before `beginNewTransaction`, followed by releasing Shift mid-drag, which would merge the move into the previous undo transaction. Show that `parentNode == nullptr` is reachable from an edit sequence.

- Recommendation: Represent each gesture as an enum state, covering a plain node move, a child-creation drag and the arrow-head drag, each with a pressed state and a moving state, so that the first-move work (transaction, grid, child creation) happens on the pressed-to-moving transition. Then remove `isDragStart` and stop using `draggedNodeTree.isValid()` as a state test.

> **Correction:** "Add `NodeDragging` and `DraggingNewNode` … drop `isDragStart`" → with only those two states, `MovingArrowHead` loses its first-move transaction and grid. The set of states must cover `handleNodeDrag`'s second caller.

- Cost: `Source/Input/NodeController.h` and `.cpp`. Low to moderate risk, because every gesture's undo grouping has to be checked again on the canvas.

### [P3] `updateArrowHover` makes three passes over all arrows on every `mouseMove`
- Where: `Source/Input/NodeController.cpp:64` — `NodeController::updateArrowHover` (at 8082e3c)
- Kind: code fact (passes); inference (impact)
- Confidence: verified (passes); suspected (impact)
- What happens: on every mouse move over the canvas, it loops over `canvas.arrowManager.all()` for flag proximity (`:66`), calls `canvas.hitTester.arrowNear()`, which runs `nearest` over every arrow (`CanvasHitTester.cpp:58`), and loops over every arrow again (`:86`), repainting only the arrows whose `hovered` changes.

> **Correction:** "iterates over all arrows twice … call `repaint()` on each arrow" → three passes; `repaint()` is guarded by a state change at `:87-90`.

> **Unverified:** "This will cause UI stutter on dense graphs". Profile `mouseMove` on a canvas with a few hundred arrows.

- Recommendation: hold the hovered arrow in a `juce::Component::SafePointer<Arrow>` and update only the old and new arrows. This is worth doing only if profiling shows a cost.
- Cost: `NodeController.h`/`.cpp`. Low risk; removes one pass of three.

### `SelectionOps::selectedNodeIds` scans all nodes
- Where: `Source/Input/SelectionOps.cpp:87` (at 8082e3c)
- Kind: code fact
- Confidence: verified
- What happens: it iterates `canvas->nodeManager.all()` checking `node->isSelected`. It is reached from `hasSelection` (`:753`), `deleteSelection` (`:194`) and `copySelection` through `selectionWithEncapsulatedMembers` (`:72`).

> **Correction:** "which are frequent operations" → each caller is a user command (context menu `NodeController.cpp:897-902`; keyboard `PluginEditor.cpp:149`, `:164`), so this is one scan per action.

> **Refuted:** recommendation "Maintain a `std::set<int> selectedIds`". `Node::isSelected` is the selection state, written by `Node::setSelectVisual` from `NodeController.cpp:525` and `:985` as well as `SelectionOps`. A set would be a second source of truth, and removal and undo would leave it stale.

### `ConnectionOps::resolveOwnership` falls back to reverse ownership

> **Refuted:** the fallback is the deliberate case for alternatives, which `ArrowManager::connectParentToChild` (`ArrowManager.cpp:96-105`) draws from child to parent. Every non-dangling arrow is built from a `NodeChildrenIds` entry, so an arrow that neither node owns cannot exist. If one did, `GraphState::disconnectNodes` checks the child at `GraphState.cpp:289` and does nothing.

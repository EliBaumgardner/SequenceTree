# NodeController Decomposition Analysis

> Type: Program audit
> Written 2026-10-04 at 8082e3c. Scope: Analyzes `NodeController` to determine if its 1000-line size warrants decomposition into smaller classes, evaluates its structural cohesion, and addresses the open questions regarding gesture state and `arrowMode`.
> Answers: "Show whether `handleNodeDragStart` can reach `:351` with `parentNode == nullptr` from a real edit sequence", "Run design-rules.sh and readability.sh on every file a design-rule finding names", and "Investigate whether arrow mode is one more state of the creation-mode enum". (Note: The claim about `updateArrowHover` causing stutter on dense graphs is downgraded as a likely non-issue given C++ speeds, and `ConnectionOps` fallback was refuted in the previous review.)

## Summary
- [P2] `NodeController` is cohesive as a gesture state machine and should not be decomposed; splitting it would fragment `DragState` across multiple files and force an artificial `GestureState` object.
- [P3] `NodeController::handleCanvasMouseDown` is 81 lines long (limit 80).
- [P3] `arrowMode` is functionally the "none selected" state of the `nodeControllerMode` toggles, acting as a default input mode.

## How It Works Now
- **NodeController** handles all canvas mouse events on the message thread. It tracks what a gesture means using the `DragState` enum and passes the actual graph mutation logic off to `SelectionOps`, `ConnectionOps`, `NodeCreationDispatcher`, and `GraphState`. 
- **Decomposition Check:** `NodeController` currently sits at ~1000 lines because it handles numerous hit tests (labels, dangling tips, arrow heads) and modifier key branches (`Shift`, `Ctrl`, `Right-Click`). It does *not* contain domain logic. Decomposing it into something like `NodeDragHandler` and `ArrowDragHandler` would require extracting `DragState`, `isDragStart`, and the various intermediate coordinates into a shared `GestureContext` struct that both handlers would mutate. The design rules state: "Don't recommend enterprise patterns ... unless a concrete, present problem calls for them." The perceived complexity of `NodeController` comes from its internal state management (as noted in the prior reviewed report regarding `isDragStart` and `draggedNodeTree`), not from a lack of class-level decomposition.

## Findings

### [P2] `NodeController` should not be decomposed into smaller classes
- Where: `Source/Input/NodeController.h:38` and `Source/Input/NodeController.cpp:26` (at 8082e3c)
- Kind: inference
- Confidence: verified
- What happens: The user requested an investigation into whether `NodeController` should be decomposed. The class maps UI gestures to the data model. If it were split into multiple classes (e.g., separating arrow gestures from node gestures), the unified `DragState` enum and tracking variables (like `isDragStart`, `dragParentCenter`, etc.) would have to be shared among them. 
- Why it matters: Splitting a highly cohesive gesture state machine into multiple classes creates artificial abstractions and violates the project design rules against enterprise patterns.
- Evidence: `NodeController` already delegates data logic to `SelectionOps`, `ConnectionOps`, and `NodeFactory`. It strictly owns gesture tracking. 
- Settles it: N/A
- Recommendation: Do not decompose `NodeController`. Instead, resolve the complexity by consolidating the gesture state into the `DragState` enum (as proposed in the reviewed finding `[P2] isDragStart and draggedNodeTree hold gesture state outside DragState`).
- Cost: 0 files touched. Saves the risk of fragmenting a unified state machine.

### [P3] `NodeController::handleCanvasMouseDown` exceeds 80 lines
- Where: `Source/Input/NodeController.cpp:781` — `NodeController::handleCanvasMouseDown` (at 8082e3c)
- Kind: code fact
- Confidence: verified
- What happens: The function contains 81 lines of hit tests, modifier checks, and dispatch logic for clicking the canvas.
- Why it matters: Violates the project's readability limit of 80 lines per function.
- Evidence: `readability.sh --file Source/Input/NodeController.cpp` reports: `NodeController::handleCanvasMouseDown is 81 lines (limit 80)`.
- Settles it: N/A
- Recommendation: Lift one of the distinct modifier branches (e.g., the right-click menu logic or the shift-left-click node creation) into its own function.
- Cost: `Source/Input/NodeController.h` and `.cpp`. Low risk; improves readability.

### [P3] `arrowMode` is functionally the unselected state of `NodeCreationMode`
- Where: `Source/UI/Bars/Titlebar.cpp:117` and `Source/Input/NodeController.h:43` (at 8082e3c)
- Kind: code fact
- Confidence: verified
- What happens: The `Titlebar` contains a `ButtonPane` with `Selection::ExclusiveOrNone` for the creation modes (Node, Modulator). When a button is selected, `NodeController::nodeControllerMode` is updated. When no button is selected (`selected == nullptr`), it calls `applicationContext.nodeController->setArrowMode(true)`. 
- Why it matters: This answers the reviewer's question: "Investigate whether arrow mode is one more state of the creation-mode enum". The UI treats "arrow mode" (normal interaction) as the default fallback when no creation mode is toggled.
- Evidence: `Titlebar.cpp:117` directly assigns `setArrowMode(selected == nullptr)`. 
- Settles it: N/A
- Recommendation: For the owner to decide: You could merge `arrowMode` and `nodeControllerMode` into a single `enum class InteractionMode { Arrow, CreateNode, CreateModulator, CreateTraversalFlag }`. This would eliminate the `bool arrowMode` completely.
- Cost: `Source/Input/NodeController.h/.cpp` and `Titlebar.cpp`. Low risk; conceptually cleaner.

### [P3] `parentNode == nullptr` is unreachable in `handleNodeDragStart`
- Where: `Source/Input/NodeController.cpp:351` (at 8082e3c)
- Kind: inference
- Confidence: traced
- What happens: This answers the reviewer's question: "Show whether `handleNodeDragStart` can reach `:351` with `parentNode == nullptr` from a real edit sequence." `parentNodeId` is set to either the dragged node's ID or the last member of a collapsed encapsulator. For the user to drag a node, its component must exist in `NodeCanvas`, which means `nodeManager.find()` will find it. If it's an encapsulator, its members are guaranteed to exist in the canvas because `NodeManager` creates components for all valid nodes in the tree.
- Why it matters: Settles the reviewer's unverified claim that releasing Shift mid-drag could cause a desync due to an early return at `:351`. 
- Evidence: `nodeManager.find(parentNodeId)` will not return `nullptr` for a component that just received a UI mouse event.
- Settles it: N/A
- Recommendation: No action needed. The code is safe.
- Cost: None.

# Root Arrows Owned by Alternatives and Modulators

> Status: Draft
> Written 2026-09-25 at 7f59922. Sources: Reviewed/ProgramAudits/software_architecture.md (What Survives: P1 "A Traversal arrow on the active alternative never fires…", P2 "Modulators can own cross-root and Traversal arrows, but the modulator walk honours neither"; ledger rows 7–10)

## Summary
The same root cause produces two findings. Arrows into another tree's root are checked on some owners and not on others. The primary walk checks **Traversal arrows** only on `primary.target` (`advance`, `peekNextTarget`). It checks **cross root tree connections** on both the target and the voiced alternative (`peekCrossTreeNode`). The modulator walk checks neither, although the canvas lets a modulator own both kinds. The recommended fix for alternatives is small: test the voiced alternative's Traversal arrows in `advance` and `peekNextTarget`, and source the trail from the alternative in `pushNote`. It is 3 steps. For modulators the fix depends on what such an arrow should mean, so it waits on the owner. The plan carries both options.

## The Problem
### Mechanism
In the user's terms, as recorded in the arrow-vocabulary memory, three connections lead into another tree's root: a **traversal arrow** (`ArrowType::Traversal` → `RTConnection::isTreeJump`), a **cross root tree connection** (`CrossRootTree`, or a bare `Node` → `isCrossRoot`), and a **regular connection that steps onto the tree** (`StepIntoTree`, which is neither flag). `RTGraphBuilder::classifyRootConnection` (`Source/Graph/RTGraphBuilder.cpp:65-89`) sets these flags from the arrow type whatever node owns the arrow.

**(a) Alternatives.**
1. When host `H` is entered with alternative `A` voiced, `primary.target = H` and `primary.alternativeTarget = A`. `advanceAlternative` sets this at `TraversalLogic.cpp:300`.
2. When `H` is left, `advance` computes `jumpCount = Count[H] + 1` and calls `selectTreeJumpChild(nodes, *targetNode, jumpCount)` (`:353-354`). `selectTreeJumpChild` (`:96-125`) iterates **`H.connections`** only, so `A`'s Traversal arrow to root `B` is never examined.
3. `peekNextTarget` (`:405-436`) likewise checks `H` only (`:412`), so the trail does not point at `B` either.
4. `peekCrossTreeNode`, by contrast, runs `scanHost(primary.target)` and then `scanHost(primary.alternativeTarget)` (`:507-510`). `TraversalDispatcher::dispatchCrossTree` already handles an alternative-owned connection. It reads the duration from `altNode->findConnection` and sets `progressSourceId` to the alternative (`TraversalDispatcher.cpp:403-412`). An alternative's cross root tree connection therefore fires, and its traversal arrow does not.
5. `advanceAlternative`'s own `selectNextChild` uses `isAlternativeChild` (`:11-13`). `RuleContext::eligibleChild` rejects tree-jump connections unless `allowTreeJumpChildren` is set (`Source/Audio/TraversalRule.cpp:9-13`), so the arrow is never taken as a chain step either. The connection is inert.

**Exposing input:** root `1` → host `2` (alternative chain `10` → `11`, as in `alternativeShape`). Give alternative `10` a Traversal arrow to root `5` → `6`, with root `5` at count limit 1. Scratch run at HEAD: `1 2 1 2 10 1 2 11 1 2 1 2 10 1 …`. The arrow never fires. With the Step 1 change: `1 2 1 2 10 5 6 5 6 5 6 5`. The walker relocates when it leaves `2` while `10` is voiced.

**(b) Modulators.**
1. `ModulatorWalk::decide` (`:142-209`) selects with `isModulatorChild` (`:7-9`: `Modulator`, `ModulatorRoot`). It never calls `selectTreeJumpChild`, and `RootNode` is not a modulator child, so a modulator's Traversal arrow and its step-into-tree arrow are both inert.
2. `peekCrossTreeNode` scans `primary.*` only. `dispatchCrossTree` also returns early unless the node played is a `Node` or `RootNode` (`TraversalDispatcher.cpp:372-374`), so a modulator's cross root tree connection is inert as well.
3. The canvas creates all three regardless. `NodeController::checkRootNodeSnap` / `connectDraggedNodeToRoot` (`Source/Input/NodeController.cpp:305-324`, `:970-990`) and `connectDanglingToTarget` (`:291-303`) do not filter the source node's type. `ConnectionOps::canBeTraversalArrow` (`Source/Input/ConnectionOps.cpp:73-82`) and `applySelectedArrowInfo` (`:142-167`) check only that the child is another tree's root.

### Evidence
- Review ledger rows 7–10, re-read at HEAD. Every line cited above matches 7f59922.
- The scratch run in `/tmp/st_alt` (driver `case.cpp`, JUCE-free core compiled directly) gives the two sequences in (a).
- `Tests/TraversalTests.cpp:398-430` covers a host-owned traversal arrow in the primary walk only. The parity case (`:535-594`) has no root-arrow shape apart from `stepIntoTreeShape`. Mirroring that shape gives the modulator walk a `ModulatorRoot` child, not a foreign `RootNode`, so it passes without testing (b).

### Why It Matters
- (a) A user who draws a traversal arrow from an alternative sees the arrow and hears nothing. The same gesture with a cross root tree connection works. This breaks "every walk is the same traversal" inside the primary walk. **P1.**
- (b) A modulator can be given an arrow that looks connected and does nothing. That is a parity gap, or a UI affordance that should not exist. **P2.**

## Current Design
### API Surface
- `TraversalLogic::advance` (`.h:142`, `.cpp:325-403`). Caller: `stepActive` `:704`, reached only through `handleNodeEvent` (`TraversalDispatcher.cpp:725`).
- `TraversalLogic::peekNextTarget` (`.h:144`, `.cpp:405-436`). Callers: `TraversalDispatcher::pushNote` `:198`, and the test at `Tests/TraversalTests.cpp:304`.
- `TraversalLogic::selectTreeJumpChild` (private, `.h:163`, `.cpp:96-125`). Callers: `advance` `:354`, `peekNextTarget` `:412`.
- `TraversalLogic::peekCrossTreeNode` (`.cpp:438-511`). Caller: `TraversalDispatcher::dispatchCrossTree` `:378`.
- `TraversalLogic::handleTreeJump` (`.cpp:666-700`), consuming `pendingJumpTargetId`.
- `TraversalDispatcher::pushNote` (`:166-330`): `nextTarget` feeds `resolveDuration` (`:254` for a voiced alternative) and `dispatchPrimaryArrow(node, nextTarget, …)` (`:313`).
- `TraversalDispatcher::dispatchPrimaryArrow` (`:332-342`) → `AudioUIBridge::pushProgress(parentId, childId, …)`. On the canvas, `AudioCommandDrainer` looks up `parentNode->nodeArrows.equal_range(childId)` (`Source/UI/Canvas/AudioCommandDrainer.cpp:97-116`), so a progress command for a parent→child pair with no arrow draws nothing.
- `ModulatorWalk::decide` (`.cpp:142-209`). Callers: `decideNextModulator` `:515`, and `TraversalDispatcher.cpp:275`.
- `ConnectionOps::canBeTraversalArrow`, `setArrowType`, `applySelectedArrowInfo`, `connectsToOtherTreeRoot` (`Source/Input/ConnectionOps.cpp:62-167`). Callers: `NodeController.cpp:107` (menu enablement), `:142` (toggle), and the connect paths.

### Structure
- `TraversalLogic` (`Source/Audio/`) is graph walking only. `TraversalDispatcher` (`Source/Audio/`) turns steps into notes and UI commands.
- `ConnectionOps` (`Source/Input/`) owns arrow creation and typing. `RTGraphBuilder` (`Source/Graph/`) translates arrow types into `RTConnection` flags.

### Data Flow
```
canvas gesture ─► ConnectionOps (ArrowType on the connection tree)
              ─► RTGraphBuilder listener ─► classifyRootConnection ─► isTreeJump / isCrossRoot
              ─► AudioSnapshotPublisher::publishGraph ─► NodeMap on the audio thread
audio: handleExpiredNote ─► handleNodeEvent ─► advance ─► pendingJumpTargetId ─► handleTreeJump
       pushNote ─► peekNextTarget ─► dispatchPrimaryArrow ─► AudioUIBridge FIFO ─► AudioCommandDrainer
       pushNote ─► dispatchCrossTree ─► peekCrossTreeNode ─► startCrossTreeTraversal
```

## Approaches Considered
**For (a):**
1. **Check the voiced alternative's Traversal arrows in `advance` and `peekNextTarget`, after the host's own.** If the host has no jump due, run `selectTreeJumpChild(nodes, *alternative, Count[alternative] + 1)`. The jump then goes through the existing `Jump` path, so `Count[H]` advances and `SwitchCandidate[H]` resets exactly as for a host-owned jump. The change is small, and it mirrors `peekCrossTreeNode`'s scan order. **Recommended.**
2. **Put the check in `advanceAlternative`.** Rejected in review (row 10): `advanceAlternative` picks the voicing and does not step.
3. **Treat host and alternative jump candidates as one pool** (the largest due count limit wins). This is more uniform, but the two owners use different counts, and ties need a rule. See *Decisions*.

The count the alternative's arrow is chosen by is a real choice. See *Decisions*. The recommendation is **the alternative's own `Count`**. That count advances each time the chain leaves the alternative (`:287`), so a limit of N fires on the alternative's Nth voicing. The host's count would tie the alternative's arrow to how often the host has been visited while other alternatives were voiced.

**For (b):** the options are in *Decisions for the Owner*. Each is written out as a conditional Step 4.

## Plan

### Step 1 — Honour a voiced alternative's traversal arrow in the step
- **Goal:** `advance` jumps on an alternative-owned Traversal arrow.
- **Changes:** `Source/Audio/TraversalLogic.cpp`, `advance` (`:353-363`). `jumpTargetId` stops being `const`. When it is `-1` and `nodes.find(primary.alternativeTarget)` is non-null, set it from `selectTreeJumpChild(nodes, *voicedAlternative, nodeState.get(NodeStateSlot::Count, primary.alternativeTarget) + 1)`. The existing `if (jumpTargetId != -1)` block is unchanged. Sketch, which is the version run in the scratch copy:
  ```cpp
  int jumpTargetId = selectTreeJumpChild(nodes, *targetNode, jumpCount);

  const RTNode* const voicedAlternative = nodes.find(primary.alternativeTarget);

  if (jumpTargetId == -1 && voicedAlternative != nullptr) {
      const int alternativeJumpCount = nodeState.get(NodeStateSlot::Count, primary.alternativeTarget) + 1;

      jumpTargetId = selectTreeJumpChild(nodes, *voicedAlternative, alternativeJumpCount);
  }
  ```
  `Count[A]` is not incremented on the jump. The chain increments it the next time it leaves `A` (`:287`), and incrementing here too would count one voicing twice.
- **Refactor commands:** none.
- **Behaviour delta:**
  - A host with no voiced alternative: unchanged.
  - A voiced alternative with no Traversal arrow: unchanged.
  - A voiced alternative with a due Traversal arrow: the walker relocates to that root (`JumpedToTree`, `rootId` reassigned, `leftAlternativeId = A` via `alternativeLast` at `:681`). Before, nothing happened.
  - A host jump and an alternative jump both due: the host's wins (see *Decisions* 2).
  - One consequence to call out: while `A` is held by its switch count (`:273-275`), `Count[A]` does not move. A due arrow therefore stays due across the held voicings. Since a jump relocates the walker for good, this only matters if the walker returns to `H`.
- **Real-time safety:** one `NodeMap::find` (a binary search over a vector) and one `selectTreeJumpChild` call, which iterates connections by const reference. No allocation, lock or throw.
- **Verification:**
  - Add `TEST_CASE("a traversal arrow on the voiced alternative relocates the walker", "[traversal]")` to `Tests/TraversalTests.cpp`: `alternativeShape()` plus an `isTreeJump` connection from `10` to root `5` → `6`, with `expected { 1, 2, 1, 2, 10, 5, 6, 5, 6, 5, 6, 5 }` over 10 steps, the sequence from the scratch run.
  - `SequenceTree_Tests` and `SequenceTree_GraphTests` pass.
  - No parity section can be added until (b) is decided, because the modulator walk has no jump mechanic.
  - Manual check in the standalone: draw a traversal arrow from an alternative to a second root. Look for the walker moving to the second tree's highlight on the alternative's voicing.
- **Rollback:** revert the hunk.

### Step 2 — Keep the peek in step with the step
- **Goal:** `peekNextTarget` reports the jump target when an alternative's arrow is due, so the trail and duration lookups see it. This is the same parity the peek test at `:268` guards.
- **Changes:** `peekNextTarget` (`:405-421`) gets the same alternative check after the host's `selectTreeJumpChild`, also using `Count[alternativeTarget] + 1`, and returns the found jump node. The function stays `const`.
- **Behaviour delta:** when an alternative jump is due, `pushNote`'s `nextTarget` becomes root `B` instead of the host's next child. `resolveDuration(*alternativeNode, …)` (`:254`) takes the `node.isAlternativeNode` branch and uses `alternativeArrowDuration` either way, so **duration is unchanged**. Only the trail target changes.
- **Real-time safety:** as in Step 1, and const.
- **Verification:** extend the peek test's shape list (`:293`) with Step 1's shape, so peeking still leaves the walk unchanged. Run `ctest`.
- **Rollback:** revert the hunk.

### Step 3 — Draw the jump trail from the alternative
- **Goal:** the trail runs along the arrow the user drew. Today `dispatchPrimaryArrow(node, nextTarget, …)` (`TraversalDispatcher.cpp:313`) would push host→`B`, which has no arrow, so nothing is drawn (`AudioCommandDrainer.cpp:106`).
- **Changes:** `pushNote`, at the `:313` call. When `alternativeNode != nullptr`, `nextTarget != nullptr` and `alternativeNode->findConnection(nextTarget->nodeID)` is a tree jump, pass `*alternativeNode` as the progress source instead of `node`. This mirrors `dispatchCrossTree`'s `progressSourceId` (`:410`). It is written inline at the call site with a local `const RTNode* progressSource`, with no new function.
- **Behaviour delta:** UI only. The trail for an alternative-owned jump now animates, and nothing else changes.
- **Real-time safety:** one linear `findConnection`, no allocation. The command goes through the existing `pushProgress` FIFO.
- **Verification:** build `SequenceTree_Standalone`, then check manually that the trail animates along the alternative's arrow for the note's duration.
- **Rollback:** revert the hunk.

### Step 4 — Modulators (conditional on Decision 1)
- **If 1(a), forbid:**
  - `ConnectionOps::connectsToOtherTreeRoot` (`ConnectionOps.cpp:62-71`) also returns `false` when the parent's tree type is `ValueTreeIdentifiers::ModulatorData`, `ModulatorRootData` or `AlternativeModulatorData` (`ValueTreeIdentifiers.h:17-19`). That one change makes `canBeTraversalArrow` false, which also greys out the "traversal arrow" menu item (`NodeController.cpp:107-108`), and stops `applySelectedArrowInfo` from assigning a root arrow type.
  - `NodeController::checkRootNodeSnap` (`:970`) and `findDanglingSnapTarget` (`:258-277`) skip root targets when the source is a modulator type, so the gesture offers no ghost.
  - Existing saved graphs keep their inert arrows. Whether to strip them on load is Decision 1c.
  - Verification: `SequenceTree_GraphTests` gets a case that builds a modulator through `NodeFactory` and asserts `canBeTraversalArrow` is false. Manual check: dragging a modulator onto a root no longer snaps.
- **If 1(b), define (cross-root spawn only):**
  - `peekCrossTreeNode` also scans `mod.walker.target` and `mod.walker.alternativeTarget`. This needs a walk-kind parameter or a second scan call at the dispatcher, and whichever it is gets asked about before writing, since it is a new function shape.
  - `dispatchCrossTree` is called for the modulator node played in `dispatchModulator`.
  - The `CrossTree` counters are keyed by the child root and shared across owners, as they already are between host and alternative.
  - Traversal arrows and step-into-tree arrows stay forbidden as in 1(a), because a modulator walk relocated onto a primary tree has no modulator children to walk.
  - Add a parity section: the cross-root test (`:432`) run with the host mirrored as a modulator.
- In both branches, check real-time safety along the same lines as Steps 1–3, and roll back by reverting the hunk.

## Risks
- **The jump fires more often than the user expects while the alternative is held** (see Step 1's delta). Low impact, because a jump is a one-way relocation. Covered by Decision 3.
- **Existing graphs change sound.** Any saved graph with an alternative-owned traversal arrow starts jumping. This is intended.
- **Step 3 picks the wrong source** if the host and the alternative both own an arrow to `B`. Step 1 prefers the host's jump, so Step 3 must check the host's connection first and fall back to the alternative's. The sketch does this by testing `node.findConnection(nextTarget->nodeID)` first.

## Out of Scope
- **Step-into-tree arrows owned by an alternative.** The alternative chain selects with `isAlternativeChild`, so these are inert too. The reviewed report does not cover this, so it needs its own review before a proposal.
- **`getTargetNode` / `getRootNode` hygiene.** See `traversal_target_lookup_hygiene.md`.
- **The `SwitchCandidate` collision in `advanceAlternative`.** See `alternative_switch_candidate_collision.md`. It is independent, and either proposal can land first.

## Decisions for the Owner
1. **What should an arrow from a modulator into another tree's root do?**
   - (a) Forbid it in `ConnectionOps` / `NodeController`.
   - (b) Define the cross root tree connection as "spawn root B's traversals when this modulator plays", and forbid the other two kinds.
   - (c) If you choose (a), should existing inert arrows be removed on load, or left in place?
   
   Recommendation: **(a)**, with existing arrows left in place. The modulator walk plays values over a host, not trees, so neither relocating it nor spawning from it has an obvious meaning.
2. **When the host and its voiced alternative both have a traversal arrow due, which wins?**
   - The host's, as the plan has it.
   - The alternative's.
   - Whichever has the larger count limit, with ties going to the host.
   
   Recommendation: **the host's**. It matches `peekCrossTreeNode`'s scan order.
3. **Which count chooses an alternative's traversal arrow?**
   - The alternative's own `Count`, as the plan has it: fires on its Nth voicing.
   - The host's `Count`: fires on the host's Nth visit, whichever voicing is active.
   
   Recommendation: **the alternative's own**.

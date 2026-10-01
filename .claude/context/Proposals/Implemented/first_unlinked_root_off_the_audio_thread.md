# Find the First Unlinked Root on the Message Thread

> Status: Implemented
> Implemented 2026-09-30 at uncommitted (on 2233a2a).
> Written 2026-09-30 at 2233a2a. The working tree has uncommitted edits, but none to the functions this plan touches. Sources: Reviewed/ProgramAudits/tracker.md (What Survives: "P2: `findFirstUnlinkedRootId` can reallocate on the audio thread"; ledger #15, #16).
> Built from 2233a2a; drift: none (the uncommitted `TraversalSession` suspend/resume edits predate the proposal and touch no site it edits).

## Summary
Before it starts a walk, the audio thread decides which root to start from. `TraversalSession::findFirstUnlinkedRootId` does this by collecting the ID of every root that some arrow points into, in a vector reserved for 256 entries. A graph with more than 256 arrows into root nodes makes that vector reallocate inside `processBlock`, which breaks the no-allocation rule. The answer depends only on the published `NodeMap`. So the plan computes it once on the message thread, when `AudioSnapshotPublisher::publishGraph` merges a graph, and stores it as a plain `int` on the `NodeMap`. The audio thread then reads one field. This also removes the scratch vector and the per-call sort from the audio thread. The fix is 3 steps, about 30 lines removed and 25 added, plus two tests. Priority P2.

## The Problem
### Mechanism
1. `processBlock` (`Source/Plugin/PluginProcessor.cpp:272`) calls `driveWalk` (`:329`). While playing with an empty traversal pool, `driveWalk` calls `traversalSession.startTraversalsFromFirstRoot(context)` (`:376`). A host relocation also reaches it, through `beginReplay` (`:353` → `Source/Audio/TraversalSession.cpp:70`).
2. `startTraversalsFromFirstRoot` (`TraversalSession.cpp:388-402`) calls `findFirstUnlinkedRootId(context.nodes)` (`:361-386`).
3. That function clears `linkedRootScratch` and walks every node's `connections`. For each connection whose child is a root (`isRootNode`: `nodeID == graphID`, `:354-357`), it calls `linkedRootScratch.push_back(childId)` (`:372`). It pushes once per connection, with no bound and no deduplication.
4. `linkedRootScratch` is reserved to `scratchCapacity` = 256 in the constructor (`TraversalSession.h:69`, `.cpp:24`). Push number 257 reallocates, which calls `malloc` and `free` on the audio thread.
5. The function then sorts the vector (`:377`, in place, no allocation) and returns the lowest-ID root that no arrow points into (`:379-383`).

**Input that exposes it:** one root `A` with 257 child nodes, each with an arrow to a second root `B`. Press play, or relocate the host playhead. Once the vector has grown, its capacity stays grown, so only the first call after `prepare` allocates.

### Evidence
- Review ledger #15 (confirmed) and #16 (the sort is a cost, not a real-time violation), re-read at `HEAD`. The lines above are current.
- The other `TraversalSession` scratch vectors are bounded. `activeRootIdScratch` and `restartRootScratch` hold distinct home roots of pool entries, so at most 128 against a reserve of 256. `removedRunIdScratch` holds at most 128 run IDs against a reserve of 128. Only `linkedRootScratch` is sized by the graph.
- No test covers it. `Tests/RealtimeTests.cpp` builds a four-node graph with no arrows into roots. Step 1 adds a test that should abort under RealtimeSanitizer at `HEAD`.

### Why It Matters
It breaks the Project Design Rule "Never allocate or free on the audio thread", and the analyst profile's audio-thread invariant. The trigger is rare, because more than 256 root-bound arrows is a large graph. The consequence is a possible dropout at the moment playback starts or the host jumps, which is when a glitch is most audible. The graph size that triggers it is the user's choice, not a bug they can avoid. **P2.**

## Current Design
### API Surface
- `int TraversalSession::findFirstUnlinkedRootId(const NodeMap&)`: private (`TraversalSession.h:59`, `.cpp:361-386`). One caller: `startTraversalsFromFirstRoot` (`.cpp:390`).
- `bool TraversalSession::startTraversalsFromFirstRoot(const DispatchContext&)`: public (`.h:30`, `.cpp:388-402`). Callers: `SequenceTreeAudioProcessor::driveWalk` (`PluginProcessor.cpp:376`) and `TraversalSession::beginReplay` (`TraversalSession.cpp:70`). It returns `false` when there is no unlinked root. `driveWalk` then silences any sounding notes and returns.
- `std::vector<int> TraversalSession::linkedRootScratch` (`.h:74`), reserved at `.cpp:24`, used only by `findFirstUnlinkedRootId`.
- `isRootNode(const RTNode&)`, in an anonymous namespace at `TraversalSession.cpp:352-359`, used only by `findFirstUnlinkedRootId` (`:371`, `:380`).
- `struct NodeMap { std::vector<RTNode> sortedById; find(); }` (`Source/Graph/RTData.h:121-134`).
- `void AudioSnapshotPublisher::publishGraph(int graphId, NodeMap graphNodes)` (`AudioSnapshotPublisher.h:47`, `.cpp:25-47`). Callers: `RTGraphBuilder::makeRTGraph` (`RTGraphBuilder.cpp:249`) and `RTGraphBuilder::discardGraph` (`:591`). It is the only function that builds a new `globalNodes` with a different set of nodes or connections.
- Other writers of `globalNodes`:
  - `RTGraphBuilder::updateDurationMaps` (`RTGraphBuilder.cpp:526-585`) copies the published `NodeMap` (`:538`) and rewrites connection durations in place through `fillDurationMap`. It never adds or removes nodes or connections.
  - `publishScript` carries `globalNodes` forward by pointer through `beginEdit` (`AudioSnapshotPublisher.cpp:13-23`).

### Structure
- `TraversalSession` (`Source/Audio/`) owns the traversal lifecycle, and the processor owns it.
- `AudioSnapshotPublisher` (`Source/Plugin/`) owns everything the audio thread reads. It is the one place a published `NodeMap` is assembled from rebuilt graphs.
- `NodeMap` (`Source/Graph/RTData.h`) is one of the plain structs allowed to cross the audio boundary. It already holds more than raw data: it carries `find`, and it guarantees `sortedById` is sorted.

### Data Flow
```
message thread                                   audio thread
GraphState edit → RTGraphBuilder listener
  → makeRTGraph → freezeNodes
  → AudioSnapshotPublisher::publishGraph
      set_union into merged NodeMap
      publish(): currentSnapshot.store ───────▶  processBlock → beginBlock → snap
                                                  driveWalk(*snap.globalNodes as context.nodes)
                                                    ├ beginReplay → startTraversalsFromFirstRoot
                                                    └ pool empty  → startTraversalsFromFirstRoot
                                                          findFirstUnlinkedRootId(context.nodes)  ← allocation here
```
After the change, the merge step in `publishGraph` fills `merged->firstUnlinkedRootId`, and `startTraversalsFromFirstRoot` reads `context.nodes.firstUnlinkedRootId`.

## Approaches Considered
1. **Compute it at publish time, and store it on `NodeMap` (recommended).** Move the same algorithm into `AudioSnapshotPublisher`, run it on the merged map in `publishGraph`, and store the result in a new `int firstUnlinkedRootId = -1` on `NodeMap`. `updateDurationMaps` copies the `NodeMap` and carries the field over, which is correct because it never changes connectivity.
   - *Cost:* about 25 lines added, 30 removed.
   - *Risk:* a future writer that changes connectivity without going through `publishGraph` would leave the field stale. Today there is none, and Step 2 documents the rule in CLAUDE.md next to the existing "keeps it sorted" rule.
   - *Rules:* fits. The field is public data, there is no accessor, and the function is not a wrapper.
   - The audio thread does no work, and there is no scratch or sort left on it.
2. **Compute at publish time, but store it on `Snapshot`.** The same computation, but kept beside `globalNodes` instead of inside it. `DispatchContext` has no snapshot, so the value would have to be added to `DispatchContext`. It is built at `PluginProcessor.cpp:341` and twice in `TraversalSession` (`:65`, `:81`), and `startTraversalsFromFirstRoot` would read it from there.
   - *Cost:* more plumbing, and one more field that every `DispatchContext` construction must copy.
   - *Risk:* the replay contexts must forward it, or replay silently starts nothing.
   - This keeps `NodeMap` free of derived data, at the price of touching three constructions.
3. **Keep it on the audio thread, but bound it.** Either scan without a scratch vector, testing each root against every connection (O(roots × connections), no allocation), or refuse to push past the reserve.
   - *Cost:* smallest diff.
   - *Risk:* the O(R × C) scan runs every block in the "every root is linked" case, and its cost grows with the graph. Refusing to push changes behaviour: a linked root past entry 256 would be treated as unlinked.
   - Neither removes work from the audio thread, and the second is wrong.

**Recommendation:** Approach 1. It removes the allocation, the sort and the per-call scan from the audio thread, and changes no behaviour. It puts the computation where the rest of the published graph is assembled. Choosing between 1 and 2 is the owner's call (Decision 1).

## Plan

### Step 1: A realtime test that allocates at `HEAD`
- **Goal:** prove the defect before fixing it, and guard the fix afterwards.
- **Changes:** add a second `TEST_CASE` to `Tests/RealtimeTests.cpp`, "starting a walk stays realtime-safe with more root-bound arrows than the scratch reserve", tagged `[realtime]`. Declare its locals at the top, as the existing case does.
  - Build root `A` with `NodeFactory::createRootNode`, then root `B`. Create 257 children of `A` with `NodeFactory::createNode(graph, aId, ValueTreeIdentifiers::NodeData, NodePosition { 100 + 10 * i, 40 * (i % 8) - 140, 25 }, nullptr)`. Distinct X positions keep them out of chord links. Connect each child to `B` with `graph.connectNodes(childId, bId, nullptr)`. Then `processor.rtGraphBuilder.rebuildAllGraphs()`.
  - Reuse the existing `HostPlayHead` and the same prepare, play and relocate sequence: play 50 blocks, set `ppq = 16.0`, play 50 blocks. Then `REQUIRE(noteOns > 0)`.
- **Mechanical edits:** none.
- **Behaviour delta:** none. The change is test-only. The target exists only when `SEQUENCETREE_REALTIME_SANITIZER` is `ON`.
- **Invariants:** exercises the audio-thread no-allocation rule directly.
- **Verification:** configure the RealtimeSanitizer tree outside the repo, as CLAUDE.md's *Tests* section describes. No existing tree was found. Build `SequenceTree_RealtimeTests` and run it at `HEAD`. **Expected: RealtimeSanitizer aborts with a `malloc`/`realloc` stack through `std::vector<int>::push_back` ← `TraversalSession::findFirstUnlinkedRootId` ← `startTraversalsFromFirstRoot` ← `driveWalk` ← `processBlock`.** Record that stack in the implementation notes. If it aborts somewhere else first, stop: that is a different finding, and it goes back to the owner. The existing realtime case must still pass.
- **Rollback:** delete the test case.

### Step 2: Compute the first unlinked root when a graph is published
- **Goal:** every published `NodeMap` carries its first unlinked root, computed on the message thread.
- **Changes:**
  - `Source/Graph/RTData.h`, `struct NodeMap`: add the public member `int firstUnlinkedRootId = -1;` after `sortedById`.
  - `Source/Plugin/AudioSnapshotPublisher.h`: add the private `static int findFirstUnlinkedRootId(const NodeMap& nodes);`.
  - `Source/Plugin/AudioSnapshotPublisher.cpp`: define it with the algorithm from `TraversalSession.cpp:361-386`, unchanged in result. The scratch becomes a local `std::vector<int> linkedRootIds;` declared at the top, because the message thread may allocate. The `isRootNode` helper is not carried over; its one comparison `node.nodeID == node.graphID` is written at the two places it is used, so no single-operation function is added. In `publishGraph`, after either branch has filled `merged->sortedById` and before `edit->globalNodes = std::move(merged);`, set `merged->firstUnlinkedRootId = findFirstUnlinkedRootId(*merged);`.
  - `.claude/CLAUDE.md`, the `RTData.h` bullet under *Source/Graph/*: add that `publishGraph` also sets `firstUnlinkedRootId`, the lowest-ID root no arrow points into, and that anything that changes which nodes or connections a published `NodeMap` holds must go through `publishGraph` so that field stays true.
- **Mechanical edits:** none. This is a cross-class move, which no `refactor.*` command performs. The old copy stays until Step 3, so the tree builds at every step.
- **Behaviour delta:** none observable. The field is written and not yet read. `publishGraph` does one extra O(C log C) pass per graph publish, on the message thread.
- **Invariants:** only the message thread runs it. The field is a plain `int` inside an `RTData` struct that is already published through `publish()`, so it crosses the boundary like the rest of `NodeMap`. `updateDurationMaps` copies it over at `RTGraphBuilder.cpp:538` and never changes connectivity (`fillDurationMap` only writes `connection.duration`), so the copy stays correct.
- **Verification:**
  - Build `SequenceTree_Standalone`, then `SequenceTree_Tests` and `SequenceTree_GraphTests`, and run `ctest`.
  - New `GraphTests.cpp` case, "the published graph names its first unlinked root", tagged `[graph]`. It uses the existing `createRoot`, `createChild` and `rebuildAndPublish` helpers. The expected values come from the definition, "the lowest-ID root that no connection's child is":
    - `A`, `B` roots (`A` created first, so it has the lower ID), `C` a child of `A` → `firstUnlinkedRootId == A`.
    - `connectNodes(C, B)` → still `A`.
    - `D` a child of `B`, `connectNodes(D, A)` → `-1`.
    - `disconnectNodes(C, B)` → `B`.
  - Run `design-rules.sh --file` and `readability.sh --file` on `AudioSnapshotPublisher.cpp`, `AudioSnapshotPublisher.h` and `RTData.h`.
- **Rollback:** revert the three source files and the CLAUDE.md sentence, and delete the test case.

### Step 3: Read the published value on the audio thread, and delete the scan
- **Goal:** remove the allocation, the scratch vector and the sort from the audio thread.
- **Changes:**
  - `TraversalSession::startTraversalsFromFirstRoot` (`TraversalSession.cpp:388-402`): replace `const int rootId = findFirstUnlinkedRootId(context.nodes);` with a read of `context.nodes.firstUnlinkedRootId`. Replace the unchecked `*context.nodes.find(rootId)` with a `const RTNode* const rootNode = context.nodes.find(rootId);` checked for `nullptr` (return `false`). Its non-null guarantee now spans two classes, and this checked-lookup convention is the one `traversal_target_lookup_hygiene.md` established. Declare the locals at the top.
  - Delete `TraversalSession::findFirstUnlinkedRootId` (`.h:59`, `.cpp:361-386`), the anonymous-namespace `isRootNode` (`.cpp:352-359`), the `linkedRootScratch` member (`.h:74`) and its reserve (`.cpp:24`). `scratchCapacity` stays, because the other two scratches use it.
- **Mechanical edits:** `refactor.decap TraversalSession::findFirstUnlinkedRootId` is not used, because the body is replaced rather than dissolved. Delete by hand, then confirm with `refactor.find` / clangd that no references remain.
- **Behaviour delta:** for every published graph, `startTraversalsFromFirstRoot` picks the same root as before, because the field was computed by the same algorithm from the same `NodeMap`. With no unlinked root, it returns `false` as before. With an empty or unpublished map, the field defaults to `-1`, as before. The one new branch, a stale field naming a missing node, returns `false` where the old code would have dereferenced `nullptr`. That cannot happen through `publishGraph` and exists only as a guard.
- **Invariants:** `processBlock` no longer allocates or sorts on this path. The only audio-thread work left is one field read and one `lower_bound`.
- **Verification:**
  - Build `SequenceTree_Standalone`, the two test targets and `ctest`.
  - Rebuild the RealtimeSanitizer tree and run `SequenceTree_RealtimeTests`. **Both cases must pass, and the Step 1 case must now run to completion.**
  - Run `design-rules.sh --file` and `readability.sh --file` on `TraversalSession.cpp` and `TraversalSession.h`.
  - Manual check in the standalone: two trees with a cross-root arrow from the first to the second. Press play: the first tree's root sounds and highlights first. Add an arrow from the second tree back to the first, then stop and play: nothing starts, as today. Then in a host (AU in Live or Logic), move the playhead while playing and check the walk restarts from the same root.
- **Rollback:** restore `findFirstUnlinkedRootId`, `isRootNode` and `linkedRootScratch`, and the old first line of `startTraversalsFromFirstRoot`. Step 2's field is harmless on its own.

### Step 4: Record it
- **Goal:** keep the issues file accurate.
- **Changes:** `.claude/notes/ongoing-issues.md`, a new entry #15, "Starting a walk could allocate on the audio thread". It gives the mechanism above, the Step 1 RealtimeSanitizer stack as evidence, and the status "resolved <date>, `Proposals/Implemented/first_unlinked_root_off_the_audio_thread.md`", naming both new tests.
- **Behaviour delta, invariants, rollback:** documentation only.

## Risks
- **A future connectivity writer bypasses `publishGraph`.** Low likelihood. The symptom would be a wrong start root, or nothing starting, after that edit. Guards: the CLAUDE.md rule from Step 2, the GraphTests case, and the null check from Step 3, which turns a stale ID into "no start" rather than undefined behaviour.
- **The 257-child test graph trips RealtimeSanitizer somewhere else first.** It would show up as an abort with a different stack in Step 1. If it happens, that is a real finding. The plan stops and reports it rather than shrinking the graph to hide it.
- **The RealtimeSanitizer tree is not configured on this machine.** No tree was found outside the repo. Step 1 needs Homebrew LLVM, which is present at `/opt/homebrew/opt/llvm/bin/clang++`, and a fresh configure, which takes a few minutes.
- **Performance.** One more pass per `publishGraph` on the message thread, O(C log C) in the number of connections. It is negligible next to the graph rebuild that precedes it.

## Out of Scope
- **Four `ongoing-issues.md` entries (#3, #5, #6, #9) describe code that no longer exists.** Review P3. The cause and the fix are unrelated, and it is a documentation edit that doesn't need a plan.
- **The flag scheduler's linear scan** (`FlagScheduler.cpp:81-93`), **triple path flattening per trail** (#10), and **one `VBlankAttachment` per arrow and per node.** All P3 and unmeasured. Each needs a profile before any proposal.
- **The selection being read from view components** (`SelectionOps.cpp:33-44`). P3, with no present defect.

## Decisions for the Owner
1. **Where should the precomputed root live?**
   - (a) **Recommended:** a public `int firstUnlinkedRootId` on `NodeMap`. The audio thread reads `context.nodes.firstUnlinkedRootId`, and nothing else changes shape.
   - (b) A field on `AudioSnapshotPublisher::Snapshot`, added to `DispatchContext` and forwarded by all three of its constructions (`PluginProcessor.cpp:341`, `TraversalSession.cpp:65`, `:81`). This keeps `NodeMap` to nodes only.
2. **Where should the computation live?**
   - (a) **Recommended:** a private static `AudioSnapshotPublisher::findFirstUnlinkedRootId`, called from `publishGraph`, where every published node set is assembled.
   - (b) A member function on `NodeMap` that sets its own field, beside `find`. The derived value would sit next to its data, but a struct in `RTData.h` would gain logic that only the publisher calls.
3. **Should Step 3 add the `nullptr` check** on the start root? Recommended yes. It is behaviour-neutral for every graph `publishGraph` can produce, and it follows the checked-lookup convention of `traversal_target_lookup_hygiene.md`.

**Owner's answers (2026-09-30):** 1 → (a) `NodeMap` field. 2 → (a) private static on `AudioSnapshotPublisher`. 3 → yes, add the `nullptr` check.

## Implementation Notes

- **Step 1.** Added "starting a walk stays realtime-safe with more root-bound arrows than the scratch reserve" at `Tests/RealtimeTests.cpp:117`, as planned. The RealtimeSanitizer tree was configured in the session scratchpad, per the owner, so it disappears with the session and the next run must reconfigure. At 2233a2a it aborted with `unsafe-library-call` in `malloc` ← `TraversalSession::findFirstUnlinkedRootId` ← `startTraversalsFromFirstRoot` ← `beginReplay` ← `driveWalk` ← `processBlock`. It arrived through `beginReplay` rather than the empty-pool path: the host starting at PPQ 0 counts as a relocation, so the replay reached it first. The existing realtime case passed on its own.
- **Step 2.** Added `NodeMap::firstUnlinkedRootId` (`Source/Graph/RTData.h`), the private static `AudioSnapshotPublisher::findFirstUnlinkedRootId` (`.h:61`, `.cpp:113-138`), and the call in `publishGraph` (`.cpp:45`). The algorithm is unchanged; the root test is written in place of `isRootNode`, and the scratch is a local vector. Added "the published graph names its first unlinked root" to `Tests/GraphTests.cpp`, and a sentence to CLAUDE.md's `RTData.h` bullet. No differences from the plan.
- **Step 3.** `startTraversalsFromFirstRoot` reads `context.nodes.firstUnlinkedRootId` and returns `false` when `find` gives `nullptr`. That check also covers `-1`, so the separate `-1` test was dropped as redundant. Deleted `findFirstUnlinkedRootId`, `isRootNode`, `linkedRootScratch` and its reserve. `scratchCapacity` stays for the other two scratch vectors. Both realtime cases pass.
- **Step 4.** Added `ongoing-issues.md` #15, marked resolved.
- **Verification as run:** `SequenceTree_Standalone`, `SequenceTree_Tests` and `SequenceTree_GraphTests` build, `ctest` 60/60, `SequenceTree_RealtimeTests` 2/2. `design-rules.sh` shows no violations on every changed file, and `readability.sh` flags only the pre-existing 89-line realtime test case at `Tests/RealtimeTests.cpp:27`.
- **Manual checks still owed:**
  - In the standalone, with two trees and a cross-root arrow from the first to the second: play starts on the first root.
  - With an arrow back from the second tree to the first, stop then play: nothing starts.
  - In a host (AU in Live or Logic), moving the playhead while playing restarts the walk from the same root.


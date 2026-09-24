#include <catch2/catch_test_macros.hpp>

#include "Audio/ScriptTraversalRule.h"
#include "Audio/TraversalLogic.h"
#include "Script/ScriptCompiler.h"

#include <algorithm>
#include <vector>

static RTNode makeNode(int id, int parentId, RTNode::NodeType type, int countLimit, const std::vector<int>& childIds)
{
    RTNode node;

    node.nodeID            = id;
    node.parentId          = parentId;
    node.nodeType          = type;
    node.countLimit        = countLimit;
    node.switchCountLimit  = 1;
    node.subLoopCountLimit = 1;

    for (int childId : childIds) {
        RTConnection connection;
        connection.childId  = childId;
        connection.duration = 500;

        node.connections.push_back(connection);
    }

    return node;
}

static NodeMap makeMap(const std::vector<RTNode>& nodes)
{
    NodeMap map { nodes };

    std::ranges::sort(map.sortedById, {}, &RTNode::nodeID);

    return map;
}

static NodeMap mirrorAsModulators(const NodeMap& tree)
{
    NodeMap mirror;

    for (const RTNode& node : tree.sortedById) {
        RTNode modulator = node;

        switch (node.nodeType) {
            case RTNode::NodeType::RootNode:
                modulator.nodeType = RTNode::NodeType::ModulatorRoot;
                break;

            case RTNode::NodeType::Node:
                modulator.nodeType = RTNode::NodeType::Modulator;
                break;

            case RTNode::NodeType::Alternative:
                modulator.nodeType = RTNode::NodeType::AlternativeModulator;
                break;

            default:
                break;
        }

        mirror.sortedById.push_back(modulator);
    }

    return mirror;
}

static std::vector<int> walkPrimary(const NodeMap& nodes, int rootId, int steps, const TraversalRule& rule)
{
    TraversalLogic logic;

    logic.nodeState.prepare();
    logic.reset(rootId, RTtraversal {});
    logic.rule = &rule;
    logic.begin(nodes, rootId, 0);

    std::vector<int> visited { logic.primary.target };

    if (logic.primary.alternativeTarget != -1) {
        visited.push_back(logic.primary.alternativeTarget);
    }

    for (int step = 0; step < steps; ++step) {
        const TraversalLogic::StepResult result = logic.handleNodeEvent(nodes);

        visited.push_back(result.enteredId);

        if (result.enteredAlternativeId != -1) {
            visited.push_back(result.enteredAlternativeId);
        }
    }

    return visited;
}

static std::vector<int> walkModulator(const NodeMap& nodes, int modulatorRootId, int steps)
{
    const int hostId = 0;

    TraversalLogic logic;

    logic.nodeState.prepare();
    logic.reset(hostId, RTtraversal {});
    logic.mod.activate(modulatorRootId, hostId);
    logic.advanceAlternative(nodes, modulatorRootId);

    std::vector<int> visited { logic.mod.walker.target };

    if (logic.mod.walker.alternativeTarget != -1) {
        visited.push_back(logic.mod.walker.alternativeTarget);
    }

    for (int step = 0; step < steps; ++step) {
        logic.decideNextModulator(nodes);
        logic.mod.step();
        logic.advanceAlternative(nodes, logic.mod.walker.target);

        visited.push_back(logic.mod.walker.target);

        if (logic.mod.walker.alternativeTarget != -1) {
            visited.push_back(logic.mod.walker.alternativeTarget);
        }
    }

    return visited;
}

static NodeMap chainShape()
{
    return makeMap({
        makeNode(1, 0, RTNode::NodeType::RootNode, 1, { 2 }),
        makeNode(2, 1, RTNode::NodeType::Node,     1, { 3 }),
        makeNode(3, 2, RTNode::NodeType::Node,     1, {})
    });
}

static NodeMap countLimitShape()
{
    return makeMap({
        makeNode(1, 0, RTNode::NodeType::RootNode, 1, { 2, 3 }),
        makeNode(2, 1, RTNode::NodeType::Node,     1, {}),
        makeNode(3, 1, RTNode::NodeType::Node,     2, {})
    });
}

static NodeMap switchCountShape()
{
    RTNode held = makeNode(2, 1, RTNode::NodeType::Node, 2, {});
    held.switchCountLimit = 3;

    return makeMap({
        makeNode(1, 0, RTNode::NodeType::RootNode, 1, { 2, 3 }),
        held,
        makeNode(3, 1, RTNode::NodeType::Node,     1, {})
    });
}

static NodeMap subLoopShape()
{
    RTNode loopEntry = makeNode(2, 1, RTNode::NodeType::Node, 1, { 3 });
    loopEntry.subLoopCountLimit = 2;

    return makeMap({
        makeNode(1, 0, RTNode::NodeType::RootNode, 1, { 2 }),
        loopEntry,
        makeNode(3, 2, RTNode::NodeType::Node,     1, {})
    });
}

static NodeMap triggerLimitShape()
{
    RTNode spent = makeNode(2, 1, RTNode::NodeType::Node, 1, {});
    spent.triggerLimit = 2;

    return makeMap({
        makeNode(1, 0, RTNode::NodeType::RootNode, 1, { 2 }),
        spent
    });
}

static NodeMap alternativeShape()
{
    RTNode host = makeNode(2, 1, RTNode::NodeType::Node, 1, { 10 });
    host.alternativeRootId = 10;

    RTNode firstAlternative = makeNode(10, 2, RTNode::NodeType::Alternative, 1, { 11 });
    firstAlternative.alternativeRootId = 10;

    RTNode secondAlternative = makeNode(11, 10, RTNode::NodeType::Alternative, 1, {});
    secondAlternative.alternativeRootId = 10;

    return makeMap({
        makeNode(1, 0, RTNode::NodeType::RootNode, 1, { 2 }),
        host,
        firstAlternative,
        secondAlternative
    });
}

static NodeMap encapsulationShape()
{
    RTNode entry = makeNode(2, 1, RTNode::NodeType::Node, 1, { 3 });
    entry.encapsulationEntryId = 2;
    entry.subLoopCountLimit    = 2;

    RTNode member = makeNode(3, 2, RTNode::NodeType::Node, 1, { 4 });
    member.encapsulationEntryId = 2;

    return makeMap({
        makeNode(1, 0, RTNode::NodeType::RootNode, 1, { 2 }),
        entry,
        member,
        makeNode(4, 3, RTNode::NodeType::Node,     1, {})
    });
}

static NodeMap stepIntoTreeShape(int foreignLoopLimit)
{
    RTNode foreignRoot = makeNode(5, 2, RTNode::NodeType::RootNode, 1, { 6 });
    foreignRoot.subLoopCountLimit = foreignLoopLimit;

    return makeMap({
        makeNode(1, 0, RTNode::NodeType::RootNode, 1, { 2 }),
        makeNode(2, 1, RTNode::NodeType::Node,     1, { 5 }),
        foreignRoot,
        makeNode(6, 5, RTNode::NodeType::Node,     1, {})
    });
}

TEST_CASE("a chain plays in order and loops back to the root", "[traversal]")
{
    const std::vector<int> expected { 1, 2, 3, 1, 2, 3, 1 };

    CHECK(walkPrimary(chainShape(), 1, 6, NativeTraversalRule::instance()) == expected);
}

TEST_CASE("the largest count limit that divides the parent count wins", "[traversal]")
{
    const std::vector<int> expected { 1, 2, 1, 3, 1, 2, 1, 3 };

    CHECK(walkPrimary(countLimitShape(), 1, 7, NativeTraversalRule::instance()) == expected);
}

TEST_CASE("a switch count holds the parent on that child for that many visits", "[traversal]")
{
    const std::vector<int> expected { 1, 3, 1, 2, 1, 2, 1, 2, 1, 3, 1, 2 };

    CHECK(walkPrimary(switchCountShape(), 1, 11, NativeTraversalRule::instance()) == expected);
}

TEST_CASE("a sub loop replays its subtree before returning to the root", "[traversal]")
{
    const std::vector<int> expected { 1, 2, 3, 2, 3, 1, 2, 3, 2, 3, 1 };

    CHECK(walkPrimary(subLoopShape(), 1, 10, NativeTraversalRule::instance()) == expected);
}

TEST_CASE("a spent trigger limit makes the child ineligible", "[traversal]")
{
    const std::vector<int> expected { 1, 2, 1, 2, 1, 1, 1 };

    CHECK(walkPrimary(triggerLimitShape(), 1, 6, NativeTraversalRule::instance()) == expected);
}

TEST_CASE("peeking at the next target leaves the walk unchanged", "[traversal]")
{
    RTNode heldOnSecondCount = makeNode(2, 1, RTNode::NodeType::Node, 2, {});
    heldOnSecondCount.switchCountLimit = 3;

    const NodeMap countGap = makeMap({
        makeNode(1, 0, RTNode::NodeType::RootNode, 1, { 2, 3 }),
        heldOnSecondCount,
        makeNode(3, 1, RTNode::NodeType::Node,     3, {})
    });

    RTNode heldOnTie = makeNode(2, 1, RTNode::NodeType::Node, 1, {});
    heldOnTie.switchCountLimit = 3;

    const NodeMap tiedLimits = makeMap({
        makeNode(1, 0, RTNode::NodeType::RootNode, 1, { 2, 3 }),
        heldOnTie,
        makeNode(3, 1, RTNode::NodeType::Node,     1, {})
    });

    const int steps = 64;

    for (const NodeMap& nodes : { countGap, tiedLimits, switchCountShape() }) {
        TraversalLogic stepped;
        TraversalLogic peeked;

        for (TraversalLogic* logic : { &stepped, &peeked }) {
            logic->nodeState.prepare();
            logic->reset(1, RTtraversal {});
            logic->begin(nodes, 1, 0);
        }

        std::vector<int> steppedWalk;
        std::vector<int> peekedWalk;

        for (int step = 0; step < steps; ++step) {
            peeked.peekNextTarget(nodes);

            steppedWalk.push_back(stepped.handleNodeEvent(nodes).enteredId);
            steppedWalk.push_back(stepped.nodeState.get(NodeStateSlot::Count, 1));

            peekedWalk.push_back(peeked.handleNodeEvent(nodes).enteredId);
            peekedWalk.push_back(peeked.nodeState.get(NodeStateSlot::Count, 1));
        }

        CHECK(peekedWalk == steppedWalk);
    }
}

TEST_CASE("zero duration and disabled arrows are never taken", "[traversal]")
{
    RTNode root = makeNode(1, 0, RTNode::NodeType::RootNode, 1, { 2, 3, 4 });

    root.connections[0].duration = 0;
    root.connections[1].disabledTraversals.push_back(TraversalKey {});

    const NodeMap nodes = makeMap({
        root,
        makeNode(2, 1, RTNode::NodeType::Node, 1, {}),
        makeNode(3, 1, RTNode::NodeType::Node, 1, {}),
        makeNode(4, 1, RTNode::NodeType::Node, 1, {})
    });

    const std::vector<int> expected { 1, 4, 1, 4, 1 };

    CHECK(walkPrimary(nodes, 1, 4, NativeTraversalRule::instance()) == expected);
}

TEST_CASE("the graph loop limit ends the traversal", "[traversal]")
{
    const NodeMap nodes = makeMap({
        makeNode(1, 0, RTNode::NodeType::RootNode, 1, { 2 }),
        makeNode(2, 1, RTNode::NodeType::Node,     1, {})
    });

    TraversalLogic logic;

    logic.nodeState.prepare();
    logic.reset(1, RTtraversal {});
    logic.begin(nodes, 1, 2);

    using Kind = TraversalLogic::StepResult::Kind;

    const std::vector<Kind> expected { Kind::Advanced, Kind::LoopedToRoot, Kind::Advanced, Kind::Ended, Kind::Ended };

    std::vector<Kind> kinds;

    for (std::size_t step = 0; step < expected.size(); ++step) {
        kinds.push_back(logic.handleNodeEvent(nodes).kind);
    }

    CHECK(kinds == expected);
    CHECK_FALSE(logic.shouldTraverse());
}

TEST_CASE("alternatives rotate from the host through each alternative and back", "[traversal]")
{
    const std::vector<int> expected { 1, 2, 1, 2, 10, 1, 2, 11, 1, 2, 1, 2, 10, 1, 2, 11, 1 };

    CHECK(walkPrimary(alternativeShape(), 1, 12, NativeTraversalRule::instance()) == expected);
}

TEST_CASE("an encapsulated group replays from its entry before leaving through its exit", "[traversal]")
{
    const std::vector<int> expected { 1, 2, 3, 2, 3, 4, 1, 2, 3, 2, 3, 4, 1 };

    CHECK(walkPrimary(encapsulationShape(), 1, 12, NativeTraversalRule::instance()) == expected);
}

TEST_CASE("a stepped into tree loops as many times as its loop limit, then returns home", "[traversal]")
{
    SECTION("loop limit 1 passes through once") {
        const std::vector<int> expected { 1, 2, 5, 6, 1, 2, 5, 6, 1 };

        CHECK(walkPrimary(stepIntoTreeShape(1), 1, 8, NativeTraversalRule::instance()) == expected);
    }

    SECTION("loop limit 2 loops twice") {
        const std::vector<int> expected { 1, 2, 5, 6, 5, 6, 1, 2, 5, 6, 5, 6, 1 };

        CHECK(walkPrimary(stepIntoTreeShape(2), 1, 12, NativeTraversalRule::instance()) == expected);
    }

    SECTION("loop limit 0 never returns home") {
        const std::vector<int> expected { 1, 2, 5, 6, 5, 6, 5, 6, 5, 6, 5 };

        CHECK(walkPrimary(stepIntoTreeShape(0), 1, 10, NativeTraversalRule::instance()) == expected);
    }
}

TEST_CASE("a traversal arrow relocates the walker onto the other tree for good", "[traversal]")
{
    RTNode leaving = makeNode(2, 1, RTNode::NodeType::Node, 1, { 5 });
    leaving.connections[0].isTreeJump = true;

    const NodeMap nodes = makeMap({
        makeNode(1, 0, RTNode::NodeType::RootNode, 1, { 2 }),
        leaving,
        makeNode(5, 0, RTNode::NodeType::RootNode, 2, { 6 }),
        makeNode(6, 5, RTNode::NodeType::Node,     1, {})
    });

    const std::vector<int> expected { 1, 2, 1, 2, 5, 6, 5, 6, 5 };

    CHECK(walkPrimary(nodes, 1, 8, NativeTraversalRule::instance()) == expected);

    TraversalLogic logic;

    logic.nodeState.prepare();
    logic.reset(1, RTtraversal {});
    logic.begin(nodes, 1, 0);

    TraversalLogic::StepResult jump;

    for (int step = 0; step < 4; ++step) {
        jump = logic.handleNodeEvent(nodes);
    }

    CHECK(jump.kind == TraversalLogic::StepResult::Kind::JumpedToTree);
    CHECK(jump.jumpedFromRootId == 1);
    CHECK(jump.enteredId == 5);
    CHECK(logic.rootId == 5);
}

TEST_CASE("a cross root tree connection fires on its count limit and holds for its switch count", "[traversal]")
{
    RTNode host = makeNode(1, 0, RTNode::NodeType::RootNode, 1, { 5, 7 });
    host.connections[0].isCrossRoot = true;
    host.connections[1].isCrossRoot = true;
    host.connections[1].disabledTraversals.push_back(TraversalKey {});

    RTNode foreignRoot = makeNode(5, 0, RTNode::NodeType::RootNode, 2, {});
    foreignRoot.switchCountLimit = 3;

    const NodeMap nodes = makeMap({
        host,
        foreignRoot,
        makeNode(7, 0, RTNode::NodeType::RootNode, 1, {})
    });

    TraversalLogic logic;

    logic.nodeState.prepare();
    logic.reset(1, RTtraversal {});
    logic.begin(nodes, 1, 0);

    std::vector<int> spawned;
    std::vector<int> spawnedPerVisit;

    for (int visit = 0; visit < 8; ++visit) {
        logic.peekCrossTreeNode(nodes, spawned);
        spawnedPerVisit.push_back(static_cast<int>(spawned.size()));

        if (!spawned.empty()) {
            CHECK(spawned.front() == 5);
        }
    }

    const std::vector<int> expected { 0, 1, 1, 1, 0, 1, 1, 1 };

    CHECK(spawnedPerVisit == expected);

    const std::vector<int> neverEntered { 1, 1, 1, 1 };

    CHECK(walkPrimary(nodes, 1, 3, NativeTraversalRule::instance()) == neverEntered);
}

TEST_CASE("a modulator root starts on its count limit, and an unsynced one is not restarted mid walk", "[traversal]")
{
    RTNode host = makeNode(1, 0, RTNode::NodeType::RootNode, 1, { 20 });

    const NodeMap synced = makeMap({
        host,
        makeNode(20, 1, RTNode::NodeType::ModulatorRoot, 2, {})
    });

    host.connections[0].isSynced = false;

    const NodeMap unsynced = makeMap({
        host,
        makeNode(20, 1, RTNode::NodeType::ModulatorRoot, 2, {})
    });

    host.connections[0].disabledTraversals.push_back(TraversalKey {});

    const NodeMap disabled = makeMap({
        host,
        makeNode(20, 1, RTNode::NodeType::ModulatorRoot, 2, {})
    });

    TraversalLogic logic;

    logic.nodeState.prepare();
    logic.reset(1, RTtraversal {});

    CHECK(logic.findActiveModulatorRoot(synced, 1) == -1);

    logic.nodeState.set(NodeStateSlot::Count, 1, 1);

    CHECK(logic.findActiveModulatorRoot(synced,   1) == 20);
    CHECK(logic.findActiveModulatorRoot(unsynced, 1) == 20);
    CHECK(logic.findActiveModulatorRoot(disabled, 1) == -1);

    logic.mod.activate(20, 1);

    CHECK(logic.findActiveModulatorRoot(synced,   1) == 20);
    CHECK(logic.findActiveModulatorRoot(unsynced, 1) == -1);
}

TEST_CASE("the largest dangling arrow limit that divides the count wins", "[traversal]")
{
    RTNode node;

    node.danglingArrows.push_back({ 500, 1, {} });
    node.danglingArrows.push_back({ 500, 2, {} });
    node.danglingArrows.push_back({ 0,   4, {} });
    node.danglingArrows.push_back({ 500, 3, { TraversalKey {} } });

    const TraversalRule& rule = NativeTraversalRule::instance();

    CHECK(rule.selectDanglingArrow(node, 1, TraversalKey {}) == 0);
    CHECK(rule.selectDanglingArrow(node, 2, TraversalKey {}) == 1);
    CHECK(rule.selectDanglingArrow(node, 3, TraversalKey {}) == 0);
    CHECK(rule.selectDanglingArrow(node, 4, TraversalKey {}) == 1);
    CHECK(rule.selectDanglingArrow(node, 3, TraversalKey { 1, 0 }) == 3);
}

TEST_CASE("a modulator walk unfolds the same sequence as a node traversal", "[traversal][parity]")
{
    const int steps = 24;

    SECTION("chain") {
        CHECK(walkModulator(mirrorAsModulators(chainShape()), 1, steps)
              == walkPrimary(chainShape(), 1, steps, NativeTraversalRule::instance()));
    }

    SECTION("count limits") {
        CHECK(walkModulator(mirrorAsModulators(countLimitShape()), 1, steps)
              == walkPrimary(countLimitShape(), 1, steps, NativeTraversalRule::instance()));
    }

    SECTION("switch count") {
        CHECK(walkModulator(mirrorAsModulators(switchCountShape()), 1, steps)
              == walkPrimary(switchCountShape(), 1, steps, NativeTraversalRule::instance()));
    }

    SECTION("sub loop") {
        CHECK(walkModulator(mirrorAsModulators(subLoopShape()), 1, steps)
              == walkPrimary(subLoopShape(), 1, steps, NativeTraversalRule::instance()));
    }

    SECTION("trigger limit") {
        CHECK(walkModulator(mirrorAsModulators(triggerLimitShape()), 1, steps)
              == walkPrimary(triggerLimitShape(), 1, steps, NativeTraversalRule::instance()));
    }

    SECTION("alternatives") {
        CHECK(walkModulator(mirrorAsModulators(alternativeShape()), 1, steps)
              == walkPrimary(alternativeShape(), 1, steps, NativeTraversalRule::instance()));
    }

    SECTION("encapsulation") {
        CHECK(walkModulator(mirrorAsModulators(encapsulationShape()), 1, steps)
              == walkPrimary(encapsulationShape(), 1, steps, NativeTraversalRule::instance()));
    }

    SECTION("encapsulation entered from the root") {
        RTNode entryRoot = makeNode(1, 0, RTNode::NodeType::RootNode, 1, { 2 });
        entryRoot.encapsulationEntryId = 1;
        entryRoot.subLoopCountLimit    = 2;

        RTNode member = makeNode(2, 1, RTNode::NodeType::Node, 1, { 3 });
        member.encapsulationEntryId = 1;

        const NodeMap nodes = makeMap({ entryRoot, member, makeNode(3, 2, RTNode::NodeType::Node, 1, {}) });

        CHECK(walkModulator(mirrorAsModulators(nodes), 1, steps)
              == walkPrimary(nodes, 1, steps, NativeTraversalRule::instance()));
    }

    SECTION("stepping into a tree") {
        for (int foreignLoopLimit : { 0, 1, 2 }) {
            CHECK(walkModulator(mirrorAsModulators(stepIntoTreeShape(foreignLoopLimit)), 1, steps)
                  == walkPrimary(stepIntoTreeShape(foreignLoopLimit), 1, steps, NativeTraversalRule::instance()));
        }
    }
}

TEST_CASE("the default script picks the same children as the native rule", "[traversal][script]")
{
    const ScriptCompileResult compiled = compileTraversalScript(defaultTraversalScriptSource());

    REQUIRE(compiled.succeeded());

    ScriptTraversalRule scriptRule;
    scriptRule.script = &compiled.script;

    RTNode weightedRoot = makeNode(1, 0, RTNode::NodeType::RootNode, 1, { 2, 3 });

    RTNode heavy = makeNode(2, 1, RTNode::NodeType::Node, 1, {});
    RTNode light = makeNode(3, 1, RTNode::NodeType::Node, 1, {});

    heavy.probability = 70;
    light.probability = 30;

    const NodeMap weighted = makeMap({ weightedRoot, heavy, light });

    const int steps = 64;

    for (const NodeMap& nodes : { chainShape(), countLimitShape(), switchCountShape(), subLoopShape(),
                                  triggerLimitShape(), weighted, alternativeShape(), encapsulationShape(),
                                  stepIntoTreeShape(0), stepIntoTreeShape(2) }) {
        CHECK(walkPrimary(nodes, 1, steps, scriptRule) == walkPrimary(nodes, 1, steps, NativeTraversalRule::instance()));
    }
}

TEST_CASE("node ids past the state table size walk like small ones", "[traversal][parity]")
{
    const int steps  = 24;
    const int offset = 5000;

    NodeMap shifted;

    for (const RTNode& node : switchCountShape().sortedById) {
        RTNode moved = node;

        moved.nodeID = moved.nodeID + offset;

        if (moved.nodeType != RTNode::NodeType::RootNode) {
            moved.parentId = moved.parentId + offset;
        }

        for (RTConnection& connection : moved.connections) {
            connection.childId = connection.childId + offset;
        }

        shifted.sortedById.push_back(moved);
    }

    std::vector<int> expected = walkPrimary(switchCountShape(), 1, steps, NativeTraversalRule::instance());

    for (int& id : expected) {
        id = id + offset;
    }

    CHECK(walkPrimary(shifted, 1 + offset, steps, NativeTraversalRule::instance()) == expected);
    CHECK(walkModulator(mirrorAsModulators(shifted), 1 + offset, steps) == expected);
}

TEST_CASE("every row of the state table holds its own value when every id collides", "[state]")
{
    NodeStateTable table;
    table.prepare();

    const int collidingStride = NodeRowMap::keyCapacity;

    for (int row = 0; row < NodeStateTable::maxNodeIds; ++row) {
        table.set(NodeStateSlot::Count, row * collidingStride, row + 1);
        table.set(NodeStateSlot::LastNode, row * collidingStride, row);
    }

    int mismatches = 0;

    for (int row = 0; row < NodeStateTable::maxNodeIds; ++row) {
        if (table.get(NodeStateSlot::Count, row * collidingStride) != row + 1
            || table.get(NodeStateSlot::LastNode, row * collidingStride) != row
            || table.get(NodeStateSlot::Trigger, row * collidingStride) != 0) {
            mismatches = mismatches + 1;
        }
    }

    CHECK(mismatches == 0);
    CHECK(table.get(NodeStateSlot::Count, 1) == 0);
}

TEST_CASE("clearing the state table restores every default and frees every row", "[state]")
{
    NodeStateTable table;
    table.prepare();

    for (int nodeId = 0; nodeId < NodeStateTable::maxNodeIds; ++nodeId) {
        table.set(NodeStateSlot::Count, nodeId, 5);
        table.set(NodeStateSlot::ActiveAlternative, nodeId, 9);
    }

    table.clear();

    CHECK(table.get(NodeStateSlot::Count, 0) == 0);
    CHECK(table.get(NodeStateSlot::ActiveAlternative, 0) == -1);
    CHECK(table.get(NodeStateSlot::SwitchCandidate, 0) == -1);

    const int freshIdOffset = 100000;

    for (int nodeId = 0; nodeId < NodeStateTable::maxNodeIds; ++nodeId) {
        table.increment(NodeStateSlot::Count, nodeId + freshIdOffset);
    }

    CHECK(table.get(NodeStateSlot::Count, freshIdOffset) == 1);
    CHECK(table.get(NodeStateSlot::Count, freshIdOffset + NodeStateTable::maxNodeIds - 1) == 1);
    CHECK(table.get(NodeStateSlot::Count, 0) == 0);
}

TEST_CASE("an unprepared state table drops every write and reads defaults", "[state]")
{
    NodeStateTable table;

    table.set(NodeStateSlot::Count, 5, 3);
    table.ref(NodeStateSlot::CrossTree, 5) = 7;

    CHECK(table.increment(NodeStateSlot::Count, 5) == 0);
    CHECK(table.get(NodeStateSlot::Count, 5) == 0);
    CHECK(table.get(NodeStateSlot::CrossTree, 5) == 0);
    CHECK(table.get(NodeStateSlot::LastNode, 5) == -1);
}

TEST_CASE("a script that declines every child sends the walker back to its root", "[traversal][script]")
{
    const ScriptCompileResult compiled = compileTraversalScript("return -1;");

    REQUIRE(compiled.succeeded());

    ScriptTraversalRule scriptRule;
    scriptRule.script = &compiled.script;

    const std::vector<int> expected { 1, 1, 1, 1 };

    CHECK(walkPrimary(chainShape(), 1, 3, scriptRule) == expected);
}

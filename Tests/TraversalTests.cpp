#include <catch2/catch_test_macros.hpp>

#include "Audio/ScriptTraversalRule.h"
#include "Audio/TraversalLogic.h"
#include "Script/ScriptCompiler.h"

#include <memory>
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
    NodeMap map;

    for (const RTNode& node : nodes) {
        map[node.nodeID] = std::make_shared<const RTNode>(node);
    }

    return map;
}

static NodeMap mirrorAsModulators(const NodeMap& tree)
{
    NodeMap mirror;

    for (const auto& [id, node] : tree) {
        RTNode modulator = *node;

        switch (node->nodeType) {
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

        mirror[id] = std::make_shared<const RTNode>(modulator);
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

    for (int step = 0; step < steps; ++step) {
        visited.push_back(logic.handleNodeEvent(nodes).enteredId);
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

    for (int step = 0; step < steps; ++step) {
        logic.decideNextModulator(nodes);
        logic.mod.step();
        logic.advanceAlternative(nodes, logic.mod.walker.target);

        visited.push_back(logic.mod.walker.target);
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
                                  triggerLimitShape(), weighted }) {
        CHECK(walkPrimary(nodes, 1, steps, scriptRule) == walkPrimary(nodes, 1, steps, NativeTraversalRule::instance()));
    }
}

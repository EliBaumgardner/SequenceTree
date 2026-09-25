#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Plugin/PluginProcessor.h"
#include "Graph/GraphState.h"
#include "Graph/RTGraphBuilder.h"
#include "Graph/ValueTreeIdentifiers.h"
#include "Audio/TraversalLogic.h"
#include "UI/Node/NodeFactory.h"

#include <vector>

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    return Catch::Session().run(argc, argv);
}

static const NodeMap& rebuildAndPublish(SequenceTreeAudioProcessor& processor)
{
    processor.rtGraphBuilder.rebuildAllGraphs();

    const AudioSnapshotPublisher::Snapshot* snapshot = processor.snapshots.getPublished();

    REQUIRE(snapshot != nullptr);
    REQUIRE(snapshot->globalNodes != nullptr);

    return *snapshot->globalNodes;
}

static int createRoot(GraphState& graph, int x, int y)
{
    NodeFactory::createRootNode(graph, NodePosition { x, y, 25 }, nullptr);

    const juce::ValueTree root = graph.nodeMap.getChild(graph.nodeMap.getNumChildren() - 1);

    REQUIRE(root.getType() == ValueTreeIdentifiers::RootNodeData);

    return root.getProperty(ValueTreeIdentifiers::Id);
}

static int createChild(GraphState& graph, int parentId, int x, int y)
{
    const juce::ValueTree child = NodeFactory::createNode(graph, parentId, ValueTreeIdentifiers::NodeData,
                                                          NodePosition { x, y, 25 }, nullptr);

    return child.getProperty(ValueTreeIdentifiers::Id);
}

TEST_CASE("arrow length sets the connection duration, and moving a node retimes it", "[graph]")
{
    SequenceTreeAudioProcessor processor;
    GraphState& graph = processor.graphState;

    const int rootId  = createRoot(graph, 0, 0);
    const int childId = createChild(graph, rootId, 100, 0);

    const NodeMap& built = rebuildAndPublish(processor);

    REQUIRE(built.find(rootId)->findConnection(childId) != nullptr);
    CHECK(built.find(rootId)->findConnection(childId)->duration == 500);

    GraphState::setNodePosition(graph.getNode(childId), NodePosition { 200, 0, 25 }, nullptr);
    const std::vector<int> movedIds { childId };
    processor.rtGraphBuilder.updateDurationMaps(movedIds);

    const NodeMap& moved = *processor.snapshots.getPublished()->globalNodes;

    CHECK(moved.find(rootId)->findConnection(childId)->duration == 1000);
}

TEST_CASE("moving a node retimes the arrow from every one of its parents", "[graph]")
{
    SequenceTreeAudioProcessor processor;
    GraphState& graph = processor.graphState;

    const int rootId         = createRoot(graph, 0, 0);
    const int firstParentId  = createChild(graph, rootId, 100, 0);
    const int secondParentId = createChild(graph, rootId, 100, 200);
    const int sharedChildId  = createChild(graph, firstParentId, 200, 0);

    graph.connectNodes(secondParentId, sharedChildId, nullptr);

    const NodeMap& built = rebuildAndPublish(processor);

    REQUIRE(built.find(secondParentId)->findConnection(sharedChildId) != nullptr);
    CHECK(built.find(firstParentId) ->findConnection(sharedChildId)->duration == 500);
    CHECK(built.find(secondParentId)->findConnection(sharedChildId)->duration == 500);

    GraphState::setNodePosition(graph.getNode(sharedChildId), NodePosition { 300, 0, 25 }, nullptr);
    const std::vector<int> movedIds { sharedChildId };
    processor.rtGraphBuilder.updateDurationMaps(movedIds);

    const NodeMap& moved = *processor.snapshots.getPublished()->globalNodes;

    CHECK(moved.find(firstParentId) ->findConnection(sharedChildId)->duration == 1000);
    CHECK(moved.find(secondParentId)->findConnection(sharedChildId)->duration == 1000);
}

TEST_CASE("a child directly below its parent is a chord link and is never stepped into", "[graph]")
{
    SequenceTreeAudioProcessor processor;
    GraphState& graph = processor.graphState;

    const int rootId  = createRoot(graph, 0, 0);
    const int chordId = createChild(graph, rootId, 0, 100);

    const NodeMap& nodes = rebuildAndPublish(processor);

    CHECK(nodes.find(rootId)->findConnection(chordId)->duration == 0);

    TraversalLogic logic;

    logic.nodeState.prepare();
    logic.reset(rootId, RTtraversal {});
    logic.begin(nodes, rootId, 0);

    for (int step = 0; step < 8; ++step) {
        CHECK(logic.handleNodeEvent(nodes).enteredId != chordId);
    }
}

TEST_CASE("a created child inherits its parent's limits", "[graph]")
{
    SequenceTreeAudioProcessor processor;
    GraphState& graph = processor.graphState;

    const int rootId   = createRoot(graph, 0, 0);
    const int parentId = createChild(graph, rootId, 100, 0);

    juce::ValueTree parent = graph.getNode(parentId);
    parent.setProperty(ValueTreeIdentifiers::CountLimit,       3, nullptr);
    parent.setProperty(ValueTreeIdentifiers::SwitchCountLimit, 2, nullptr);

    const int childId = createChild(graph, parentId, 200, 0);

    const NodeMap& nodes = rebuildAndPublish(processor);

    CHECK(nodes.find(childId)->countLimit       == 3);
    CHECK(nodes.find(childId)->switchCountLimit == 2);
}

TEST_CASE("a chain built through the graph walks in order and loops", "[graph][traversal]")
{
    SequenceTreeAudioProcessor processor;
    GraphState& graph = processor.graphState;

    const int rootId   = createRoot(graph, 0, 0);
    const int middleId = createChild(graph, rootId, 100, 0);
    const int leafId   = createChild(graph, middleId, 200, 0);

    const NodeMap& nodes = rebuildAndPublish(processor);

    TraversalLogic logic;

    logic.nodeState.prepare();
    logic.reset(rootId, RTtraversal {});
    logic.begin(nodes, rootId, 0);

    std::vector<int> visited { logic.primary.target };

    for (int step = 0; step < 6; ++step) {
        visited.push_back(logic.handleNodeEvent(nodes).enteredId);
    }

    const std::vector<int> expected { rootId, middleId, leafId, rootId, middleId, leafId, rootId };

    CHECK(visited == expected);
}

TEST_CASE("a modulator chain built through the graph walks like a node chain", "[graph][traversal][parity]")
{
    SequenceTreeAudioProcessor processor;
    GraphState& graph = processor.graphState;

    const int rootId   = createRoot(graph, 0, 0);
    const int middleId = createChild(graph, rootId, 100, 0);
    const int leafId   = createChild(graph, middleId, 200, 0);

    const juce::ValueTree modulatorRoot = NodeFactory::createModulatorRoot(graph, rootId, NodePosition { 0, 200, 25 }, nullptr);
    const int modulatorRootId = modulatorRoot.getProperty(ValueTreeIdentifiers::Id);

    const juce::ValueTree modulatorMiddle = NodeFactory::createModulator(graph, modulatorRootId, NodePosition { 100, 200, 25 }, nullptr);
    const int modulatorMiddleId = modulatorMiddle.getProperty(ValueTreeIdentifiers::Id);

    const juce::ValueTree modulatorLeaf = NodeFactory::createModulator(graph, modulatorMiddleId, NodePosition { 200, 200, 25 }, nullptr);
    const int modulatorLeafId = modulatorLeaf.getProperty(ValueTreeIdentifiers::Id);

    const NodeMap& nodes = rebuildAndPublish(processor);

    TraversalLogic logic;

    logic.nodeState.prepare();
    logic.reset(rootId, RTtraversal {});
    logic.mod.activate(modulatorRootId, rootId);
    logic.advanceAlternative(nodes, modulatorRootId);

    std::vector<int> visited { logic.mod.walker.target };

    for (int step = 0; step < 6; ++step) {
        logic.decideNextModulator(nodes);
        logic.mod.step();
        logic.advanceAlternative(nodes, logic.mod.walker.target);

        visited.push_back(logic.mod.walker.target);
    }

    const std::vector<int> expected { modulatorRootId, modulatorMiddleId, modulatorLeafId,
                                      modulatorRootId, modulatorMiddleId, modulatorLeafId, modulatorRootId };

    CHECK(visited == expected);
    CHECK(nodes.find(middleId) != nullptr);
    CHECK(nodes.find(leafId)   != nullptr);
}

TEST_CASE("moving a pitch-bound node transposes around the pitch last typed", "[graph][pitch]")
{
    SequenceTreeAudioProcessor processor;
    GraphState& graph = processor.graphState;

    const int rootId  = createRoot(graph, 0, 0);
    const int childId = createChild(graph, rootId, 100, 0);

    graph.arrows.syncPitchBindings(childId, nullptr);

    juce::ValueTree childNote = graph.getMidiNotes(childId).getChildWithName(ValueTreeIdentifiers::MidiNoteData);

    REQUIRE(childNote.isValid());

    const int startPitch = childNote.getProperty(ValueTreeIdentifiers::MidiPitch);

    GraphState::setNodePosition(graph.getNode(childId), NodePosition { 100, -100, 25 }, nullptr);
    graph.arrows.syncPitchBindings(childId, nullptr);

    CHECK(static_cast<int>(childNote.getProperty(ValueTreeIdentifiers::MidiPitch)) == startPitch + 2);

    childNote.setProperty(ValueTreeIdentifiers::MidiPitch, 70, nullptr);

    GraphState::setNodePosition(graph.getNode(childId), NodePosition { 100, 0, 25 }, nullptr);
    graph.arrows.syncPitchBindings(childId, nullptr);

    CHECK(static_cast<int>(childNote.getProperty(ValueTreeIdentifiers::MidiPitch)) == 68);

    const NodeMap& nodes = rebuildAndPublish(processor);

    REQUIRE_FALSE(nodes.find(childId)->notes.empty());
    CHECK(nodes.find(childId)->notes.front().pitch == 68);
}

TEST_CASE("undo and redo keep the node index, parent links and published graph in step", "[graph][undo]")
{
    SequenceTreeAudioProcessor processor;
    GraphState& graph = processor.graphState;
    juce::UndoManager undoManager;

    const int rootId = createRoot(graph, 0, 0);

    undoManager.beginNewTransaction();

    const juce::ValueTree child = NodeFactory::createNode(graph, rootId, ValueTreeIdentifiers::NodeData,
                                                          NodePosition { 100, 0, 25 }, &undoManager);
    const int childId = child.getProperty(ValueTreeIdentifiers::Id);

    const std::vector<int> rootOnly { rootId };

    undoManager.beginNewTransaction();
    graph.removeNode(childId, &undoManager);

    CHECK_FALSE(graph.getNode(childId).isValid());
    CHECK_FALSE(graph.getConnection(rootId, childId).isValid());
    CHECK(graph.parentIdsOf.count(childId) == 0);
    CHECK(rebuildAndPublish(processor).find(childId) == nullptr);

    undoManager.undo();

    REQUIRE(graph.getNode(childId).isValid());
    CHECK(graph.getConnection(rootId, childId).isValid());
    CHECK(graph.parentIdsOf[childId] == rootOnly);
    CHECK(rebuildAndPublish(processor).find(childId) != nullptr);

    undoManager.undo();

    CHECK_FALSE(graph.getNode(childId).isValid());
    CHECK(graph.parentIdsOf.count(childId) == 0);

    undoManager.redo();

    CHECK(graph.getNode(childId).isValid());
    CHECK(graph.parentIdsOf[childId] == rootOnly);

    undoManager.redo();

    CHECK_FALSE(graph.getNode(childId).isValid());
    CHECK(graph.parentIdsOf.count(childId) == 0);
}

TEST_CASE("removing an encapsulator removes its members and every link to them", "[graph][encapsulation]")
{
    SequenceTreeAudioProcessor processor;
    GraphState& graph = processor.graphState;

    const int rootId  = createRoot(graph, 0, 0);
    const int firstId = createChild(graph, rootId, 100, 0);
    const int lastId  = createChild(graph, firstId, 200, 0);
    const int afterId = createChild(graph, lastId, 300, 0);

    const std::vector<int> memberIds { firstId, lastId };
    const juce::ValueTree encapsulator = NodeFactory::createEncapsulator(graph, memberIds, 1, nullptr);
    const int encapsulatorId = encapsulator.getProperty(ValueTreeIdentifiers::Id);

    graph.removeNode(encapsulatorId, nullptr);

    CHECK_FALSE(graph.getNode(encapsulatorId).isValid());
    CHECK_FALSE(graph.getNode(firstId).isValid());
    CHECK_FALSE(graph.getNode(lastId).isValid());
    CHECK(graph.getNode(afterId).isValid());

    CHECK_FALSE(graph.getConnection(rootId, firstId).isValid());
    CHECK(graph.parentIdsOf.count(firstId) == 0);
    CHECK(graph.parentIdsOf.count(lastId)  == 0);
    CHECK(graph.parentIdsOf.count(afterId) == 0);

    const NodeMap& nodes = rebuildAndPublish(processor);

    CHECK(nodes.find(firstId) == nullptr);
    CHECK(nodes.find(lastId)  == nullptr);
}

TEST_CASE("dissolving an encapsulator keeps its members and their arrows", "[graph][encapsulation]")
{
    SequenceTreeAudioProcessor processor;
    GraphState& graph = processor.graphState;

    const int rootId  = createRoot(graph, 0, 0);
    const int firstId = createChild(graph, rootId, 100, 0);
    const int lastId  = createChild(graph, firstId, 200, 0);

    const std::vector<int> memberIds { firstId, lastId };
    const juce::ValueTree encapsulator = NodeFactory::createEncapsulator(graph, memberIds, 2, nullptr);
    const int encapsulatorId = encapsulator.getProperty(ValueTreeIdentifiers::Id);

    CHECK(graph.encapsulation.memberIds(encapsulatorId) == std::vector<int> { firstId, lastId });

    graph.encapsulation.dissolve(encapsulatorId, nullptr);

    CHECK_FALSE(graph.getNode(encapsulatorId).isValid());
    REQUIRE(graph.getNode(firstId).isValid());
    REQUIRE(graph.getNode(lastId).isValid());
    CHECK_FALSE(graph.getNode(firstId).hasProperty(ValueTreeIdentifiers::EncapsulatorId));
    CHECK_FALSE(graph.getNode(lastId).hasProperty(ValueTreeIdentifiers::EncapsulatorId));
    CHECK(graph.getConnection(rootId, firstId).isValid());
    CHECK(graph.getConnection(firstId, lastId).isValid());

    const NodeMap& nodes = rebuildAndPublish(processor);

    CHECK(nodes.find(firstId)->encapsulationEntryId == -1);
}

TEST_CASE("removing an encapsulator's last member dissolves it", "[graph][encapsulation]")
{
    SequenceTreeAudioProcessor processor;
    GraphState& graph = processor.graphState;

    const int rootId   = createRoot(graph, 0, 0);
    const int memberId = createChild(graph, rootId, 100, 0);

    const std::vector<int> memberIds { memberId };
    const juce::ValueTree encapsulator = NodeFactory::createEncapsulator(graph, memberIds, 1, nullptr);
    const int encapsulatorId = encapsulator.getProperty(ValueTreeIdentifiers::Id);

    graph.removeNode(memberId, nullptr);

    CHECK_FALSE(graph.getNode(encapsulatorId).isValid());
}

TEST_CASE("a node's parent is found through a chain of traversal flags", "[graph]")
{
    SequenceTreeAudioProcessor processor;
    GraphState& graph = processor.graphState;

    const int rootId = createRoot(graph, 0, 0);
    const int noteId = createChild(graph, rootId, 100, 0);

    const juce::ValueTree firstFlag = NodeFactory::createTraversalFlagNode(graph, noteId, NodePosition { 200, 0, 25 }, nullptr);
    const int firstFlagId = firstFlag.getProperty(ValueTreeIdentifiers::Id);

    const juce::ValueTree secondFlag = NodeFactory::createTraversalFlagNode(graph, firstFlagId, NodePosition { 300, 0, 25 }, nullptr);
    const int secondFlagId = secondFlag.getProperty(ValueTreeIdentifiers::Id);

    CHECK(static_cast<int>(graph.getNodeParent(secondFlagId).getProperty(ValueTreeIdentifiers::Id)) == noteId);
    CHECK(static_cast<int>(graph.getNodeParent(firstFlagId).getProperty(ValueTreeIdentifiers::Id))  == noteId);
    CHECK(static_cast<int>(graph.getNodeParent(noteId).getProperty(ValueTreeIdentifiers::Id))       == rootId);
    CHECK_FALSE(graph.getNodeParent(rootId).isValid());
}

TEST_CASE("a restored graph indexes, numbers and walks like the one it was saved from", "[graph][traversal]")
{
    SequenceTreeAudioProcessor original;
    SequenceTreeAudioProcessor restored;

    const int rootId   = createRoot(original.graphState, 0, 0);
    const int everyId  = createChild(original.graphState, rootId, 100, 0);
    const int secondId = createChild(original.graphState, rootId, 100, 100);

    original.graphState.getNode(secondId).setProperty(ValueTreeIdentifiers::CountLimit, 2, nullptr);

    restored.graphState.replaceState(original.graphState.nodeMap, original.graphState.traversals.map);

    CHECK(restored.graphState.nodeIdIncrement == original.graphState.nodeIdIncrement);
    CHECK(restored.graphState.parentIdsOf     == original.graphState.parentIdsOf);
    CHECK(restored.graphState.getNode(everyId).isValid());

    std::vector<std::vector<int>> walks;

    for (SequenceTreeAudioProcessor* processor : { &original, &restored }) {
        const NodeMap& nodes = rebuildAndPublish(*processor);

        TraversalLogic logic;

        logic.nodeState.prepare();
        logic.reset(rootId, RTtraversal {});
        logic.begin(nodes, rootId, 0);

        std::vector<int> visited { logic.primary.target };

        for (int step = 0; step < 8; ++step) {
            visited.push_back(logic.handleNodeEvent(nodes).enteredId);
        }

        walks.push_back(visited);
    }

    const std::vector<int> expected { rootId, everyId, rootId, secondId, rootId, everyId, rootId, secondId, rootId };

    CHECK(walks[0] == expected);
    CHECK(walks[1] == expected);
}

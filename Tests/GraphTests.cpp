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

    REQUIRE(built.at(rootId)->findConnection(childId) != nullptr);
    CHECK(built.at(rootId)->findConnection(childId)->duration == 500);

    GraphState::setNodePosition(graph.getNode(childId), NodePosition { 200, 0, 25 }, nullptr);
    processor.rtGraphBuilder.updateDurationMaps({ childId });

    const NodeMap& moved = *processor.snapshots.getPublished()->globalNodes;

    CHECK(moved.at(rootId)->findConnection(childId)->duration == 1000);
}

TEST_CASE("a child directly below its parent is a chord link and is never stepped into", "[graph]")
{
    SequenceTreeAudioProcessor processor;
    GraphState& graph = processor.graphState;

    const int rootId  = createRoot(graph, 0, 0);
    const int chordId = createChild(graph, rootId, 0, 100);

    const NodeMap& nodes = rebuildAndPublish(processor);

    CHECK(nodes.at(rootId)->findConnection(chordId)->duration == 0);

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

    CHECK(nodes.at(childId)->countLimit       == 3);
    CHECK(nodes.at(childId)->switchCountLimit == 2);
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
    CHECK(nodes.count(middleId) == 1);
    CHECK(nodes.count(leafId)   == 1);
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

    CHECK((int) childNote.getProperty(ValueTreeIdentifiers::MidiPitch) == startPitch + 2);

    childNote.setProperty(ValueTreeIdentifiers::MidiPitch, 70, nullptr);

    GraphState::setNodePosition(graph.getNode(childId), NodePosition { 100, 0, 25 }, nullptr);
    graph.arrows.syncPitchBindings(childId, nullptr);

    CHECK((int) childNote.getProperty(ValueTreeIdentifiers::MidiPitch) == 68);

    const NodeMap& nodes = rebuildAndPublish(processor);

    REQUIRE_FALSE(nodes.at(childId)->notes.empty());
    CHECK(nodes.at(childId)->notes.front().pitch == 68);
}

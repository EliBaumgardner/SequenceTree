#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Plugin/PluginProcessor.h"
#include "Graph/GraphState.h"
#include "Graph/RTGraphBuilder.h"
#include "Graph/ValueTreeIdentifiers.h"
#include "Audio/TraversalLogic.h"
#include "Input/ConnectionOps.h"
#include "Script/ScriptCompiler.h"
#include "UI/Node/NodeFactory.h"

#include <memory>
#include <string>
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

    graph.setNodePosition(graph.getNode(childId), NodePosition { 200, 0, 25 }, nullptr);
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

    graph.setNodePosition(graph.getNode(sharedChildId), NodePosition { 300, 0, 25 }, nullptr);
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

TEST_CASE("only a note node's arrow into another tree's root is a root connection", "[graph][connection]")
{
    SequenceTreeAudioProcessor processor;
    GraphState& graph = processor.graphState;

    const int rootId        = createRoot(graph, 0, 0);
    const int nodeId        = createChild(graph, rootId, 100, 0);
    const int foreignRootId = createRoot(graph, 0, 400);

    const juce::ValueTree modulatorRoot = NodeFactory::createModulatorRoot(graph, nodeId, NodePosition { 100, 200, 25 }, nullptr);
    const int modulatorRootId = modulatorRoot.getProperty(ValueTreeIdentifiers::Id);

    const juce::ValueTree modulator = NodeFactory::createModulator(graph, modulatorRootId, NodePosition { 200, 200, 25 }, nullptr);
    const int modulatorId = modulator.getProperty(ValueTreeIdentifiers::Id);

    const juce::ValueTree alternativeModulator = NodeFactory::createAlternativeModulator(graph, modulatorId, NodePosition { 200, 300, 25 }, nullptr);
    const int alternativeModulatorId = alternativeModulator.getProperty(ValueTreeIdentifiers::Id);

    ApplicationContext context;
    context.graphState = &graph;

    const ConnectionOps connections { context };

    CHECK(connections.connectsToOtherTreeRoot(nodeId, foreignRootId));
    CHECK_FALSE(connections.connectsToOtherTreeRoot(nodeId, rootId));
    CHECK_FALSE(connections.connectsToOtherTreeRoot(modulatorRootId, foreignRootId));
    CHECK_FALSE(connections.connectsToOtherTreeRoot(modulatorId, foreignRootId));
    CHECK_FALSE(connections.connectsToOtherTreeRoot(alternativeModulatorId, foreignRootId));
}

TEST_CASE("the published graph names its first unlinked root", "[graph]")
{
    SequenceTreeAudioProcessor processor;
    GraphState& graph = processor.graphState;

    const int firstRootId   = createRoot(graph, 0, 0);
    const int secondRootId  = createRoot(graph, 0, 400);
    const int firstChildId  = createChild(graph, firstRootId, 100, 0);
    const int secondChildId = createChild(graph, secondRootId, 100, 400);

    REQUIRE(firstRootId < secondRootId);

    CHECK(rebuildAndPublish(processor).firstUnlinkedRootId == firstRootId);

    graph.connectNodes(firstChildId, secondRootId, nullptr);
    CHECK(rebuildAndPublish(processor).firstUnlinkedRootId == firstRootId);

    graph.connectNodes(secondChildId, firstRootId, nullptr);
    CHECK(rebuildAndPublish(processor).firstUnlinkedRootId == -1);

    graph.disconnectNodes(firstChildId, secondRootId, nullptr);
    CHECK(rebuildAndPublish(processor).firstUnlinkedRootId == secondRootId);
}

TEST_CASE("moving a pitch-bound node transposes around the pitch last typed", "[graph][pitch]")
{
    SequenceTreeAudioProcessor processor;
    GraphState& graph = processor.graphState;

    const int rootId  = createRoot(graph, 0, 0);
    const int childId = createChild(graph, rootId, 100, 0);

    juce::ValueTree childNote = graph.getMidiNotes(childId).getChildWithName(ValueTreeIdentifiers::MidiNoteData);

    REQUIRE(childNote.isValid());

    const int startPitch = childNote.getProperty(ValueTreeIdentifiers::MidiPitch);

    graph.setNodePosition(graph.getNode(childId), NodePosition { 100, -100, 25 }, nullptr);

    CHECK(static_cast<int>(childNote.getProperty(ValueTreeIdentifiers::MidiPitch)) == startPitch + 2);

    childNote.setProperty(ValueTreeIdentifiers::MidiPitch, 70, nullptr);

    graph.setNodePosition(graph.getNode(childId), NodePosition { 100, 0, 25 }, nullptr);

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

TEST_CASE("undoing a move restores the arrow's duration and keeps the move redoable", "[graph][undo]")
{
    SequenceTreeAudioProcessor processor;
    GraphState& graph = processor.graphState;
    juce::UndoManager& undoManager = processor.undoManager;

    const int rootId  = createRoot(graph, 0, 0);
    const int childId = createChild(graph, rootId, 100, 0);

    juce::ValueTree connection = graph.getConnection(rootId, childId);

    undoManager.beginNewTransaction();
    connection.setProperty(ValueTreeIdentifiers::ArrowDuration, 700, &undoManager);

    undoManager.beginNewTransaction();
    graph.setNodePosition(graph.getNode(childId), NodePosition { 200, 0, 25 }, &undoManager);

    CHECK(static_cast<int>(connection.getProperty(ValueTreeIdentifiers::ArrowDuration)) == ArrowInfo::noDurationOverride);

    processor.rtGraphBuilder.handleUpdateNowIfNeeded();
    undoManager.undo();
    processor.rtGraphBuilder.handleUpdateNowIfNeeded();

    CHECK(graph.getNodePosition(childId).xPosition == 100);
    CHECK(static_cast<int>(connection.getProperty(ValueTreeIdentifiers::ArrowDuration)) == 700);
    CHECK(processor.snapshots.getPublished()->globalNodes->find(rootId)->findConnection(childId)->duration == 700);
    REQUIRE(undoManager.canRedo());

    undoManager.redo();
    processor.rtGraphBuilder.handleUpdateNowIfNeeded();

    CHECK(graph.getNodePosition(childId).xPosition == 200);
    CHECK(static_cast<int>(connection.getProperty(ValueTreeIdentifiers::ArrowDuration)) == ArrowInfo::noDurationOverride);
    CHECK(processor.snapshots.getPublished()->globalNodes->find(rootId)->findConnection(childId)->duration == 1000);
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

struct HostPlayHead : juce::AudioPlayHead
{
    PositionInfo position;

    juce::Optional<PositionInfo> getPosition() const override
    {
        return position;
    }
};

struct PlayedNote
{
    int pitch    = 0;
    int velocity = 0;
    int sample   = 0;

    bool operator==(const PlayedNote&) const = default;
};

static std::vector<PlayedNote> playChainWithScript(const char* scriptSource)
{
    constexpr double sampleRate = 48000.0;
    constexpr int    blockSize  = 512;
    constexpr int    blockCount = 400;

    SequenceTreeAudioProcessor processor;
    GraphState& graph = processor.graphState;

    HostPlayHead             playHead;
    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer         midi;
    std::vector<PlayedNote>  played;
    double                   ppq = 0.0;

    const int rootId  = createRoot(graph, 0, 0);
    const int firstId = createChild(graph, rootId, 100, 0);

    createChild(graph, firstId, 220, 0);
    rebuildAndPublish(processor);

    processor.snapshots.publishScript(nullptr);

    if (scriptSource != nullptr) {
        ScriptCompileResult compiled = compileTraversalScript(scriptSource);

        REQUIRE(compiled.succeeded());

        processor.snapshots.publishScript(std::make_shared<RTScript>(std::move(compiled.script)));
    }

    playHead.position.setIsPlaying(true);
    playHead.position.setBpm(120.0);

    processor.setPlayHead(&playHead);
    processor.prepareToPlay(sampleRate, blockSize);

    midi.ensureSize(2048);

    for (int block = 0; block < blockCount; ++block) {
        playHead.position.setPpqPosition(ppq);
        processor.processBlock(buffer, midi);

        ppq += blockSize / sampleRate * *playHead.position.getBpm() / 60.0;

        for (const auto event : midi) {
            if (event.getMessage().isNoteOn()) {
                played.push_back({ event.getMessage().getNoteNumber(), event.getMessage().getVelocity(),
                                   block * blockSize + event.samplePosition });
            }
        }
    }

    processor.releaseResources();
    processor.setPlayHead(nullptr);

    return played;
}

static std::string traversalWithMain(const std::string& mainBody)
{
    return "class Traversal : defaultTraversal {\npublic:\n    Node selectedNode;\n    void main() {\n"
           + mainBody + "\n    }\n};\n";
}

TEST_CASE("the default script plays exactly what the built-in traversal plays", "[graph][script]")
{
    const std::vector<PlayedNote> builtIn = playChainWithScript(nullptr);

    REQUIRE(builtIn.size() > 4);
    CHECK(playChainWithScript(defaultTraversalScriptSource()) == builtIn);
}

TEST_CASE("main's playNote values reach the MIDI output, and the rhythm is unchanged", "[graph][script]")
{
    const std::vector<PlayedNote> builtIn = playChainWithScript(nullptr);

    const std::string shifted = traversalWithMain(
        "        selectedNode = advance(1);\n"
        "        int velocity = selectedNode.velocity;\n"
        "        velocity += 10;\n"
        "        playNote(selectedNode, selectedNode.pitch - 12, selectedNode.duration, velocity);");

    const std::vector<PlayedNote> played = playChainWithScript(shifted.c_str());

    REQUIRE(played.size() == builtIn.size());

    CHECK(played.front() == builtIn.front());

    for (std::size_t index = 1; index < played.size(); ++index) {
        CAPTURE(index);
        CHECK(played[index].sample   == builtIn[index].sample);
        CHECK(played[index].pitch    == builtIn[index].pitch - 12);
        CHECK(played[index].velocity == juce::jlimit(0, 127, builtIn[index].velocity + 10));
    }
}

TEST_CASE("a main that never plays is silent, and the walk still keeps time", "[graph][script]")
{
    const std::vector<PlayedNote> builtIn = playChainWithScript(nullptr);

    const std::string silentEveryOther = traversalWithMain(
        "        selectedNode = advance(1);\n"
        "        if selectedNode.id % 2 == 0 {\n"
        "            playNote(selectedNode, selectedNode.pitch, selectedNode.duration, selectedNode.velocity);\n"
        "        }");

    const std::vector<PlayedNote> played = playChainWithScript(silentEveryOther.c_str());

    CHECK(playChainWithScript(traversalWithMain("        selectedNode = advance(1);").c_str()).size() == 1);
    CHECK(played.size() < builtIn.size());
    CHECK(played.size() > 1);

    for (const PlayedNote& note : played) {
        CHECK(std::ranges::find(builtIn, note) != builtIn.end());
    }
}

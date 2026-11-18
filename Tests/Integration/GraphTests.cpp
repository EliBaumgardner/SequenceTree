#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Processor/PluginProcessor.h"
#include "Processor/Document/Graph/Clipboard.h"
#include "Processor/Document/Graph/GraphState.h"
#include "Processor/AudioModel/RTGraphBuilder.h"
#include "Processor/Document/TypedProperties.h"
#include "Processor/Document/ValueTreeIdentifiers.h"
#include <ArsNovaUI/Canvas/CanvasIdentifiers.h>
#include <ArsNovaUI/Theme/InterfaceIdentifiers.h>
#include "Core/Traversal/TraversalLogic.h"
#include "Core/Traversal/TraversalPool.h"
#include "Core/Traversal/TraversalRule.h"
#include "Core/Script/ScriptCompiler.h"
#include "Processor/Document/Graph/NodeFactory.h"
#include "Processor/Document/Graph/ArrowBindingOps.h"
#include "Processor/Document/StateJson.h"
#include "Processor/Document/Ornaments/OrnamentLibraries.h"
#include "Processor/Document/Ornaments/OrnamentFigure.h"
#include "Editor/Canvas/GraphDocument.h"

#include <memory>
#include <cstdlib>
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

    const Snapshot* snapshot = processor.snapshots.getPublished();

    REQUIRE(snapshot != nullptr);
    REQUIRE(snapshot->globalNodes != nullptr);

    return *snapshot->globalNodes;
}

static int createRoot(GraphState& graph, int x, int y)
{
    NodeFactory(graph, nullptr).createRootNode(NodeInfo::Position { x, y, 25 });

    const juce::ValueTree root = graph.nodeMap.getChild(graph.nodeMap.getNumChildren() - 1);

    REQUIRE(root.getType() == ValueTreeIdentifiers::RootNodeData);

    return root.getProperty(ArsNovaUI::CanvasIdentifiers::Id);
}

static int createChild(GraphState& graph, int parentId, int x, int y)
{
    const juce::ValueTree child = NodeFactory(graph, nullptr).createNode(parentId, ValueTreeIdentifiers::NodeData, NodeInfo::Position { x, y, 25 });

    return child.getProperty(ArsNovaUI::CanvasIdentifiers::Id);
}

TEST_CASE("arrow length sets the connection duration, and moving a node retimes it", "[graph]")
{
    SequenceTreeAudioProcessor processor;
    GraphState& graph = processor.document.graphState;

    const int rootId  = createRoot(graph, 0, 0);
    const int childId = createChild(graph, rootId, 100, 0);

    const NodeMap& built = rebuildAndPublish(processor);

    REQUIRE(built.find(rootId)->findConnection(childId) != nullptr);
    CHECK(built.find(rootId)->findConnection(childId)->duration == 500);

    graph.setNodePosition(graph.getNode(childId), NodeInfo::Position { 200, 0, 25 }, nullptr);
    const std::vector<int> movedIds { childId };
    processor.rtGraphBuilder.updateDurationMaps(movedIds);

    const NodeMap& moved = *processor.snapshots.getPublished()->globalNodes;

    CHECK(moved.find(rootId)->findConnection(childId)->duration == 1000);
}

TEST_CASE("moving a node retimes the arrow from every one of its parents", "[graph]")
{
    SequenceTreeAudioProcessor processor;
    GraphState& graph = processor.document.graphState;

    const int rootId         = createRoot(graph, 0, 0);
    const int firstParentId  = createChild(graph, rootId, 100, 0);
    const int secondParentId = createChild(graph, rootId, 100, 200);
    const int sharedChildId  = createChild(graph, firstParentId, 200, 0);

    graph.connectNodes(secondParentId, sharedChildId, nullptr);

    const NodeMap& built = rebuildAndPublish(processor);

    REQUIRE(built.find(secondParentId)->findConnection(sharedChildId) != nullptr);
    CHECK(built.find(firstParentId) ->findConnection(sharedChildId)->duration == 500);
    CHECK(built.find(secondParentId)->findConnection(sharedChildId)->duration == 500);

    graph.setNodePosition(graph.getNode(sharedChildId), NodeInfo::Position { 300, 0, 25 }, nullptr);
    const std::vector<int> movedIds { sharedChildId };
    processor.rtGraphBuilder.updateDurationMaps(movedIds);

    const NodeMap& moved = *processor.snapshots.getPublished()->globalNodes;

    CHECK(moved.find(firstParentId) ->findConnection(sharedChildId)->duration == 1000);
    CHECK(moved.find(secondParentId)->findConnection(sharedChildId)->duration == 1000);
}

TEST_CASE("a child directly below its parent is a chord link and is never stepped into", "[graph]")
{
    SequenceTreeAudioProcessor processor;
    GraphState& graph = processor.document.graphState;

    const int rootId  = createRoot(graph, 0, 0);
    const int chordId = createChild(graph, rootId, 0, 100);

    const NodeMap& nodes = rebuildAndPublish(processor);

    CHECK(nodes.find(rootId)->findConnection(chordId)->duration == 0);

    TraversalLogic logic;

    logic.childSelector.nodeState.prepare();
    logic.reset(rootId, RTtraversal {});
    logic.begin(nodes, rootId, 0);

    for (int step = 0; step < 8; ++step) {
        CHECK(logic.handleNodeEvent(nodes).enteredId != chordId);
    }
}

TEST_CASE("a created child inherits its parent's limits", "[graph]")
{
    SequenceTreeAudioProcessor processor;
    GraphState& graph = processor.document.graphState;

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

TEST_CASE("recolouring a node colours its descendants, is inherited by new children, and undoes as one step", "[graph][colour]")
{
    SequenceTreeAudioProcessor processor;
    GraphState& graph = processor.document.graphState;
    juce::UndoManager undoManager;

    const int rootId       = createRoot(graph, 0, 0);
    const int recolouredId = createChild(graph, rootId, 100, 0);
    const int descendantId = createChild(graph, recolouredId, 200, 0);
    const int siblingId    = createChild(graph, rootId, 100, 100);

    undoManager.beginNewTransaction();
    graph.setNodeColour(recolouredId, "ff336699", &undoManager);

    CHECK(graph.getNode(recolouredId).getProperty(ValueTreeIdentifiers::NodeColour).toString() == "ff336699");
    CHECK(graph.getNode(descendantId).getProperty(ValueTreeIdentifiers::NodeColour).toString() == "ff336699");
    CHECK_FALSE(graph.getNode(rootId).hasProperty(ValueTreeIdentifiers::NodeColour));
    CHECK_FALSE(graph.getNode(siblingId).hasProperty(ValueTreeIdentifiers::NodeColour));

    const int newChildId = createChild(graph, descendantId, 300, 0);

    CHECK(graph.getNode(newChildId).getProperty(ValueTreeIdentifiers::NodeColour).toString() == "ff336699");

    undoManager.undo();

    CHECK_FALSE(graph.getNode(recolouredId).hasProperty(ValueTreeIdentifiers::NodeColour));
    CHECK_FALSE(graph.getNode(descendantId).hasProperty(ValueTreeIdentifiers::NodeColour));
}

TEST_CASE("node colours and colour presets survive saving and restoring the plugin state", "[graph][colour]")
{
    SequenceTreeAudioProcessor original;
    SequenceTreeAudioProcessor restored;
    juce::MemoryBlock          savedState;
    juce::ValueTree            preset { ArsNovaUI::InterfaceIdentifiers::ColourPreset };
    juce::TemporaryFile        patchFile { StateJson::patchExtension };

    const int rootId = createRoot(original.document.graphState, 0, 0);

    original.document.patch.setProperty(ValueTreeIdentifiers::PatchFile, patchFile.getFile().getFullPathName(), nullptr);

    original.document.graphState.setNodeColour(rootId, "ff993300", nullptr);

    preset.setProperty(ArsNovaUI::InterfaceIdentifiers::PresetColour, "ff00ff00", nullptr);

    original.document.colourPresets.appendChild(preset, nullptr);

    original.getStateInformation(savedState);

    restored.setStateInformation(savedState.getData(), static_cast<int>(savedState.getSize()));

    CHECK(restored.document.graphState.getNode(rootId).getProperty(ValueTreeIdentifiers::NodeColour).toString() == "ff993300");
    REQUIRE(restored.document.colourPresets.getNumChildren() == 1);
    CHECK(restored.document.colourPresets.getChild(0).getProperty(ArsNovaUI::InterfaceIdentifiers::PresetColour).toString() == "ff00ff00");
}

TEST_CASE("the grid multipliers survive saving and restoring the plugin state", "[graph][canvas]")
{
    SequenceTreeAudioProcessor original;
    SequenceTreeAudioProcessor restored;
    juce::MemoryBlock          savedState;
    juce::TemporaryFile        patchFile { StateJson::patchExtension };

    original.document.patch.setProperty(ValueTreeIdentifiers::PatchFile, patchFile.getFile().getFullPathName(), nullptr);

    original.document.canvasSettings.setProperty(ValueTreeIdentifiers::GraphGridMultiplier, 3.5, nullptr);
    original.document.canvasSettings.setProperty(ValueTreeIdentifiers::OrnamentGridMultiplier, 1.5, nullptr);

    original.getStateInformation(savedState);

    restored.setStateInformation(savedState.getData(), static_cast<int>(savedState.getSize()));

    CHECK(static_cast<double>(restored.document.canvasSettings.getProperty(ValueTreeIdentifiers::GraphGridMultiplier)) == 3.5);
    CHECK(static_cast<double>(restored.document.canvasSettings.getProperty(ValueTreeIdentifiers::OrnamentGridMultiplier)) == 1.5);
}

struct LegacyStateWriter : SequenceTreeAudioProcessor
{
    static void write(const juce::XmlElement& element, juce::MemoryBlock& destination)
    {
        copyXmlToBinary(element, destination);
    }
};

TEST_CASE("the plugin state is saved as JSON text", "[graph]")
{
    SequenceTreeAudioProcessor original;
    juce::MemoryBlock          savedState;
    juce::TemporaryFile        patchFile { StateJson::patchExtension };

    original.document.patch.setProperty(ValueTreeIdentifiers::PatchFile, patchFile.getFile().getFullPathName(), nullptr);

    original.document.canvasSettings.setProperty(ValueTreeIdentifiers::GraphGridMultiplier, 3.5, nullptr);

    original.getStateInformation(savedState);

    const juce::String savedText = savedState.toString();

    juce::var parsed;

    REQUIRE(juce::JSON::parse(savedText, parsed).wasOk());
    CHECK(parsed.getProperty("type", juce::var()).toString() == ValueTreeIdentifiers::PluginState.toString());
    CHECK(parsed.getProperty("children", juce::var()).size() > 0);
}

TEST_CASE("a state saved in the old binary format still restores", "[graph]")
{
    SequenceTreeAudioProcessor original;
    SequenceTreeAudioProcessor restored;
    juce::MemoryBlock          jsonState;
    juce::MemoryBlock          binaryState;
    juce::TemporaryFile        patchFile { StateJson::patchExtension };

    original.document.patch.setProperty(ValueTreeIdentifiers::PatchFile, patchFile.getFile().getFullPathName(), nullptr);

    original.document.canvasSettings.setProperty(ValueTreeIdentifiers::GraphGridMultiplier, 3.5, nullptr);
    original.document.canvasSettings.setProperty(ValueTreeIdentifiers::OrnamentGridMultiplier, 1.5, nullptr);

    original.getStateInformation(jsonState);

    const std::unique_ptr<juce::XmlElement> element = StateJson::read(jsonState.getData(), static_cast<int>(jsonState.getSize()));

    REQUIRE(element != nullptr);

    LegacyStateWriter::write(*element, binaryState);

    restored.setStateInformation(binaryState.getData(), static_cast<int>(binaryState.getSize()));

    CHECK(static_cast<double>(restored.document.canvasSettings.getProperty(ValueTreeIdentifiers::GraphGridMultiplier)) == 3.5);
    CHECK(static_cast<double>(restored.document.canvasSettings.getProperty(ValueTreeIdentifiers::OrnamentGridMultiplier)) == 1.5);
}

TEST_CASE("every node property in the table is set on creation or inherited exactly as its row says", "[graph][properties]")
{
    GraphState graph;

    const int rootId = createRoot(graph, 0, 0);

    for (const NodePropertyTable::Row& row : NodePropertyTable::rows) {
        if (row.isSetOnCreation) {
            CHECK(graph.getNode(rootId).hasProperty(row.identifier));
            CHECK(static_cast<int>(graph.getNode(rootId).getProperty(row.identifier)) == row.initialValue);
        }
    }

    for (const NodePropertyTable::Row& row : NodePropertyTable::rows) {
        const int changedValue = row.initialValue + 3;

        if (row.isInheritedByChild) {
            graph.getNode(rootId).setProperty(row.identifier, changedValue, nullptr);
        }
    }

    const int childId = createChild(graph, rootId, 100, 0);

    for (const NodePropertyTable::Row& row : NodePropertyTable::rows) {
        const int childValue = graph.getNode(childId).getProperty(row.identifier, -999);

        if (row.isInheritedByChild) {
            CHECK(childValue == row.initialValue + 3);
        }
        else if (row.isSetOnCreation) {
            CHECK(childValue == row.initialValue);
        }
    }
}

TEST_CASE("making another rule live publishes its script with no rules window open", "[graph][rules]")
{
    SequenceTreeAudioProcessor processor;

    const std::string sourceWithGlobal = std::string(defaultTraversalScriptSource()) + "\nint marker = 3;\n";

    const int ruleId = processor.document.traversalRuleState.addRule(0, nullptr).getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    processor.document.traversalRuleState.setRuleSource(ruleId, sourceWithGlobal, nullptr);

    REQUIRE(processor.snapshots.getPublished()->traversalScript != nullptr);

    const std::uint64_t generationBefore = processor.snapshots.getPublished()->traversalScript->generation;

    processor.document.traversalRuleState.activeRuleId.setValue(ruleId, nullptr);

    const RTScript* const published = processor.snapshots.getPublished()->traversalScript.get();

    REQUIRE(published != nullptr);

    CHECK(published->generation != generationBefore);
    CHECK(published->globalDefaults.size() == 1);
}

TEST_CASE("typing in the live rule publishes its script once the typing pauses", "[graph][rules]")
{
    SequenceTreeAudioProcessor processor;

    TraversalRuleState& ruleState = processor.document.traversalRuleState;

    const std::string sourceWithGlobal = std::string(defaultTraversalScriptSource()) + "\nint marker = 3;\n";

    const int liveRuleId       = ruleState.activeRuleId;
    const int pollIntervalMs   = 50;
    const int publishTimeoutMs = 5000;

    const std::uint64_t generationBefore = processor.snapshots.getPublished()->traversalScript->generation;

    ruleState.setRuleSource(liveRuleId, sourceWithGlobal, nullptr);

    CHECK(processor.snapshots.getPublished()->traversalScript->generation == generationBefore);

    const juce::uint32 publishDeadlineMs = juce::Time::getMillisecondCounter() + publishTimeoutMs;

    while (processor.snapshots.getPublished()->traversalScript->generation == generationBefore && juce::Time::getMillisecondCounter() < publishDeadlineMs) {
        juce::Thread::sleep(pollIntervalMs);

        juce::Timer::callPendingTimersSynchronously();
    }

    const RTScript* const published = processor.snapshots.getPublished()->traversalScript.get();

    REQUIRE(published != nullptr);

    CHECK(published->generation != generationBefore);
    CHECK(published->globalDefaults.size() == 1);
}

TEST_CASE("typing in a rule that is not live publishes nothing", "[graph][rules]")
{
    SequenceTreeAudioProcessor processor;

    TraversalRuleState& ruleState = processor.document.traversalRuleState;

    const std::string sourceWithGlobal = std::string(defaultTraversalScriptSource()) + "\nint marker = 3;\n";

    const int otherRuleId = ruleState.addRule(0, nullptr).getProperty(ArsNovaUI::CanvasIdentifiers::Id);
    const int pauseWaitMs = 600;

    const std::uint64_t generationBefore = processor.snapshots.getPublished()->traversalScript->generation;

    ruleState.setRuleSource(otherRuleId, sourceWithGlobal, nullptr);

    juce::Thread::sleep(pauseWaitMs);

    juce::Timer::callPendingTimersSynchronously();

    CHECK(processor.snapshots.getPublished()->traversalScript->generation == generationBefore);
}

TEST_CASE("a rule change that leaves the live rule and its text alone publishes nothing", "[graph][rules]")
{
    SequenceTreeAudioProcessor processor;

    TraversalRuleState& ruleState = processor.document.traversalRuleState;

    const int liveRuleId = ruleState.activeRuleId;

    const std::uint64_t generationBefore = processor.snapshots.getPublished()->traversalScript->generation;

    ruleState.setRuleName(liveRuleId, "renamed", nullptr);

    CHECK(processor.snapshots.getPublished()->traversalScript->generation == generationBefore);

    const int otherRuleId = ruleState.addRule(0, nullptr).getProperty(ArsNovaUI::CanvasIdentifiers::Id);
    const int folderId    = ruleState.addFolder(0, nullptr).getProperty(ValueTreeIdentifiers::FolderId);

    CHECK(processor.snapshots.getPublished()->traversalScript->generation == generationBefore);

    ruleState.moveRule(otherRuleId, folderId, -1, nullptr);
    ruleState.moveRule(otherRuleId, 0, liveRuleId, nullptr);

    CHECK(processor.snapshots.getPublished()->traversalScript->generation == generationBefore);

    ruleState.removeRule(otherRuleId, nullptr);
    ruleState.removeFolder(folderId, nullptr);

    CHECK(processor.snapshots.getPublished()->traversalScript->generation == generationBefore);
}

TEST_CASE("processBlock leaves a restore that is still pending for the message thread", "[graph][state]")
{
    constexpr double sampleRate = 48000.0;
    constexpr int    blockSize  = 512;

    SequenceTreeAudioProcessor processor;

    juce::AudioBuffer<float> buffer(2, blockSize);

    juce::MidiBuffer midi;

    createRoot(processor.document.graphState, 0, 0);

    processor.pendingRestoreState = juce::ValueTree(ValueTreeIdentifiers::NodeMap);

    processor.prepareToPlay(sampleRate, blockSize);
    processor.processBlock(buffer, midi);
    processor.releaseResources();

    CHECK(processor.pendingRestoreState.isValid());
    CHECK(processor.document.graphState.nodeMap.getNumChildren() == 1);
}

TEST_CASE("a session saved before the schema version existed still restores and is saved with the current version", "[graph][state]")
{
    SequenceTreeAudioProcessor processor;
    juce::MemoryBlock          resavedState;

    const juce::File fixture = juce::File(__FILE__).getParentDirectory().getParentDirectory().getChildFile("Fixtures/savedBeforeSchemaVersion.sqtree");

    REQUIRE(fixture.existsAsFile());

    juce::MemoryBlock fixtureBytes;

    REQUIRE(fixture.loadFileAsData(fixtureBytes));

    const int fixtureSize = static_cast<int>(fixtureBytes.getSize());

    processor.setStateInformation(fixtureBytes.getData(), fixtureSize);

    REQUIRE(processor.document.graphState.nodeMap.getNumChildren() == 1);

    CHECK(processor.document.graphState.getNode(1).getProperty(ValueTreeIdentifiers::NodeColour).toString() == "ff993300");
    CHECK(static_cast<double>(processor.document.canvasSettings.getProperty(ValueTreeIdentifiers::GraphGridMultiplier)) == 3.5);

    processor.getStateInformation(resavedState);

    const std::unique_ptr<juce::XmlElement> resaved = StateJson::read(resavedState.getData(), static_cast<int>(resavedState.getSize()));

    REQUIRE(resaved != nullptr);

    CHECK(resaved->getIntAttribute(ValueTreeIdentifiers::SchemaVersion.toString()) == PluginDocument::schemaVersion);
}

TEST_CASE("a session saved by a newer schema version is not loaded", "[graph][state]")
{
    SequenceTreeAudioProcessor processor;

    const juce::String newerState = "{\"type\":\"PluginState\",\"properties\":{\"SchemaVersion\":\"" + juce::String(PluginDocument::schemaVersion + 1)
                                    + "\"},\"children\":[{\"type\":\"NodeMap\",\"properties\":{},\"children\":[{\"type\":\"RootNodeData\",\"properties\":{\"Id\":\"1\"},\"children\":[]}]}]}";

    const int newerStateSize = static_cast<int>(newerState.getNumBytesAsUTF8());

    processor.setStateInformation(newerState.toRawUTF8(), newerStateSize);

    CHECK(processor.document.graphState.nodeMap.getNumChildren() == 0);
}

static std::unique_ptr<juce::XmlElement> readStateText(const juce::String& text)
{
    return StateJson::read(text.toRawUTF8(), static_cast<int>(text.getNumBytesAsUTF8()));
}

static juce::String nodeMapState(const juce::String& nodes)
{
    return "{\"type\":\"PluginState\",\"properties\":{},\"children\":[{\"type\":\"NodeMap\",\"properties\":{},\"children\":[" + nodes + "]}]}";
}

TEST_CASE("a state nested far past any real document is not read, and a shallow one is", "[graph][state]")
{
    constexpr int deepLevels    = 300;
    constexpr int shallowLevels = 10;

    const juce::String opening = "{\"type\":\"Level\",\"properties\":{},\"children\":[";
    const juce::String closing = "]}";

    const juce::String deepState    = juce::String::repeatedString(opening, deepLevels) + juce::String::repeatedString(closing, deepLevels);
    const juce::String shallowState = juce::String::repeatedString(opening, shallowLevels) + juce::String::repeatedString(closing, shallowLevels);

    CHECK(readStateText(deepState) == nullptr);
    CHECK(readStateText(shallowState) != nullptr);
}

TEST_CASE("brackets, escaped quotes and non-ASCII text inside a value are not counted as nesting", "[graph][state]")
{
    constexpr int bracketCount = 500;

    const juce::String brackets = juce::String::repeatedString("[", bracketCount);
    const juce::String accent   = juce::String::fromUTF8("\xc3\xa9");

    const juce::String state = "{\"type\":\"Root\",\"properties\":{\"Label\":\"" + brackets + "\\\" \\\\ " + accent + " " + brackets + "\"},\"children\":[]}";

    const std::unique_ptr<juce::XmlElement> element = readStateText(state);

    REQUIRE(element != nullptr);

    CHECK(element->getStringAttribute("Label").contains(accent));
}

TEST_CASE("a state whose properties or children are not an object and an array is not read", "[graph][state]")
{
    CHECK(readStateText("{\"type\":\"Root\",\"properties\":\"text\",\"children\":[]}") == nullptr);
    CHECK(readStateText("{\"type\":\"Root\",\"properties\":{},\"children\":\"text\"}") == nullptr);
    CHECK(readStateText("{\"type\":\"Root\",\"properties\":{},\"children\":[]}") != nullptr);
}

TEST_CASE("a state with two nodes sharing an id is not loaded", "[graph][state]")
{
    SequenceTreeAudioProcessor processor;

    const juce::String duplicated = nodeMapState("{\"type\":\"RootNodeData\",\"properties\":{\"Id\":\"1\"},\"children\":[]},{\"type\":\"NodeData\",\"properties\":{\"Id\":\"1\"},\"children\":[]}");

    const int duplicatedSize = static_cast<int>(duplicated.getNumBytesAsUTF8());

    processor.setStateInformation(duplicated.toRawUTF8(), duplicatedSize);

    CHECK(processor.document.graphState.nodeMap.getNumChildren() == 0);
}

TEST_CASE("a state with a node type that does not exist is not loaded", "[graph][state]")
{
    SequenceTreeAudioProcessor processor;

    const juce::String unknown = nodeMapState("{\"type\":\"RootNodeData\",\"properties\":{\"Id\":\"1\"},\"children\":[]},{\"type\":\"NotANode\",\"properties\":{\"Id\":\"2\"},\"children\":[]}");

    const int unknownSize = static_cast<int>(unknown.getNumBytesAsUTF8());

    processor.setStateInformation(unknown.toRawUTF8(), unknownSize);

    CHECK(processor.document.graphState.nodeMap.getNumChildren() == 0);
}

TEST_CASE("a refused state leaves its reason in loadError and a later good state clears it", "[graph][state]")
{
    SequenceTreeAudioProcessor processor;

    const juce::String duplicated = nodeMapState("{\"type\":\"RootNodeData\",\"properties\":{\"Id\":\"1\"},\"children\":[]},{\"type\":\"NodeData\",\"properties\":{\"Id\":\"1\"},\"children\":[]}");
    const juce::String good       = nodeMapState("{\"type\":\"RootNodeData\",\"properties\":{\"Id\":\"1\"},\"children\":[]}");
    const juce::String notAState  = "not a state";

    const int duplicatedSize = static_cast<int>(duplicated.getNumBytesAsUTF8());
    const int goodSize       = static_cast<int>(good.getNumBytesAsUTF8());
    const int notAStateSize  = static_cast<int>(notAState.getNumBytesAsUTF8());

    processor.setStateInformation(duplicated.toRawUTF8(), duplicatedSize);

    CHECK(processor.loadError.contains("share the id 1"));

    processor.setStateInformation(notAState.toRawUTF8(), notAStateSize);

    CHECK(processor.loadError.contains("could not be read"));

    processor.setStateInformation(good.toRawUTF8(), goodSize);

    CHECK(processor.loadError.isEmpty());
    CHECK(processor.document.graphState.nodeMap.getNumChildren() == 1);
}

TEST_CASE("a state with a node id that is missing, below 1 or the largest int is refused with the reason in loadError", "[graph][state]")
{
    struct BadId
    {
        const char* property;
        const char* reason;
    };

    SequenceTreeAudioProcessor processor;

    const BadId badIds[] = {
        { "\"Id\":\"0\"",          "has the id 0" },
        { "\"Id\":\"-5\"",         "has the id -5" },
        { "\"Id\":\"2147483647\"", "has the id 2147483647" },
        { "",                      "has the id 0" }
    };

    for (const BadId& badId : badIds) {
        CAPTURE(badId.property);

        const juce::String state     = nodeMapState("{\"type\":\"RootNodeData\",\"properties\":{" + juce::String(badId.property) + "},\"children\":[]}");
        const int          stateSize = static_cast<int>(state.getNumBytesAsUTF8());

        processor.setStateInformation(state.toRawUTF8(), stateSize);

        CHECK(processor.loadError.contains(badId.reason));
        CHECK(processor.document.graphState.nodeMap.getNumChildren() == 0);
    }
}

TEST_CASE("a file that is not a SequenceTree patch is refused and leaves the open patch as it was", "[graph][state]")
{
    SequenceTreeAudioProcessor processor;

    const juce::String otherDocument  = "{\"type\":\"Settings\",\"properties\":{\"Theme\":\"dark\"},\"children\":[]}";
    const juce::String withoutNodeMap = "{\"type\":\"PluginState\",\"properties\":{},\"children\":[{\"type\":\"CanvasSettings\",\"properties\":{},\"children\":[]}]}";

    const int rootId             = createRoot(processor.document.graphState, 0, 0);
    const int otherDocumentSize  = static_cast<int>(otherDocument.getNumBytesAsUTF8());
    const int withoutNodeMapSize = static_cast<int>(withoutNodeMap.getNumBytesAsUTF8());

    processor.setStateInformation(otherDocument.toRawUTF8(), otherDocumentSize);

    CHECK(processor.loadError.contains("not a SequenceTree patch"));
    CHECK(processor.document.graphState.nodeMap.getNumChildren() == 1);

    processor.setStateInformation(withoutNodeMap.toRawUTF8(), withoutNodeMapSize);

    CHECK(processor.loadError.contains("not a SequenceTree patch"));
    CHECK(processor.document.graphState.nodeMap.getNumChildren() == 1);
    CHECK(processor.document.graphState.getNode(rootId).isValid());
}

TEST_CASE("a saved patch cut off partway is refused and leaves the open patch as it was", "[graph][state]")
{
    SequenceTreeAudioProcessor original;
    SequenceTreeAudioProcessor processor;
    juce::MemoryBlock          savedState;
    juce::TemporaryFile        patchFile { StateJson::patchExtension };

    const int originalRootId = createRoot(original.document.graphState, 0, 0);
    const int rootId         = createRoot(processor.document.graphState, 0, 0);

    createChild(original.document.graphState, originalRootId, 100, 0);

    original.document.patch.setProperty(ValueTreeIdentifiers::PatchFile, patchFile.getFile().getFullPathName(), nullptr);

    original.getStateInformation(savedState);

    const int cutOffSize = static_cast<int>(savedState.getSize()) / 2;

    processor.setStateInformation(savedState.getData(), cutOffSize);

    CHECK(processor.loadError.isNotEmpty());
    CHECK(processor.document.graphState.nodeMap.getNumChildren() == 1);
    CHECK(processor.document.graphState.getNode(rootId).isValid());
}

TEST_CASE("an arrow type or binding outside its enum reads as the default", "[graph][canvas]")
{
    constexpr int outOfRangeType    = 9;
    constexpr int outOfRangeBinding = -4;

    const ArrowInfo::Settings defaults;

    juce::ValueTree arrow { "Arrow" };

    arrow.setProperty(ValueTreeIdentifiers::ArrowType,     outOfRangeType,    nullptr);
    arrow.setProperty(ValueTreeIdentifiers::ArrowXBinding, outOfRangeBinding, nullptr);
    arrow.setProperty(ValueTreeIdentifiers::ArrowYBinding, outOfRangeBinding, nullptr);

    const ArrowInfo::Settings read = ArrowBindingOps::getArrowInfo(arrow);

    CHECK(read.type == defaults.type);
    CHECK(read.xBinding == defaults.xBinding);
    CHECK(read.yBinding == defaults.yBinding);
}

TEST_CASE("the arrow binding multipliers survive saving and restoring the plugin state", "[graph][canvas]")
{
    SequenceTreeAudioProcessor original;
    SequenceTreeAudioProcessor restored;
    juce::MemoryBlock          savedState;
    juce::TemporaryFile        patchFile { StateJson::patchExtension };

    const int durationBinding = static_cast<int>(ArrowInfo::Binding::DurationBind);
    const int pitchBinding    = static_cast<int>(ArrowInfo::Binding::PitchBind);

    original.document.patch.setProperty(ValueTreeIdentifiers::PatchFile, patchFile.getFile().getFullPathName(), nullptr);

    original.document.canvasSettings.setProperty(ValueTreeIdentifiers::ArrowXMultiplier, 2.5, nullptr);
    original.document.canvasSettings.setProperty(ValueTreeIdentifiers::ArrowYMultiplier, 0.75, nullptr);
    original.document.canvasSettings.setProperty(ValueTreeIdentifiers::ArrowXBinding,    durationBinding, nullptr);
    original.document.canvasSettings.setProperty(ValueTreeIdentifiers::ArrowYBinding,    pitchBinding, nullptr);
    original.document.canvasSettings.setProperty(ValueTreeIdentifiers::ArrowXTarget,     durationBinding, nullptr);
    original.document.canvasSettings.setProperty(ValueTreeIdentifiers::ArrowYTarget,     pitchBinding, nullptr);

    original.getStateInformation(savedState);

    restored.setStateInformation(savedState.getData(), static_cast<int>(savedState.getSize()));

    CHECK(static_cast<double>(restored.document.canvasSettings.getProperty(ValueTreeIdentifiers::ArrowXMultiplier)) == 2.5);
    CHECK(static_cast<double>(restored.document.canvasSettings.getProperty(ValueTreeIdentifiers::ArrowYMultiplier)) == 0.75);
    CHECK(static_cast<int>(restored.document.canvasSettings.getProperty(ValueTreeIdentifiers::ArrowXBinding)) == durationBinding);
    CHECK(static_cast<int>(restored.document.canvasSettings.getProperty(ValueTreeIdentifiers::ArrowYBinding)) == pitchBinding);
    CHECK(static_cast<int>(restored.document.canvasSettings.getProperty(ValueTreeIdentifiers::ArrowXTarget)) == durationBinding);
    CHECK(static_cast<int>(restored.document.canvasSettings.getProperty(ValueTreeIdentifiers::ArrowYTarget)) == pitchBinding);
}

TEST_CASE("a new arrow takes the arrow multiplier the canvas settings hold when it is made", "[graph][canvas]")
{
    SequenceTreeAudioProcessor processor;
    GraphDocument              document(processor.document.graphState, processor.document.undoManager, processor.document.canvasSettings);

    const juce::Point<int>   childPosition { 100, 0 };
    const juce::ModifierKeys noModifiers;

    processor.document.canvasSettings.setProperty(ValueTreeIdentifiers::ArrowXMultiplier, 2.5, nullptr);

    const int rootId = createRoot(processor.document.graphState, 0, 0);

    const juce::ValueTree child = document.addChildNode(rootId, childPosition, noModifiers);

    REQUIRE(child.isValid());

    const int             childId  = child.getProperty(ArsNovaUI::CanvasIdentifiers::Id);
    const juce::ValueTree children = processor.document.graphState.getNode(rootId).getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds);
    const juce::ValueTree arrow    = children.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, childId);

    REQUIRE(arrow.isValid());

    CHECK(static_cast<double>(arrow.getProperty(ValueTreeIdentifiers::ArrowXMultiplier)) == 2.5);
}

TEST_CASE("a chain built through the graph walks in order and loops", "[graph][traversal]")
{
    SequenceTreeAudioProcessor processor;
    GraphState& graph = processor.document.graphState;

    const int rootId   = createRoot(graph, 0, 0);
    const int middleId = createChild(graph, rootId, 100, 0);
    const int leafId   = createChild(graph, middleId, 200, 0);

    const NodeMap& nodes = rebuildAndPublish(processor);

    TraversalLogic logic;

    logic.childSelector.nodeState.prepare();
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
    GraphState& graph = processor.document.graphState;

    const int rootId   = createRoot(graph, 0, 0);
    const int middleId = createChild(graph, rootId, 100, 0);
    const int leafId   = createChild(graph, middleId, 200, 0);

    const juce::ValueTree modulatorRoot = NodeFactory(graph, nullptr).createModulatorNode(NodeInfo::Kind::ModulatorRoot, rootId, NodeInfo::Position { 0, 200, 25 });
    const int modulatorRootId = modulatorRoot.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    const juce::ValueTree modulatorMiddle = NodeFactory(graph, nullptr).createModulatorNode(NodeInfo::Kind::Modulator, modulatorRootId, NodeInfo::Position { 100, 200, 25 });
    const int modulatorMiddleId = modulatorMiddle.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    const juce::ValueTree modulatorLeaf = NodeFactory(graph, nullptr).createModulatorNode(NodeInfo::Kind::Modulator, modulatorMiddleId, NodeInfo::Position { 200, 200, 25 });
    const int modulatorLeafId = modulatorLeaf.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    const NodeMap& nodes = rebuildAndPublish(processor);

    TraversalLogic logic;

    logic.childSelector.nodeState.prepare();
    logic.reset(rootId, RTtraversal {});
    logic.mod.activate(modulatorRootId, rootId);
    logic.childSelector.advanceAlternative(nodes, logic.mod.walker, Walk::Modulator, modulatorRootId);

    std::vector<int> visited { logic.mod.walker.target };

    for (int step = 0; step < 6; ++step) {
        logic.decideNextModulator(nodes);
        logic.mod.step();
        logic.childSelector.advanceAlternative(nodes, logic.mod.walker, Walk::Modulator, logic.mod.walker.target);

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
    GraphState& graph = processor.document.graphState;

    const int rootId        = createRoot(graph, 0, 0);
    const int nodeId        = createChild(graph, rootId, 100, 0);
    const int foreignRootId = createRoot(graph, 0, 400);

    const juce::ValueTree modulatorRoot = NodeFactory(graph, nullptr).createModulatorNode(NodeInfo::Kind::ModulatorRoot, nodeId, NodeInfo::Position { 100, 200, 25 });
    const int modulatorRootId = modulatorRoot.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    const juce::ValueTree modulator = NodeFactory(graph, nullptr).createModulatorNode(NodeInfo::Kind::Modulator, modulatorRootId, NodeInfo::Position { 200, 200, 25 });
    const int modulatorId = modulator.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    const juce::ValueTree alternativeModulator = NodeFactory(graph, nullptr).createModulatorNode(NodeInfo::Kind::AlternativeModulator, modulatorId, NodeInfo::Position { 200, 300, 25 });
    const int alternativeModulatorId = alternativeModulator.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    CHECK(graph.connectsToOtherTreeRoot(nodeId, foreignRootId));
    CHECK_FALSE(graph.connectsToOtherTreeRoot(nodeId, rootId));
    CHECK_FALSE(graph.connectsToOtherTreeRoot(modulatorRootId, foreignRootId));
    CHECK_FALSE(graph.connectsToOtherTreeRoot(modulatorId, foreignRootId));
    CHECK_FALSE(graph.connectsToOtherTreeRoot(alternativeModulatorId, foreignRootId));
}

TEST_CASE("the published graph starts from its first unlinked root, or its first root when every root is linked", "[graph]")
{
    SequenceTreeAudioProcessor processor;
    GraphState& graph = processor.document.graphState;

    const int firstRootId   = createRoot(graph, 0, 0);
    const int secondRootId  = createRoot(graph, 0, 400);
    const int firstChildId  = createChild(graph, firstRootId, 100, 0);
    const int secondChildId = createChild(graph, secondRootId, 100, 400);

    REQUIRE(firstRootId < secondRootId);

    CHECK(rebuildAndPublish(processor).startRootId == firstRootId);

    graph.connectNodes(firstChildId, secondRootId, nullptr);
    CHECK(rebuildAndPublish(processor).startRootId == firstRootId);

    graph.connectNodes(secondChildId, firstRootId, nullptr);
    CHECK(rebuildAndPublish(processor).startRootId == firstRootId);

    graph.disconnectNodes(firstChildId, secondRootId, nullptr);
    CHECK(rebuildAndPublish(processor).startRootId == secondRootId);
}

TEST_CASE("moving a pitch-bound node transposes around the pitch last typed", "[graph][pitch]")
{
    SequenceTreeAudioProcessor processor;
    GraphState& graph = processor.document.graphState;

    const int rootId  = createRoot(graph, 0, 0);
    const int childId = createChild(graph, rootId, 100, 0);

    juce::ValueTree childNote = graph.getMidiNotes(childId).getChildWithName(ValueTreeIdentifiers::MidiNoteData);

    REQUIRE(childNote.isValid());

    const int startPitch = childNote.getProperty(ValueTreeIdentifiers::MidiPitch);

    graph.setNodePosition(graph.getNode(childId), NodeInfo::Position { 100, -100, 25 }, nullptr);

    CHECK(static_cast<int>(childNote.getProperty(ValueTreeIdentifiers::MidiPitch)) == startPitch + 2);

    childNote.setProperty(ValueTreeIdentifiers::MidiPitch, 70, nullptr);

    graph.setNodePosition(graph.getNode(childId), NodeInfo::Position { 100, 0, 25 }, nullptr);

    CHECK(static_cast<int>(childNote.getProperty(ValueTreeIdentifiers::MidiPitch)) == 68);

    const NodeMap& nodes = rebuildAndPublish(processor);

    REQUIRE_FALSE(nodes.find(childId)->notes.empty());
    CHECK(nodes.find(childId)->notes.front().pitch == 68);
}

TEST_CASE("a pasted pitch-bound arrow keeps its pitch offset, so moving the pasted child transposes it as it does the original", "[graph][pitch][clipboard]")
{
    SequenceTreeAudioProcessor processor;

    GraphState& graph = processor.document.graphState;

    Clipboard clipboard(graph, processor.document.undoManager);

    const int rootId  = createRoot(graph, 0, 0);
    const int childId = createChild(graph, rootId, 100, 0);

    const std::vector<int> copiedIds { rootId, childId };

    clipboard.copy(copiedIds);

    const std::vector<int> pastedIds = clipboard.pasteAt({ 600, 600 });

    int pastedChildId = 0;

    for (const int pastedId : pastedIds) {
        if (graph.getNode(pastedId).getType() != ValueTreeIdentifiers::RootNodeData) {
            pastedChildId = pastedId;
        }
    }

    REQUIRE(pastedIds.size() == 2);
    REQUIRE(pastedChildId != 0);

    const juce::ValueTree originalNote = graph.getMidiNotes(childId).getChildWithName(ValueTreeIdentifiers::MidiNoteData);
    const juce::ValueTree pastedNote   = graph.getMidiNotes(pastedChildId).getChildWithName(ValueTreeIdentifiers::MidiNoteData);

    REQUIRE(originalNote.isValid());
    REQUIRE(pastedNote.isValid());

    const int originalStartPitch = originalNote.getProperty(ValueTreeIdentifiers::MidiPitch);
    const int pastedStartPitch   = pastedNote.getProperty(ValueTreeIdentifiers::MidiPitch);

    NodeInfo::Position originalMoved = graph.getNodePosition(childId);
    NodeInfo::Position pastedMoved   = graph.getNodePosition(pastedChildId);

    originalMoved.yPosition -= 100;
    pastedMoved.yPosition   -= 100;

    graph.setNodePosition(graph.getNode(childId), originalMoved, nullptr);
    graph.setNodePosition(graph.getNode(pastedChildId), pastedMoved, nullptr);

    const int originalEndPitch = originalNote.getProperty(ValueTreeIdentifiers::MidiPitch);
    const int pastedEndPitch   = pastedNote.getProperty(ValueTreeIdentifiers::MidiPitch);

    const int originalShift = originalEndPitch - originalStartPitch;
    const int pastedShift   = pastedEndPitch - pastedStartPitch;

    REQUIRE(originalShift != 0);
    CHECK(pastedShift == originalShift);
}

TEST_CASE("copying a group node brings its members, and copying one member copies only that node", "[graph][clipboard][encapsulation]")
{
    SequenceTreeAudioProcessor processor;

    GraphState& graph = processor.document.graphState;

    Clipboard clipboard(graph, processor.document.undoManager);

    const int rootId   = createRoot(graph, 0, 0);
    const int firstId  = createChild(graph, rootId, 100, 0);
    const int secondId = createChild(graph, firstId, 200, 0);
    const int thirdId  = createChild(graph, secondId, 300, 0);

    const std::vector<int> memberIds { firstId, secondId, thirdId };

    const juce::ValueTree encapsulator   = NodeFactory(graph, nullptr).createEncapsulator(memberIds);
    const int             encapsulatorId = encapsulator.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    const std::vector<int> copiedMember { secondId };
    const std::vector<int> copiedGroup { encapsulatorId };

    clipboard.copy(copiedMember);

    const std::vector<int> pastedMember = clipboard.pasteAt({ 600, 600 });

    REQUIRE(pastedMember.size() == 1);
    CHECK(graph.getNode(pastedMember.front()).getType() != ValueTreeIdentifiers::EncapsulatorData);

    clipboard.copy(copiedGroup);

    const std::vector<int> pastedGroup = clipboard.pasteAt({ 900, 900 });

    int pastedEncapsulators = 0;

    for (const int pastedId : pastedGroup) {
        if (graph.getNode(pastedId).getType() == ValueTreeIdentifiers::EncapsulatorData) {
            ++pastedEncapsulators;
        }
    }

    CHECK(pastedGroup.size() == memberIds.size() + 1);
    CHECK(pastedEncapsulators == 1);
}

TEST_CASE("undo and redo keep the node index, parent links and published graph in step", "[graph][undo]")
{
    SequenceTreeAudioProcessor processor;
    GraphState& graph = processor.document.graphState;
    juce::UndoManager undoManager;

    const int rootId = createRoot(graph, 0, 0);

    undoManager.beginNewTransaction();

    const juce::ValueTree child = NodeFactory(graph, &undoManager).createNode(rootId, ValueTreeIdentifiers::NodeData, NodeInfo::Position { 100, 0, 25 });
    const int childId = child.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

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
    GraphState& graph = processor.document.graphState;
    juce::UndoManager& undoManager = processor.document.undoManager;

    const int rootId  = createRoot(graph, 0, 0);
    const int childId = createChild(graph, rootId, 100, 0);

    juce::ValueTree connection = graph.getConnection(rootId, childId);

    undoManager.beginNewTransaction();
    connection.setProperty(ValueTreeIdentifiers::ArrowDuration, 700, &undoManager);

    undoManager.beginNewTransaction();
    graph.setNodePosition(graph.getNode(childId), NodeInfo::Position { 200, 0, 25 }, &undoManager);

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
    GraphState& graph = processor.document.graphState;

    const int rootId  = createRoot(graph, 0, 0);
    const int firstId = createChild(graph, rootId, 100, 0);
    const int lastId  = createChild(graph, firstId, 200, 0);
    const int afterId = createChild(graph, lastId, 300, 0);

    const std::vector<int> memberIds { firstId, lastId };
    const juce::ValueTree encapsulator = NodeFactory(graph, nullptr).createEncapsulator(memberIds);
    const int encapsulatorId = encapsulator.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

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
    GraphState& graph = processor.document.graphState;

    const int rootId  = createRoot(graph, 0, 0);
    const int firstId = createChild(graph, rootId, 100, 0);
    const int lastId  = createChild(graph, firstId, 200, 0);

    const std::vector<int> memberIds { firstId, lastId };
    const juce::ValueTree encapsulator = NodeFactory(graph, nullptr).createEncapsulator(memberIds);
    const int encapsulatorId = encapsulator.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

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
    GraphState& graph = processor.document.graphState;

    const int rootId   = createRoot(graph, 0, 0);
    const int memberId = createChild(graph, rootId, 100, 0);

    const std::vector<int> memberIds { memberId };
    const juce::ValueTree encapsulator = NodeFactory(graph, nullptr).createEncapsulator(memberIds);
    const int encapsulatorId = encapsulator.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    graph.removeNode(memberId, nullptr);

    CHECK_FALSE(graph.getNode(encapsulatorId).isValid());
}

TEST_CASE("a node's parent is found through a chain of traversal flags", "[graph]")
{
    SequenceTreeAudioProcessor processor;
    GraphState& graph = processor.document.graphState;

    const int rootId = createRoot(graph, 0, 0);
    const int noteId = createChild(graph, rootId, 100, 0);

    const juce::ValueTree firstFlag = NodeFactory(graph, nullptr).createTraversalFlagNode(noteId, NodeInfo::Position { 200, 0, 25 });
    const int firstFlagId = firstFlag.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    const juce::ValueTree secondFlag = NodeFactory(graph, nullptr).createTraversalFlagNode(firstFlagId, NodeInfo::Position { 300, 0, 25 });
    const int secondFlagId = secondFlag.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    CHECK(static_cast<int>(graph.getNodeParent(secondFlagId).getProperty(ArsNovaUI::CanvasIdentifiers::Id)) == noteId);
    CHECK(static_cast<int>(graph.getNodeParent(firstFlagId).getProperty(ArsNovaUI::CanvasIdentifiers::Id))  == noteId);
    CHECK(static_cast<int>(graph.getNodeParent(noteId).getProperty(ArsNovaUI::CanvasIdentifiers::Id))       == rootId);
    CHECK_FALSE(graph.getNodeParent(rootId).isValid());
}

TEST_CASE("a restored graph indexes, numbers and walks like the one it was saved from", "[graph][traversal]")
{
    SequenceTreeAudioProcessor original;
    SequenceTreeAudioProcessor restored;

    const int rootId   = createRoot(original.document.graphState, 0, 0);
    const int everyId  = createChild(original.document.graphState, rootId, 100, 0);
    const int secondId = createChild(original.document.graphState, rootId, 100, 100);

    original.document.graphState.getNode(secondId).setProperty(ValueTreeIdentifiers::CountLimit, 2, nullptr);

    restored.document.graphState.replaceState(original.document.graphState.nodeMap, original.document.graphState.traversals.map);

    CHECK(restored.document.graphState.nodeIdIncrement == original.document.graphState.nodeIdIncrement);
    CHECK(restored.document.graphState.parentIdsOf     == original.document.graphState.parentIdsOf);
    CHECK(restored.document.graphState.getNode(everyId).isValid());

    std::vector<std::vector<int>> walks;

    for (SequenceTreeAudioProcessor* processor : { &original, &restored }) {
        const NodeMap& nodes = rebuildAndPublish(*processor);

        TraversalLogic logic;

        logic.childSelector.nodeState.prepare();
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

static std::vector<PlayedNote> playProcessor(SequenceTreeAudioProcessor& processor)
{
    constexpr double sampleRate = 48000.0;
    constexpr int    blockSize  = 512;
    constexpr int    blockCount = 400;

    HostPlayHead             playHead;
    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer         midi;
    std::vector<PlayedNote>  played;
    double                   ppq = 0.0;

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

static std::vector<PlayedNote> playChainWithScript(const char* scriptSource)
{
    SequenceTreeAudioProcessor processor;
    GraphState& graph = processor.document.graphState;

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

    return playProcessor(processor);
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

TEST_CASE("a saved patch with branches and a live rule loads and plays exactly as the patch it was saved from", "[graph][state][script]")
{
    SequenceTreeAudioProcessor original;
    SequenceTreeAudioProcessor restored;
    juce::MemoryBlock          savedState;
    juce::TemporaryFile        patchFile { StateJson::patchExtension };

    GraphState&         graph     = original.document.graphState;
    TraversalRuleState& ruleState = original.document.traversalRuleState;

    const std::string shifted = traversalWithMain(
        "        selectedNode = advance(1);\n"
        "        playNote(selectedNode, selectedNode.pitch - 12, selectedNode.duration, selectedNode.velocity);");

    const int rootId   = createRoot(graph, 0, 0);
    const int firstId  = createChild(graph, rootId, 100, 0);
    const int secondId = createChild(graph, rootId, 100, 100);
    const int ruleId   = ruleState.addRule(0, nullptr).getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    createChild(graph, firstId, 220, 0);
    createChild(graph, secondId, 220, 100);

    original.document.patch.setProperty(ValueTreeIdentifiers::PatchFile, patchFile.getFile().getFullPathName(), nullptr);

    ruleState.setRuleSource(ruleId, shifted, nullptr);

    ruleState.activeRuleId.setValue(ruleId, nullptr);

    rebuildAndPublish(original);

    original.getStateInformation(savedState);

    const int savedSize = static_cast<int>(savedState.getSize());

    restored.setStateInformation(savedState.getData(), savedSize);

    const std::vector<PlayedNote> originalNotes = playProcessor(original);
    const std::vector<PlayedNote> restoredNotes = playProcessor(restored);

    CHECK(restored.loadError.isEmpty());
    CHECK(restored.document.traversalRuleState.activeRuleId.get() == ruleId);
    REQUIRE(originalNotes.size() > 4);
    CHECK(restoredNotes == originalNotes);
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

struct TemporaryLibraryFolder
{
    juce::File folder = juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("GraphTestsOrnaments", "", false);

    ~TemporaryLibraryFolder()
    {
        folder.deleteRecursively();
    }
};

static constexpr int ornamentedPitch = 72;

static std::vector<PlayedNote> playLowerGraceChain(bool ornamentsFirstNode)
{
    TemporaryLibraryFolder     temporary;
    SequenceTreeAudioProcessor processor;
    GraphState&                graph     = processor.document.graphState;
    OrnamentLibraries&         ornaments = processor.document.ornamentLibraries;

    ornaments.load(temporary.folder);

    const juce::ValueTree library  = ornaments.addLibrary("Graph tests", nullptr);
    juce::ValueTree       ornament = ornaments.addOrnament(library, nullptr);
    juce::ValueTree       grace    = OrnamentFigure::addNode(ornament, -1, { 0, 0 }, nullptr);
    const juce::ValueTree host     = OrnamentFigure::addNode(ornament, grace.getProperty(ArsNovaUI::CanvasIdentifiers::Id), { 100, 0 }, nullptr);

    grace.setProperty(ValueTreeIdentifiers::OrnamentOffset,   -1, nullptr);
    grace.setProperty(ValueTreeIdentifiers::OrnamentVelocity, 70, nullptr);

    OrnamentFigure::toggleHost(ornament, host.getProperty(ArsNovaUI::CanvasIdentifiers::Id), nullptr);
    OrnamentFigure::setLastNodeLength(ornament, { 200, 0 }, nullptr);
    OrnamentFigure::publish(ornament, nullptr);

    const int       rootId  = createRoot(graph, 0, 0);
    const int       firstId = createChild(graph, rootId, 100, 0);
    juce::ValueTree first   = graph.getNode(firstId);

    createChild(graph, firstId, 220, 0);

    first.getChildWithName(ValueTreeIdentifiers::MidiNotesData).getChild(0).setProperty(ValueTreeIdentifiers::MidiPitch, ornamentedPitch, nullptr);

    if (ornamentsFirstNode) {
        first.setProperty(ValueTreeIdentifiers::OrnamentUid, ornament.getProperty(ValueTreeIdentifiers::OrnamentUid), nullptr);
    }

    rebuildAndPublish(processor);
    processor.snapshots.publishScript(nullptr);

    return playProcessor(processor);
}

TEST_CASE("an ornamented node plays its figure across its own duration, and the walk keeps its rhythm", "[graph][ornament]")
{
    const std::vector<PlayedNote> plain      = playLowerGraceChain(false);
    const std::vector<PlayedNote> ornamented = playLowerGraceChain(true);
    std::vector<PlayedNote>       expected;

    REQUIRE(plain.size() > 4);
    CHECK(std::ranges::count(plain, ornamentedPitch, &PlayedNote::pitch) > 1);

    for (std::size_t index = 0; index + 1 < plain.size(); ++index) {
        const PlayedNote& note   = plain[index];
        const int         length = plain[index + 1].sample - note.sample;

        if (note.pitch != ornamentedPitch) {
            expected.push_back(note);
            continue;
        }

        expected.push_back({ note.pitch - 1, juce::roundToInt(note.velocity * 0.7), note.sample });
        expected.push_back({ note.pitch, note.velocity, note.sample + length / 3 });
    }

    REQUIRE(ornamented.size() >= expected.size());

    for (std::size_t index = 0; index < expected.size(); ++index) {
        CAPTURE(index);
        CHECK(ornamented[index].pitch    == expected[index].pitch);
        CHECK(ornamented[index].velocity == expected[index].velocity);
        CHECK(std::abs(ornamented[index].sample - expected[index].sample) <= 1);
    }
}

TEST_CASE("Removing a rule folder removes the folders and rules inside it but keeps the live rule", "[rules]")
{
    TraversalRuleState state;

    const int outerId = state.addFolder(0, nullptr).getProperty(ValueTreeIdentifiers::FolderId);
    const int innerId = state.addFolder(outerId, nullptr).getProperty(ValueTreeIdentifiers::FolderId);
    const int keptId  = state.addRule(0, nullptr).getProperty(ArsNovaUI::CanvasIdentifiers::Id);
    const int liveId  = state.addRule(innerId, nullptr).getProperty(ArsNovaUI::CanvasIdentifiers::Id);
    const int goneId  = state.addRule(outerId, nullptr).getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    state.rules.setProperty(ValueTreeIdentifiers::ActiveRuleId, liveId, nullptr);

    state.removeFolder(outerId, nullptr);

    CHECK(state.folders.getNumChildren() == 0);
    CHECK(state.rules.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, keptId).isValid());
    CHECK_FALSE(state.rules.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, goneId).isValid());
    REQUIRE(state.rules.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, liveId).isValid());
    CHECK(static_cast<int>(state.rules.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, liveId).getProperty(ValueTreeIdentifiers::RuleFolderId)) == 0);
}

TEST_CASE("Moving rules and folders keeps their order and never puts a folder inside itself", "[rules]")
{
    TraversalRuleState state;

    const int folderId = state.addFolder(0, nullptr).getProperty(ValueTreeIdentifiers::FolderId);
    const int childId  = state.addFolder(folderId, nullptr).getProperty(ValueTreeIdentifiers::FolderId);
    const int firstId  = state.addRule(0, nullptr).getProperty(ArsNovaUI::CanvasIdentifiers::Id);
    const int movedId  = state.addRule(0, nullptr).getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    state.addRule(0, nullptr);

    state.moveRule(movedId, folderId, firstId, nullptr);

    CHECK(static_cast<int>(state.rules.getChild(0).getProperty(ArsNovaUI::CanvasIdentifiers::Id)) == movedId);
    CHECK(static_cast<int>(state.rules.getChild(0).getProperty(ValueTreeIdentifiers::RuleFolderId)) == folderId);

    state.moveRule(movedId, 0, -1, nullptr);

    CHECK(static_cast<int>(state.rules.getChild(2).getProperty(ArsNovaUI::CanvasIdentifiers::Id)) == movedId);
    CHECK(static_cast<int>(state.rules.getChild(2).getProperty(ValueTreeIdentifiers::RuleFolderId)) == 0);

    state.moveFolder(folderId, childId, -1, nullptr);

    CHECK(static_cast<int>(state.folders.getChildWithProperty(ValueTreeIdentifiers::FolderId, folderId).getProperty(ValueTreeIdentifiers::FolderParentId)) == 0);
    CHECK(state.isInsideFolder(childId, folderId));
    CHECK_FALSE(state.isInsideFolder(folderId, childId));
    CHECK_FALSE(state.isInsideFolder(0, folderId));
}

TEST_CASE("A rule's folder, its path and a folder's rule count follow the folders, and a missing folder counts as the top level", "[rules]")
{
    TraversalRuleState state;

    const int outerId  = state.addFolder(0, nullptr).getProperty(ValueTreeIdentifiers::FolderId);
    const int innerId  = state.addFolder(outerId, nullptr).getProperty(ValueTreeIdentifiers::FolderId);
    const int nestedId = state.addRule(innerId, nullptr).getProperty(ArsNovaUI::CanvasIdentifiers::Id);
    const int strayId  = state.addRule(99, nullptr).getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    state.addRule(outerId, nullptr);

    state.setFolderName(outerId, "outer", nullptr);
    state.setFolderName(innerId, "inner", nullptr);

    state.setRuleName(nestedId, "nested", nullptr);

    CHECK(state.folderOfRule(nestedId) == innerId);
    CHECK(state.folderOfRule(strayId) == 0);
    CHECK(state.parentOfFolder(innerId) == outerId);
    CHECK(state.parentOfFolder(outerId) == 0);
    CHECK(state.countRulesInside(outerId) == 2);
    CHECK(state.countRulesInside(innerId) == 1);
    CHECK(state.pathOf(nestedId) == juce::StringArray { "outer", "inner", "nested" });
    CHECK(state.pathOf(strayId).size() == 1);
}

TEST_CASE("Rule folders are saved with the plugin state and restored", "[rules]")
{
    SequenceTreeAudioProcessor original;
    SequenceTreeAudioProcessor restored;
    juce::MemoryBlock          savedState;

    const int folderId = original.document.traversalRuleState.addFolder(0, nullptr).getProperty(ValueTreeIdentifiers::FolderId);
    const int ruleId   = original.document.traversalRuleState.addRule(folderId, nullptr).getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    original.document.traversalRuleState.setFolderName(folderId, "Walks", nullptr);

    original.getStateInformation(savedState);

    restored.setStateInformation(savedState.getData(), static_cast<int>(savedState.getSize()));

    REQUIRE(restored.document.traversalRuleState.folders.getNumChildren() == 1);
    CHECK(restored.document.traversalRuleState.folders.getChild(0).getProperty(ValueTreeIdentifiers::FolderName).toString() == "Walks");
    CHECK(static_cast<int>(restored.document.traversalRuleState.rules.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, ruleId).getProperty(ValueTreeIdentifiers::RuleFolderId)) == folderId);
    CHECK(static_cast<int>(restored.document.traversalRuleState.addFolder(0, nullptr).getProperty(ValueTreeIdentifiers::FolderId)) == folderId + 1);
}

TEST_CASE("an orphaned note restarts its own walk, not the walk of the note swapped into its place", "[audio]")
{
    constexpr int childOffset   = 100;
    constexpr int deletedNodeId = 999;
    constexpr int orphanedRunId = 1;
    constexpr int keptRunId     = 2;
    constexpr int poolCapacity  = 4;
    constexpr int blockSize     = 512;

    constexpr double sampleRate        = 48000.0;
    constexpr double noteLengthSamples = 1.0e9;

    SequenceTreeAudioProcessor processor;

    GraphState&   graph        = processor.document.graphState;
    EventManager& eventManager = processor.playback.eventManager;

    TraversalPool pool;

    juce::MidiBuffer midi;

    NoteScheduler::ActiveNote orphanedNote;
    NoteScheduler::ActiveNote keptNote;

    const int rootId  = createRoot(graph, 0, 0);
    const int childId = createChild(graph, rootId, childOffset, 0);

    const NodeMap& nodes = rebuildAndPublish(processor);

    const RTNode* const rootNode = nodes.find(rootId);

    const DispatchContext context { nodes, pool, midi, sampleRate };

    REQUIRE(rootNode != nullptr);
    REQUIRE(!rootNode->traversals.empty());

    processor.prepareToPlay(sampleRate, blockSize);

    pool.prepare(poolCapacity, NativeTraversalRule::instance());

    TraversalPool::Instance* const orphanedWalk = pool.acquire(orphanedRunId, rootId, rootNode->traversals.front());
    TraversalPool::Instance* const keptWalk     = pool.acquire(keptRunId, rootId, rootNode->traversals.front());

    REQUIRE(orphanedWalk != nullptr);
    REQUIRE(keptWalk != nullptr);

    orphanedWalk->logic.primary.target = deletedNodeId;
    orphanedWalk->logic.state          = TraversalLogic::TraversalState::Active;

    keptWalk->logic.primary.target = childId;
    keptWalk->logic.state          = TraversalLogic::TraversalState::Active;

    orphanedNote.runId            = orphanedRunId;
    orphanedNote.nodeId           = deletedNodeId;
    orphanedNote.remainingSamples = noteLengthSamples;

    keptNote.runId            = keptRunId;
    keptNote.nodeId           = childId;
    keptNote.remainingSamples = noteLengthSamples;

    eventManager.scheduler.activeNotes.push_back(orphanedNote);
    eventManager.scheduler.activeNotes.push_back(keptNote);

    eventManager.processEvents(blockSize, context);

    CHECK(orphanedWalk->logic.primary.target != deletedNodeId);
    CHECK(keptWalk->logic.primary.target == childId);
}

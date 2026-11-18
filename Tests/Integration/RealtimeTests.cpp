#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Processor/PluginProcessor.h"
#include "Processor/Document/Graph/GraphState.h"
#include "Processor/AudioModel/RTGraphBuilder.h"
#include "Processor/Document/ValueTreeIdentifiers.h"
#include <ArsNovaUI/Canvas/CanvasIdentifiers.h>
#include "Processor/Document/Graph/NodeFactory.h"
#include "Processor/Document/Ornaments/OrnamentLibraries.h"
#include "Processor/Document/Ornaments/OrnamentFigure.h"

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    return Catch::Session().run(argc, argv);
}

struct HostPlayHead : juce::AudioPlayHead
{
    PositionInfo position;

    juce::Optional<PositionInfo> getPosition() const override
    {
        return position;
    }
};

struct TemporaryLibraryFolder
{
    juce::File folder = juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("RealtimeTestsOrnaments", "", false);

    ~TemporaryLibraryFolder()
    {
        folder.deleteRecursively();
    }
};

TEST_CASE("processBlock stays realtime-safe while playing, editing, retiming, relocating and resetting", "[realtime]")
{
    constexpr double sampleRate = 48000.0;
    constexpr int    blockSize  = 512;

    TemporaryLibraryFolder     temporary;
    SequenceTreeAudioProcessor processor;
    GraphState&                graph     = processor.document.graphState;
    OrnamentLibraries&         ornaments = processor.document.ornamentLibraries;

    ornaments.load(temporary.folder);

    juce::ValueTree       ornament = ornaments.addOrnament(ornaments.addLibrary("Realtime tests", nullptr), nullptr);
    juce::ValueTree       grace    = OrnamentFigure::addNode(ornament, -1, { 0, 0 }, nullptr);
    const juce::ValueTree host     = OrnamentFigure::addNode(ornament, grace.getProperty(ArsNovaUI::CanvasIdentifiers::Id), { 100, 0 }, nullptr);

    grace.setProperty(ValueTreeIdentifiers::OrnamentOffset, -1, nullptr);

    OrnamentFigure::addNode(ornament, host.getProperty(ArsNovaUI::CanvasIdentifiers::Id), { 150, 0 }, nullptr);
    OrnamentFigure::toggleHost(ornament, host.getProperty(ArsNovaUI::CanvasIdentifiers::Id), nullptr);
    OrnamentFigure::publish(ornament, nullptr);

    NodeFactory(graph, nullptr).createRootNode(NodeInfo::Position { 0, 0, 25 });

    const int rootId = graph.nodeMap.getChild(graph.nodeMap.getNumChildren() - 1)
                                    .getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    juce::ValueTree first = NodeFactory(graph, nullptr).createNode(rootId, ValueTreeIdentifiers::NodeData, NodeInfo::Position { 100, 0, 25 });

    const int firstId = first.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    NodeFactory(graph, nullptr).createNode(rootId, ValueTreeIdentifiers::NodeData, NodeInfo::Position { 150, 50, 25 });
    NodeFactory(graph, nullptr).createNode(firstId, ValueTreeIdentifiers::NodeData, NodeInfo::Position { 200, 0, 25 });

    first.setProperty(ValueTreeIdentifiers::OrnamentUid, ornament.getProperty(ValueTreeIdentifiers::OrnamentUid), nullptr);
    graph.getNode(rootId).setProperty(ValueTreeIdentifiers::OrnamentUid, ornament.getProperty(ValueTreeIdentifiers::OrnamentUid), nullptr);

    processor.rtGraphBuilder.rebuildAllGraphs();

    HostPlayHead playHead;
    playHead.position.setIsPlaying(true);
    playHead.position.setBpm(120.0);

    processor.setPlayHead(&playHead);
    processor.prepareToPlay(sampleRate, blockSize);

    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer         midi;
    int                      noteOns = 0;
    double                   ppq     = 0.0;

    midi.ensureSize(2048);

    auto playBlocks = [&](int blockCount) {
        for (int block = 0; block < blockCount; ++block) {
            playHead.position.setPpqPosition(ppq);
            processor.processBlock(buffer, midi);

            ppq += blockSize / sampleRate * *playHead.position.getBpm() / 60.0;

            for (const auto event : midi) {
                if (event.getMessage().isNoteOn()) {
                    ++noteOns;
                }
            }
        }
    };

    playBlocks(200);

    graph.setNodePosition(first, NodeInfo::Position { 300, 100, 25 }, nullptr);
    processor.rtGraphBuilder.rebuildAllGraphs();
    playBlocks(100);

    playHead.position.setBpm(187.0);
    playBlocks(100);

    NodeFactory(graph, nullptr).createNode(firstId, ValueTreeIdentifiers::NodeData, NodeInfo::Position { 400, -50, 25 });
    processor.rtGraphBuilder.rebuildAllGraphs();
    playBlocks(100);

    graph.removeNode(firstId, nullptr);
    processor.rtGraphBuilder.rebuildAllGraphs();
    playBlocks(100);

    ppq = 96.0;
    playBlocks(100);

    ppq = 4.0;
    playBlocks(100);

    processor.playback.resetRequested.store(true);
    playBlocks(100);

    playHead.position.setIsPlaying(false);
    playBlocks(50);

    playHead.position.setIsPlaying(true);
    playBlocks(100);

    processor.releaseResources();
    processor.setPlayHead(nullptr);

    REQUIRE(noteOns > 0);
}

TEST_CASE("starting a walk stays realtime-safe with more root-bound arrows than the scratch reserve", "[realtime]")
{
    constexpr double sampleRate      = 48000.0;
    constexpr int    blockSize       = 512;
    constexpr int    rootBoundArrows = 257;

    SequenceTreeAudioProcessor processor;
    GraphState& graph = processor.document.graphState;

    HostPlayHead             playHead;
    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer         midi;
    int                      noteOns = 0;
    double                   ppq     = 0.0;

    NodeFactory(graph, nullptr).createRootNode(NodeInfo::Position { 0, 0, 25 });

    const int sourceRootId = graph.nodeMap.getChild(graph.nodeMap.getNumChildren() - 1)
                                          .getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    NodeFactory(graph, nullptr).createRootNode(NodeInfo::Position { 0, 600, 25 });

    const int targetRootId = graph.nodeMap.getChild(graph.nodeMap.getNumChildren() - 1)
                                          .getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    for (int i = 0; i < rootBoundArrows; ++i) {
        const juce::ValueTree child = NodeFactory(graph, nullptr).createNode(sourceRootId, ValueTreeIdentifiers::NodeData, NodeInfo::Position { 100 + 10 * i, 40 * (i % 8) - 140, 25 });

        graph.connectNodes(child.getProperty(ArsNovaUI::CanvasIdentifiers::Id), targetRootId, nullptr);
    }

    processor.rtGraphBuilder.rebuildAllGraphs();

    playHead.position.setIsPlaying(true);
    playHead.position.setBpm(120.0);

    processor.setPlayHead(&playHead);
    processor.prepareToPlay(sampleRate, blockSize);

    midi.ensureSize(2048);

    auto playBlocks = [&](int blockCount) {
        for (int block = 0; block < blockCount; ++block) {
            playHead.position.setPpqPosition(ppq);
            processor.processBlock(buffer, midi);

            ppq += blockSize / sampleRate * *playHead.position.getBpm() / 60.0;

            for (const auto event : midi) {
                if (event.getMessage().isNoteOn()) {
                    ++noteOns;
                }
            }
        }
    };

    playBlocks(50);

    ppq = 16.0;
    playBlocks(50);

    processor.releaseResources();
    processor.setPlayHead(nullptr);

    REQUIRE(noteOns > 0);
}

TEST_CASE("processBlock stays realtime-safe while a restored state waits for the message thread", "[realtime]")
{
    constexpr double sampleRate = 48000.0;
    constexpr int    blockSize  = 512;
    constexpr int    blockCount = 20;

    SequenceTreeAudioProcessor processor;

    HostPlayHead playHead;

    juce::AudioBuffer<float> buffer(2, blockSize);

    juce::MidiBuffer midi;

    NodeFactory(processor.document.graphState, nullptr).createRootNode(NodeInfo::Position { 0, 0, 25 });

    processor.rtGraphBuilder.rebuildAllGraphs();

    playHead.position.setIsPlaying(true);
    playHead.position.setBpm(120.0);

    processor.setPlayHead(&playHead);
    processor.prepareToPlay(sampleRate, blockSize);

    midi.ensureSize(2048);

    processor.pendingRestoreState = juce::ValueTree(ValueTreeIdentifiers::NodeMap);

    for (int block = 0; block < blockCount; ++block) {
        processor.processBlock(buffer, midi);
    }

    processor.releaseResources();
    processor.setPlayHead(nullptr);

    REQUIRE(processor.pendingRestoreState.isValid());
}

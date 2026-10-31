#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Plugin/PluginProcessor.h"
#include "Graph/GraphState.h"
#include "Graph/RTGraphBuilder.h"
#include "Graph/ValueTreeIdentifiers.h"
#include <ArsNovaUI/Canvas/CanvasIdentifiers.h>
#include "Graph/NodeFactory.h"
#include "Graph/OrnamentLibraries.h"

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
    GraphState&                graph     = processor.graphState;
    OrnamentLibraries&         ornaments = processor.ornamentLibraries;

    ornaments.load(temporary.folder);

    juce::ValueTree       ornament = ornaments.addOrnament(ornaments.addLibrary("Realtime tests", nullptr), nullptr);
    juce::ValueTree       grace    = ornaments.addFigureNode(ornament, -1, { 0, 0 }, nullptr);
    const juce::ValueTree host     = ornaments.addFigureNode(ornament, grace.getProperty(ArsNovaUI::CanvasIdentifiers::Id), { 100, 0 }, nullptr);

    grace.setProperty(ValueTreeIdentifiers::OrnamentOffset, -1, nullptr);

    ornaments.addFigureNode(ornament, host.getProperty(ArsNovaUI::CanvasIdentifiers::Id), { 150, 0 }, nullptr);
    ornaments.toggleHost(ornament, host.getProperty(ArsNovaUI::CanvasIdentifiers::Id), nullptr);
    ornaments.publish(ornament, nullptr);

    NodeFactory::createRootNode(graph, NodeInfo::Position { 0, 0, 25 }, nullptr);

    const int rootId = graph.nodeMap.getChild(graph.nodeMap.getNumChildren() - 1)
                                    .getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    juce::ValueTree first = NodeFactory::createNode(graph, rootId, ValueTreeIdentifiers::NodeData,
                                                    NodeInfo::Position { 100, 0, 25 }, nullptr);

    const int firstId = first.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    NodeFactory::createNode(graph, rootId, ValueTreeIdentifiers::NodeData, NodeInfo::Position { 150, 50, 25 }, nullptr);
    NodeFactory::createNode(graph, firstId, ValueTreeIdentifiers::NodeData, NodeInfo::Position { 200, 0, 25 }, nullptr);

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

    NodeFactory::createNode(graph, firstId, ValueTreeIdentifiers::NodeData, NodeInfo::Position { 400, -50, 25 }, nullptr);
    processor.rtGraphBuilder.rebuildAllGraphs();
    playBlocks(100);

    graph.removeNode(firstId, nullptr);
    processor.rtGraphBuilder.rebuildAllGraphs();
    playBlocks(100);

    ppq = 96.0;
    playBlocks(100);

    ppq = 4.0;
    playBlocks(100);

    processor.resetRequested.store(true);
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
    GraphState& graph = processor.graphState;

    HostPlayHead             playHead;
    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer         midi;
    int                      noteOns = 0;
    double                   ppq     = 0.0;

    NodeFactory::createRootNode(graph, NodeInfo::Position { 0, 0, 25 }, nullptr);

    const int sourceRootId = graph.nodeMap.getChild(graph.nodeMap.getNumChildren() - 1)
                                          .getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    NodeFactory::createRootNode(graph, NodeInfo::Position { 0, 600, 25 }, nullptr);

    const int targetRootId = graph.nodeMap.getChild(graph.nodeMap.getNumChildren() - 1)
                                          .getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    for (int i = 0; i < rootBoundArrows; ++i) {
        const juce::ValueTree child = NodeFactory::createNode(graph, sourceRootId, ValueTreeIdentifiers::NodeData,
                                                              NodeInfo::Position { 100 + 10 * i, 40 * (i % 8) - 140, 25 },
                                                              nullptr);

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

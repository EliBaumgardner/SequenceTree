#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Plugin/PluginProcessor.h"
#include "Graph/GraphState.h"
#include "Graph/RTGraphBuilder.h"
#include "Graph/ValueTreeIdentifiers.h"
#include "UI/Node/NodeFactory.h"

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

TEST_CASE("processBlock stays realtime-safe while playing, editing, retiming, relocating and resetting", "[realtime]")
{
    constexpr double sampleRate = 48000.0;
    constexpr int    blockSize  = 512;

    SequenceTreeAudioProcessor processor;
    GraphState& graph = processor.graphState;

    NodeFactory::createRootNode(graph, NodePosition { 0, 0, 25 }, nullptr);

    const int rootId = graph.nodeMap.getChild(graph.nodeMap.getNumChildren() - 1)
                                    .getProperty(ValueTreeIdentifiers::Id);

    juce::ValueTree first = NodeFactory::createNode(graph, rootId, ValueTreeIdentifiers::NodeData,
                                                    NodePosition { 100, 0, 25 }, nullptr);

    const int firstId = first.getProperty(ValueTreeIdentifiers::Id);

    NodeFactory::createNode(graph, rootId, ValueTreeIdentifiers::NodeData, NodePosition { 150, 50, 25 }, nullptr);
    NodeFactory::createNode(graph, firstId, ValueTreeIdentifiers::NodeData, NodePosition { 200, 0, 25 }, nullptr);

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

    graph.setNodePosition(first, NodePosition { 300, 100, 25 }, nullptr);
    processor.rtGraphBuilder.rebuildAllGraphs();
    playBlocks(100);

    playHead.position.setBpm(187.0);
    playBlocks(100);

    NodeFactory::createNode(graph, firstId, ValueTreeIdentifiers::NodeData, NodePosition { 400, -50, 25 }, nullptr);
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

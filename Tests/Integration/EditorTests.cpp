#include <catch2/catch_test_macros.hpp>

#include "Plugin/PluginProcessor.h"
#include "Plugin/PluginEditor.h"
#include "UI/Canvas/NodeCanvas.h"
#include "Graph/GraphState.h"
#include "Graph/NodeFactory.h"
#include "Graph/ValueTreeIdentifiers.h"
#include <ArsNovaUI/Canvas/CanvasIdentifiers.h>

#include <array>
#include <memory>
#include <set>
#include <vector>

struct EditorFormat
{
    const char*                       name;
    juce::AudioProcessor::WrapperType wrapperType;
};

static const std::array<EditorFormat, 3> editorFormats { {
    { "Standalone", juce::AudioProcessor::wrapperType_Standalone },
    { "VST3",       juce::AudioProcessor::wrapperType_VST3 },
    { "AU",         juce::AudioProcessor::wrapperType_AudioUnit }
} };

static constexpr int   nodeRadius   = 25;
static constexpr int   firstColumn  = 100;
static constexpr int   secondColumn = 200;
static constexpr int   firstRow     = 100;
static constexpr int   secondRow    = 200;
static constexpr float visibleInset = 100.0f;

static std::unique_ptr<SequenceTreeAudioProcessor> makeProcessor(juce::AudioProcessor::WrapperType wrapperType)
{
    juce::AudioProcessor::setTypeOfNextNewPlugin(wrapperType);

    auto processor = std::make_unique<SequenceTreeAudioProcessor>();

    juce::AudioProcessor::setTypeOfNextNewPlugin(juce::AudioProcessor::wrapperType_Undefined);

    return processor;
}

static juce::Point<int> visibleWorldOrigin(const NodeCanvas& canvas)
{
    const juce::AffineTransform screenToWorld = canvas.viewTransform.inverted();
    const juce::Point<float>    screenPoint { visibleInset, visibleInset };

    return screenPoint.transformedBy(screenToWorld).toInt();
}

static void addVariedGraph(GraphState& graph, const juce::Point<int>& origin)
{
    const int firstX  = origin.x + firstColumn;
    const int secondX = origin.x + secondColumn;
    const int firstY  = origin.y + firstRow;
    const int secondY = origin.y + secondRow;

    NodeFactory(graph, nullptr).createRootNode({ origin.x, origin.y, nodeRadius });

    const juce::ValueTree root   = graph.nodeMap.getChild(graph.nodeMap.getNumChildren() - 1);
    const int             rootId = root.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    const juce::ValueTree first   = NodeFactory(graph, nullptr).createNode(rootId, ValueTreeIdentifiers::NodeData, { firstX, origin.y, nodeRadius });
    const int             firstId = first.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    const juce::ValueTree second   = NodeFactory(graph, nullptr).createNode(firstId, ValueTreeIdentifiers::NodeData, { secondX, origin.y, nodeRadius });
    const int             secondId = second.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    const juce::ValueTree modulatorRoot   = NodeFactory(graph, nullptr).createModulatorNode(NodeInfo::Kind::ModulatorRoot, rootId, { origin.x, secondY, nodeRadius });
    const int             modulatorRootId = modulatorRoot.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    const std::vector<int> members { firstId, secondId };

    NodeFactory(graph, nullptr).createTraversalFlagNode(rootId, { firstX, firstY, nodeRadius });
    NodeFactory(graph, nullptr).createModulatorNode(NodeInfo::Kind::Modulator, modulatorRootId, { firstX, secondY, nodeRadius });
    NodeFactory(graph, nullptr).createEncapsulator(members);
}

static juce::Image drawEditor(juce::AudioProcessorEditor& editor)
{
    return editor.createComponentSnapshot(editor.getLocalBounds(), true, 1.0f);
}

static int countDistinctColours(const juce::Image& image)
{
    std::set<juce::uint32> colours;

    const juce::Image::BitmapData pixels(image, juce::Image::BitmapData::readOnly);

    for (int y = 0; y < image.getHeight(); ++y) {
        for (int x = 0; x < image.getWidth(); ++x) {
            colours.insert(pixels.getPixelColour(x, y).getARGB());
        }
    }

    return static_cast<int>(colours.size());
}

static bool imagesDiffer(const juce::Image& first, const juce::Image& second)
{
    if (first.getBounds() != second.getBounds()) {
        return true;
    }

    const juce::Image::BitmapData firstPixels(first, juce::Image::BitmapData::readOnly);
    const juce::Image::BitmapData secondPixels(second, juce::Image::BitmapData::readOnly);

    for (int y = 0; y < first.getHeight(); ++y) {
        for (int x = 0; x < first.getWidth(); ++x) {
            if (firstPixels.getPixelColour(x, y) != secondPixels.getPixelColour(x, y)) {
                return true;
            }
        }
    }

    return false;
}


TEST_CASE("the editor opens and draws under every plugin format, at its opening size and at its smallest", "[editor]")
{
    for (const EditorFormat& format : editorFormats) {
        INFO(format.name);

        const std::unique_ptr<SequenceTreeAudioProcessor> processor = makeProcessor(format.wrapperType);

        REQUIRE(processor->wrapperType == format.wrapperType);

        const std::unique_ptr<juce::AudioProcessorEditor> editor(processor->createEditor());

        REQUIRE(editor != nullptr);

        const int minimumWidth  = editor->getConstrainer()->getMinimumWidth();
        const int minimumHeight = editor->getConstrainer()->getMinimumHeight();

        CHECK(minimumWidth > 0);
        CHECK(minimumHeight > 0);
        CHECK(editor->getWidth() >= minimumWidth);
        CHECK(editor->getHeight() >= minimumHeight);

        const juce::Image openingImage = drawEditor(*editor);

        CHECK(countDistinctColours(openingImage) > 1);

        editor->setSize(minimumWidth, minimumHeight);

        const juce::Image smallestImage = drawEditor(*editor);

        CHECK(smallestImage.getWidth() == minimumWidth);
        CHECK(smallestImage.getHeight() == minimumHeight);
        CHECK(countDistinctColours(smallestImage) > 1);
    }
}

TEST_CASE("nodes, flags, modulators and groups added to the graph show up in the drawn editor under every plugin format", "[editor]")
{
    for (const EditorFormat& format : editorFormats) {
        INFO(format.name);

        const std::unique_ptr<SequenceTreeAudioProcessor> processor = makeProcessor(format.wrapperType);

        const std::unique_ptr<juce::AudioProcessorEditor> editor(processor->createEditor());

        REQUIRE(editor != nullptr);

        SequenceTreeAudioProcessorEditor& sequenceTreeEditor = static_cast<SequenceTreeAudioProcessorEditor&>(*editor);

        const juce::Point<int> origin = visibleWorldOrigin(*sequenceTreeEditor.nodeCanvas);

        const juce::Image emptyImage = drawEditor(*editor);

        addVariedGraph(processor->document.graphState, origin);

        sequenceTreeEditor.nodeCanvas->handleAsyncUpdate();

        const juce::Image graphImage = drawEditor(*editor);

        CHECK(imagesDiffer(emptyImage, graphImage));
    }
}

TEST_CASE("the plugin formats reserve room for the host sync button that the standalone does not show", "[editor]")
{
    std::vector<int> minimumWidths;

    for (const EditorFormat& format : editorFormats) {
        const std::unique_ptr<SequenceTreeAudioProcessor> processor = makeProcessor(format.wrapperType);

        const std::unique_ptr<juce::AudioProcessorEditor> editor(processor->createEditor());

        REQUIRE(editor != nullptr);

        minimumWidths.push_back(editor->getConstrainer()->getMinimumWidth());
    }

    CHECK(minimumWidths[1] > minimumWidths[0]);
    CHECK(minimumWidths[2] > minimumWidths[0]);
}

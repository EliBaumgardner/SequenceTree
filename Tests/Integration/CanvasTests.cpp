#include <catch2/catch_test_macros.hpp>

#include "Plugin/PluginProcessor.h"
#include "Graph/GraphState.h"
#include "Graph/NodeFactory.h"
#include "Graph/OrnamentLibraries.h"
#include "Graph/OrnamentFigure.h"
#include "Graph/ValueTreeIdentifiers.h"
#include <ArsNovaUI/Canvas/CanvasIdentifiers.h>
#include "Input/GraphDocument.h"
#include "Input/FigureDocument.h"
#include "UI/Canvas/NodeArrowCanvas.h"
#include "UI/Node/Node.h"
#include "UI/Ornaments/OrnamentCanvas.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

#include <memory>

struct CanvasTestFolder
{
    juce::File folder = juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("CanvasTests", "", false);

    ~CanvasTestFolder()
    {
        folder.deleteRecursively();
    }
};

struct PlainCanvas : public NodeArrowCanvas
{
    using NodeArrowCanvas::NodeArrowCanvas;

    std::unique_ptr<ArsNovaUI::CanvasNode> makeNode(const juce::ValueTree&) override
    {
        return std::make_unique<Node>(document.undoManager);
    }
};


TEST_CASE("moving a graph node carries its descendants and the group its first member opens", "[canvas][graph]")
{
    SequenceTreeAudioProcessor processor;
    GraphState&                graph = processor.document.graphState;
    GraphDocument              document(graph, processor.document.undoManager, processor.document.canvasSettings);

    const auto storedPosition = [](const juce::ValueTree& nodeTree) {
        return juce::Point<int> { nodeTree.getProperty(ArsNovaUI::CanvasIdentifiers::XPosition), nodeTree.getProperty(ArsNovaUI::CanvasIdentifiers::YPosition) };
    };

    NodeFactory(graph, nullptr).createRootNode({ 0, 0, 25 });

    const juce::ValueTree root       = graph.nodeMap.getChild(graph.nodeMap.getNumChildren() - 1);
    const juce::ValueTree first      = NodeFactory(graph, nullptr).createNode(root.getProperty(ArsNovaUI::CanvasIdentifiers::Id), ValueTreeIdentifiers::NodeData, { 100, 0, 25 });
    const juce::ValueTree second     = NodeFactory(graph, nullptr).createNode(root.getProperty(ArsNovaUI::CanvasIdentifiers::Id), ValueTreeIdentifiers::NodeData, { 100, 100, 25 });
    const juce::ValueTree grandchild = NodeFactory(graph, nullptr).createNode(first.getProperty(ArsNovaUI::CanvasIdentifiers::Id), ValueTreeIdentifiers::NodeData, { 200, 0, 25 });

    const std::vector<int> members { first.getProperty(ArsNovaUI::CanvasIdentifiers::Id), grandchild.getProperty(ArsNovaUI::CanvasIdentifiers::Id) };

    const juce::ValueTree group = NodeFactory(graph, nullptr).createEncapsulator(members);

    REQUIRE(group.isValid());

    const juce::Point<int> groupBefore = storedPosition(group);

    document.moveNode(root, { 50, 30 });

    CHECK(storedPosition(root)       == juce::Point<int> { 50, 30 });
    CHECK(storedPosition(first)      == juce::Point<int> { 150, 30 });
    CHECK(storedPosition(second)     == juce::Point<int> { 150, 130 });
    CHECK(storedPosition(grandchild) == juce::Point<int> { 250, 30 });
    CHECK(storedPosition(group)      == groupBefore + juce::Point<int> { 50, 30 });

    document.moveNode(grandchild, { 300, 30 });

    CHECK(storedPosition(grandchild) == juce::Point<int> { 300, 30 });
    CHECK(storedPosition(group)      == groupBefore + juce::Point<int> { 50, 30 });
}

TEST_CASE("an edit canvas builds, places and removes components as its document's tree changes", "[canvas]")
{
    SequenceTreeAudioProcessor   processor;
    GraphState&                  graph = processor.document.graphState;
    GraphDocument                document(graph, processor.document.undoManager, processor.document.canvasSettings);
    ArsNovaUI::CustomLookAndFeel lookAndFeel;
    PlainCanvas                  canvas(document);

    canvas.setLookAndFeel(&lookAndFeel);

    graph.nodeMap.addListener(&canvas.treeListener);

    NodeFactory(graph, nullptr).createRootNode({ 0, 0, 25 });

    const juce::ValueTree root  = graph.nodeMap.getChild(graph.nodeMap.getNumChildren() - 1);
    const juce::ValueTree child = NodeFactory(graph, nullptr).createNode(root.getProperty(ArsNovaUI::CanvasIdentifiers::Id), ValueTreeIdentifiers::NodeData, { 150, 50, 25 });

    const int childId = child.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    NodeFactory(graph, nullptr).createDanglingArrow(child, { 100, 0 }, ArrowInfo::Settings {});

    canvas.handleAsyncUpdate();

    REQUIRE(canvas.nodeManager.all().size() == 2);
    REQUIRE(canvas.arrowManager.all().size() == 2);

    CHECK(canvas.nodeManager.find(childId)->getBounds().getCentre() == juce::Point<int> { 150, 50 });

    graph.removeNode(childId, nullptr);

    canvas.handleAsyncUpdate();

    CHECK(canvas.nodeManager.all().size() == 1);
    CHECK(canvas.arrowManager.all().isEmpty());

    graph.nodeMap.removeListener(&canvas.treeListener);

    canvas.clearCanvas();
    canvas.setLookAndFeel(nullptr);
}

TEST_CASE("the figure canvas shows one chain, grows only from its last node and marks the host", "[canvas][ornament]")
{
    CanvasTestFolder             temporary;
    OrnamentLibraries            libraries;
    juce::UndoManager            undoManager;
    FigureDocument               document(libraries, undoManager);
    ArsNovaUI::CustomLookAndFeel lookAndFeel;

    juce::ValueTree canvasSettings { ValueTreeIdentifiers::CanvasSettings };

    const auto storedPosition = [](const juce::ValueTree& nodeTree) {
        return juce::Point<int> { nodeTree.getProperty(ArsNovaUI::CanvasIdentifiers::XPosition), nodeTree.getProperty(ArsNovaUI::CanvasIdentifiers::YPosition) };
    };

    libraries.load(temporary.folder);

    document.ornament = libraries.addOrnament({}, nullptr);
    document.nodes    = document.ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);

    OrnamentCanvas canvas(document, canvasSettings);

    canvas.setLookAndFeel(&lookAndFeel);

    document.nodes.addListener(&canvas.treeListener);

    canvas.rebuildFromNodeMap(document.nodes);

    document.addRootNode({ 0, 0 });

    const int firstId = document.nodes.getChild(0).getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    const juce::ValueTree middle = document.addChildNode(firstId, { 100, 0 }, {});

    const int middleId = middle.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    const juce::ValueTree last = document.addChildNode(middleId, { 200, 0 }, {});

    const int lastId = last.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    document.addDanglingArrow(last, { 100, 0 });

    canvas.handleAsyncUpdate();

    REQUIRE(canvas.nodeManager.all().size() == 3);
    REQUIRE(canvas.arrowManager.all().size() == 3);

    CHECK(document.growFrom(middleId) == -1);
    CHECK(document.growFrom(lastId)   == lastId);
    CHECK_FALSE(document.addChildNode(middleId, { 150, 50 }, {}).isValid());

    document.moveNode(document.findNode(firstId), { 0, 50 });

    canvas.handleAsyncUpdate();

    CHECK(storedPosition(middle) == juce::Point<int> { 100, 50 });
    CHECK(storedPosition(last)   == juce::Point<int> { 200, 50 });
    CHECK(canvas.nodeManager.find(lastId)->getBounds().getCentre() == juce::Point<int> { 200, 50 });

    OrnamentFigure::toggleHost(document.ornament, middleId, nullptr);

    canvas.refreshHost();

    CHECK(static_cast<Node*>(canvas.nodeManager.find(middleId))->hasInnerRim);
    CHECK_FALSE(static_cast<Node*>(canvas.nodeManager.find(firstId))->hasInnerRim);
    CHECK_FALSE(static_cast<Node*>(canvas.nodeManager.find(lastId))->hasInnerRim);

    document.nodes.removeListener(&canvas.treeListener);

    canvas.clearCanvas();
    canvas.setLookAndFeel(nullptr);
}

TEST_CASE("the grid multiplier spaces a canvas's grid lines and snap points", "[canvas][ornament]")
{
    CanvasTestFolder  temporary;
    OrnamentLibraries libraries;
    juce::UndoManager undoManager;
    FigureDocument    document(libraries, undoManager);

    juce::ValueTree canvasSettings { ValueTreeIdentifiers::CanvasSettings };

    libraries.load(temporary.folder);

    document.ornament = libraries.addOrnament({}, nullptr);
    document.nodes    = document.ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);

    canvasSettings.setProperty(ValueTreeIdentifiers::OrnamentGridMultiplier, 2.0, nullptr);

    OrnamentCanvas canvas(document, canvasSettings);

    canvas.rebuildFromNodeMap(document.nodes);

    CHECK(canvas.gridSpacing == 100.0f);
    CHECK(canvas.snapPointToGrid({ 148, 102 }) == juce::Point<int> { 148, 100 });

    canvasSettings.setProperty(ValueTreeIdentifiers::OrnamentGridMultiplier, 3.0, nullptr);

    CHECK(canvas.gridSpacing == 150.0f);
    CHECK(canvas.snapPointToGrid({ 148, 102 }) == juce::Point<int> { 150, 102 });
}

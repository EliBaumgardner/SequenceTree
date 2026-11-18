#include <catch2/catch_test_macros.hpp>

#include "Processor/PluginProcessor.h"
#include "Processor/Document/Graph/GraphState.h"
#include "Processor/Document/Graph/NodeFactory.h"
#include "Processor/Document/TypedProperties.h"
#include "Processor/Document/ValueTreeIdentifiers.h"
#include <ArsNovaUI/Canvas/CanvasIdentifiers.h>
#include "Editor/Canvas/GraphDocument.h"
#include "Editor/Canvas/NodeController.h"
#include "Editor/Canvas/NotePreviewGestures.h"
#include "Editor/Canvas/NodeCanvas.h"
#include "Editor/Nodes/Node.h"
#include "Editor/Nodes/RootNode.h"
#include "Editor/Encapsulation/Encapsulator.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

#include <algorithm>
#include <vector>

struct GestureCanvas
{
    GestureCanvas()
    {
        graph.nodeMap.addListener(&canvas.treeListener);

        canvas.setSize(canvasSide, canvasSide);
    }

    ~GestureCanvas()
    {
        graph.nodeMap.removeListener(&canvas.treeListener);
    }

    int addRoot(int x)
    {
        NodeFactory(graph, nullptr).createRootNode({ x, nodeRowY, nodeRadius });

        const int lastIndex = graph.nodeMap.getNumChildren() - 1;

        return graph.nodeMap.getChild(lastIndex).getProperty(ArsNovaUI::CanvasIdentifiers::Id);
    }

    int addChild(int parentId, int x)
    {
        const juce::ValueTree child = NodeFactory(graph, nullptr).createNode(parentId, ValueTreeIdentifiers::NodeData, { x, nodeRowY, nodeRadius });

        REQUIRE(child.isValid());

        return child.getProperty(ArsNovaUI::CanvasIdentifiers::Id);
    }

    int dragOutChild(int parentId)
    {
        const juce::ModifierKeys shiftLeftButton(juce::ModifierKeys::leftButtonModifier | juce::ModifierKeys::shiftModifier);

        const juce::Point<float> dragEnd { 0.0f, dragOutDistance };

        Node& parentNode = nodeComponent(parentId);

        const juce::MouseEvent press = mouseEventOn(parentNode, {}, shiftLeftButton);
        const juce::MouseEvent drag  = press.withNewPosition(dragEnd);

        controller.mouseDown(press);
        controller.mouseDrag(drag);
        controller.mouseUp(drag);

        const int lastIndex = graph.nodeMap.getNumChildren() - 1;

        return graph.nodeMap.getChild(lastIndex).getProperty(ArsNovaUI::CanvasIdentifiers::Id);
    }

    Node& nodeComponent(int nodeId)
    {
        Node* const node = dynamic_cast<Node*>(canvas.nodeManager.find(nodeId));

        REQUIRE(node != nullptr);

        return *node;
    }

    Encapsulator& groupComponent(int groupId)
    {
        Encapsulator* const group = dynamic_cast<Encapsulator*>(canvas.nodeManager.find(groupId));

        REQUIRE(group != nullptr);

        return *group;
    }

    std::vector<int> selectedIds()
    {
        std::vector<int> ids;

        for (auto& [nodeId, node] : canvas.nodeManager.all()) {
            if (node->isSelected) {
                ids.push_back(nodeId);
            }
        }

        std::ranges::sort(ids);

        return ids;
    }

    juce::MouseEvent mouseEventOn(juce::Component& component, juce::Point<float> position, juce::ModifierKeys modifiers)
    {
        const juce::MouseInputSource source = juce::Desktop::getInstance().getMainMouseSource();

        const juce::Time now = juce::Time::getCurrentTime();

        return juce::MouseEvent(source, position, modifiers, juce::MouseInputSource::defaultPressure, juce::MouseInputSource::defaultOrientation,
                                juce::MouseInputSource::defaultRotation, juce::MouseInputSource::defaultTiltX, juce::MouseInputSource::defaultTiltY,
                                &component, &component, now, position, now, 1, false);
    }

    static constexpr int canvasSide = 1000;
    static constexpr int nodeRowY   = 100;
    static constexpr int nodeRadius = 25;

    static constexpr float dragOutDistance = 80.0f;

    SequenceTreeAudioProcessor processor;

    GraphState& graph = processor.document.graphState;

    GraphDocument                document    { graph, processor.document.undoManager, processor.document.canvasSettings };
    ArsNovaUI::CustomLookAndFeel lookAndFeel;
    NodeCanvas                   canvas      { document, processor.bridge, processor.document.ornamentLibraries, lookAndFeel };
    NotePreviewGestures          notePreview { processor.playback.traversalSession, processor.rtGraphBuilder };
    NodeController               controller  { canvas, document, notePreview, processor.document.ornamentLibraries };
};


TEST_CASE("deleting the selection removes every selected node in one undo transaction", "[gestures][selection]")
{
    GestureCanvas fixture;

    const int rootId   = fixture.addRoot(100);
    const int firstId  = fixture.addChild(rootId, 200);
    const int secondId = fixture.addChild(firstId, 300);

    fixture.canvas.handleAsyncUpdate();

    fixture.nodeComponent(firstId).setSelectVisual(true);
    fixture.nodeComponent(secondId).setSelectVisual(true);

    fixture.controller.selectionGestures.deleteSelection();

    CHECK(fixture.graph.getNode(rootId).isValid());
    CHECK_FALSE(fixture.graph.getNode(firstId).isValid());
    CHECK_FALSE(fixture.graph.getNode(secondId).isValid());

    fixture.processor.document.undoManager.undo();

    CHECK(fixture.graph.getNode(firstId).isValid());
    CHECK(fixture.graph.getNode(secondId).isValid());
}

TEST_CASE("pasting adds copies of the copied nodes and leaves only the copies selected", "[gestures][selection]")
{
    GestureCanvas fixture;

    const int rootId  = fixture.addRoot(100);
    const int childId = fixture.addChild(rootId, 200);

    fixture.canvas.handleAsyncUpdate();

    fixture.nodeComponent(rootId).setSelectVisual(true);
    fixture.nodeComponent(childId).setSelectVisual(true);

    fixture.controller.selectionGestures.copySelection();
    fixture.controller.selectionGestures.pasteAt({ 300, 300 });

    const std::vector<int> pastedIds = fixture.selectedIds();

    CHECK(fixture.canvas.nodeManager.all().size() == 4);
    CHECK(pastedIds.size() == 2);
    CHECK(std::ranges::find(pastedIds, rootId) == pastedIds.end());
    CHECK(std::ranges::find(pastedIds, childId) == pastedIds.end());
}

TEST_CASE("two span clicks in one tree group the nodes from the first to the second, and a click on the group dissolves it", "[gestures][encapsulation]")
{
    GestureCanvas fixture;

    const int rootId   = fixture.addRoot(100);
    const int firstId  = fixture.addChild(rootId, 200);
    const int middleId = fixture.addChild(firstId, 300);
    const int lastId   = fixture.addChild(middleId, 400);

    const juce::ModifierKeys leftButton(juce::ModifierKeys::leftButtonModifier);

    fixture.canvas.handleAsyncUpdate();

    Node& firstNode = fixture.nodeComponent(firstId);
    Node& lastNode  = fixture.nodeComponent(lastId);

    fixture.canvas.setActiveTool(NodeCanvas::CanvasTool::Span);

    fixture.controller.mouseDown(fixture.mouseEventOn(firstNode, {}, leftButton));

    CHECK(firstNode.isOutlined);

    fixture.controller.mouseDown(fixture.mouseEventOn(lastNode, {}, leftButton));

    const int groupId = fixture.graph.getNode(firstId).getProperty(ValueTreeIdentifiers::EncapsulatorId, -1);

    REQUIRE(groupId != -1);
    CHECK(fixture.graph.encapsulation.memberIds(groupId) == std::vector<int> { firstId, middleId, lastId });

    fixture.canvas.handleAsyncUpdate();

    Encapsulator& groupNode = fixture.groupComponent(groupId);

    fixture.controller.mouseDown(fixture.mouseEventOn(groupNode, {}, leftButton));

    CHECK_FALSE(fixture.graph.getNode(groupId).isValid());
    CHECK(static_cast<int>(fixture.graph.getNode(firstId).getProperty(ValueTreeIdentifiers::EncapsulatorId, -1)) == -1);
}

TEST_CASE("a span click in another tree starts the span again there instead of grouping", "[gestures][encapsulation]")
{
    GestureCanvas fixture;

    const int firstRootId  = fixture.addRoot(100);
    const int firstChildId = fixture.addChild(firstRootId, 200);
    const int otherRootId  = fixture.addRoot(500);
    const int otherChildId = fixture.addChild(otherRootId, 600);
    const int nodeCount    = fixture.graph.nodeMap.getNumChildren();

    const juce::ModifierKeys leftButton(juce::ModifierKeys::leftButtonModifier);

    fixture.canvas.handleAsyncUpdate();

    Node& firstChild = fixture.nodeComponent(firstChildId);
    Node& otherChild = fixture.nodeComponent(otherChildId);

    fixture.canvas.setActiveTool(NodeCanvas::CanvasTool::Span);

    fixture.controller.mouseDown(fixture.mouseEventOn(firstChild, {}, leftButton));
    fixture.controller.mouseDown(fixture.mouseEventOn(otherChild, {}, leftButton));

    CHECK_FALSE(firstChild.isOutlined);
    CHECK(otherChild.isOutlined);
    CHECK(fixture.graph.nodeMap.getNumChildren() == nodeCount);
}

TEST_CASE("right-clicking expands a collapsed group, and collapses it from its first or last member only", "[gestures][encapsulation]")
{
    GestureCanvas fixture;

    const int rootId   = fixture.addRoot(100);
    const int firstId  = fixture.addChild(rootId, 200);
    const int middleId = fixture.addChild(firstId, 300);
    const int lastId   = fixture.addChild(middleId, 400);

    const std::vector<int> memberIds { firstId, middleId, lastId };

    const juce::ValueTree group = NodeFactory(fixture.graph, nullptr).createEncapsulator(memberIds);

    const int groupId = group.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    const juce::ModifierKeys rightButton(juce::ModifierKeys::rightButtonModifier);

    fixture.canvas.handleAsyncUpdate();

    Encapsulator& groupNode = fixture.groupComponent(groupId);

    CHECK_FALSE(groupNode.isExpanded);

    fixture.controller.mouseDown(fixture.mouseEventOn(groupNode, {}, rightButton));

    Node& middleNode = fixture.nodeComponent(middleId);
    Node& lastNode   = fixture.nodeComponent(lastId);

    CHECK(groupNode.isExpanded);

    fixture.controller.mouseDown(fixture.mouseEventOn(middleNode, {}, rightButton));

    CHECK(groupNode.isExpanded);

    fixture.controller.mouseDown(fixture.mouseEventOn(lastNode, {}, rightButton));

    CHECK_FALSE(groupNode.isExpanded);
}

TEST_CASE("a child dragged out of an expanded group's member joins the group after its parent, unless the parent is the last member", "[gestures][encapsulation]")
{
    GestureCanvas fixture;

    const int rootId  = fixture.addRoot(100);
    const int firstId = fixture.addChild(rootId, 200);
    const int lastId  = fixture.addChild(firstId, 300);

    const std::vector<int> memberIds { firstId, lastId };

    const juce::ValueTree group = NodeFactory(fixture.graph, nullptr).createEncapsulator(memberIds);

    const int groupId = group.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    fixture.canvas.handleAsyncUpdate();

    fixture.canvas.encapsulationView.expand(groupId);

    const int firstChildId = fixture.dragOutChild(firstId);

    fixture.dragOutChild(lastId);

    CHECK(fixture.graph.encapsulation.memberIds(groupId) == std::vector<int> { firstId, firstChildId, lastId });
}

TEST_CASE("leaving the span tool forgets the span start, so the next span click starts a new span", "[gestures][encapsulation]")
{
    GestureCanvas fixture;

    const int rootId    = fixture.addRoot(100);
    const int firstId   = fixture.addChild(rootId, 200);
    const int secondId  = fixture.addChild(firstId, 300);
    const int nodeCount = fixture.graph.nodeMap.getNumChildren();

    const juce::ModifierKeys leftButton(juce::ModifierKeys::leftButtonModifier);

    fixture.canvas.handleAsyncUpdate();

    Node& firstNode  = fixture.nodeComponent(firstId);
    Node& secondNode = fixture.nodeComponent(secondId);

    fixture.canvas.setActiveTool(NodeCanvas::CanvasTool::Span);

    fixture.controller.mouseDown(fixture.mouseEventOn(firstNode, {}, leftButton));

    fixture.canvas.setActiveTool(NodeCanvas::CanvasTool::None);
    fixture.canvas.setActiveTool(NodeCanvas::CanvasTool::Span);

    fixture.controller.mouseDown(fixture.mouseEventOn(secondNode, {}, leftButton));

    CHECK(secondNode.isOutlined);
    CHECK(fixture.graph.nodeMap.getNumChildren() == nodeCount);
}

TEST_CASE("in arrow mode a shift-drag on the empty canvas draws a selection box and selects the nodes inside it on release", "[gestures][controller]")
{
    GestureCanvas fixture;

    const int rootId   = fixture.addRoot(100);
    const int insideId = fixture.addChild(rootId, 200);

    const juce::ModifierKeys shiftLeftButton(juce::ModifierKeys::leftButtonModifier | juce::ModifierKeys::shiftModifier);

    fixture.addChild(rootId, 400);

    fixture.canvas.handleAsyncUpdate();

    fixture.controller.arrowGestures.arrowMode = true;

    fixture.controller.mouseDown(fixture.mouseEventOn(fixture.canvas, { 150.0f, 50.0f }, shiftLeftButton));
    fixture.controller.mouseDrag(fixture.mouseEventOn(fixture.canvas, { 250.0f, 150.0f }, shiftLeftButton));

    CHECK(fixture.canvas.selectionBounds == juce::Rectangle<int> { 150, 50, 100, 100 });

    fixture.controller.mouseUp(fixture.mouseEventOn(fixture.canvas, { 250.0f, 150.0f }, {}));

    CHECK(fixture.selectedIds() == std::vector<int> { insideId });
    CHECK(fixture.canvas.selectionBounds.isEmpty());
}

TEST_CASE("traversals typed on a root reach the saved document without playing", "[gestures][roots]")
{
    GestureCanvas fixture;

    const int rootId = fixture.addRoot(100);

    fixture.canvas.handleAsyncUpdate();

    RootNode* const rootNode = dynamic_cast<RootNode*>(fixture.canvas.nodeManager.find(rootId));

    REQUIRE(rootNode != nullptr);

    rootNode->rootRectangle.traversalEditor.commitText("4 5b");

    rootNode->rootRectangle.traversalEditor.boundValue.getValueSource().sendChangeMessage(true);

    const juce::ValueTree traversalChildrenIds = fixture.graph.getNode(rootId).getChildWithName(ValueTreeIdentifiers::TraversalChildrenIds);

    REQUIRE(traversalChildrenIds.getNumChildren() == 2);

    const TraversalReferenceProperties first (traversalChildrenIds.getChild(0));
    const TraversalReferenceProperties second(traversalChildrenIds.getChild(1));

    CHECK(first.traversalId        == 4);
    CHECK(first.traversalInstance  == 0);
    CHECK(second.traversalId       == 5);
    CHECK(second.traversalInstance == 1);
}

#include "NodeCanvas.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "../Node/Node.h"
#include "../Node/Arrow.h"
#include "../Node/Encapsulator.h"
#include "../Node/RootNode.h"
#include "../Node/Modulator.h"
#include "../Node/TraversalFlagNode.h"
#include "../../Input/GraphDocument.h"
#include "../../Graph/GraphState.h"
#include "../../Graph/RTGraphBuilder.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include <ArsNovaUI/Canvas/CanvasIdentifiers.h>
#include "../../Plugin/PluginProcessor.h"

#include <cmath>

NodeCanvas::NodeCanvas(SequenceTreeAudioProcessor& processor, GraphState& graphState, GraphDocument& graphDocument, RTGraphBuilder& rtGraphBuilder,
                       ArsNovaUI::CustomLookAndFeel& lookAndFeel)
    : NodeArrowCanvas(graphDocument),
      processor(processor),
      graphState(graphState),
      rtGraphBuilder(rtGraphBuilder),
      drainer(*this, processor.eventManager.bridge, graphState),
      encapsulationView(*this, graphState),
      ornamentMarks(*this, processor.ornamentLibraries)
{
    setWantsKeyboardFocus(true);

    setLookAndFeel(&lookAndFeel);
}

NodeCanvas::~NodeCanvas()
{
    clearCanvas();
}

void NodeCanvas::paint(juce::Graphics& graphics)
{
    ArsNovaUI::EditCanvas::paint(graphics);

    if (activeTool == CanvasTool::Paint && valueField.image.isValid()) {
        graphics.setImageResamplingQuality(juce::Graphics::mediumResamplingQuality);
        graphics.drawImage(valueField.image, getLocalBounds().toFloat());
    }

    if (!selectionBounds.isEmpty()) {
        const ArsNovaUI::Theme& theme = ArsNovaUI::CustomLookAndFeel::get(*this);

        graphics.addTransform(viewTransform);

        graphics.setColour(theme.selectionBoxColour.withAlpha(0.15f));
        graphics.fillRect(selectionBounds);

        graphics.setColour(theme.selectionBoxColour);
        graphics.drawRect(selectionBounds, 1);
    }
}

void NodeCanvas::rebuildFromNodeMap(const juce::ValueTree& stateTree)
{
    std::unordered_map<int, juce::ValueTree> rootNodeMap;

    ArsNovaUI::EditCanvas::rebuildFromNodeMap(stateTree);

    for (const juce::ValueTree& nodeTree : stateTree) {
        if (nodeTree.getType() == ValueTreeIdentifiers::RootNodeData) {
            rootNodeMap[static_cast<int>(nodeTree.getProperty(ArsNovaUI::CanvasIdentifiers::Id))] = nodeTree;
        }
    }

    encapsulationView.collapseAll();

    if (!gridOriginSet && !rootNodeMap.empty()) {
        const NodeInfo::Position rootPosition = graphState.getNodePosition(rootNodeMap.begin()->first);

        gridOrigin    = { static_cast<float>(rootPosition.xPosition), static_cast<float>(rootPosition.yPosition) };
        gridSpacing   = ArrowInfo::pixelsPerGridSpace;
        gridOriginSet = true;
    }
}

void NodeCanvas::handleAsyncUpdate()
{
    drainer.drainAll();

    const bool hasQueuedUpdates = ! asyncUpdates.empty();

    ArsNovaUI::EditCanvas::handleAsyncUpdate();

    if (activeTool == CanvasTool::Paint && hasQueuedUpdates) {
        valueField.refresh();
    }
}

void NodeCanvas::nodeCreated(ArsNovaUI::CanvasNode& canvasNode)
{
    Node& node = static_cast<Node&>(canvasNode);

    const juce::ValueTree midiNotes = node.nodeValueTree.getChildWithName(ValueTreeIdentifiers::MidiNotesData);

    const juce::var colourText = node.nodeValueTree.getProperty(ValueTreeIdentifiers::NodeColour, ArsNovaUI::CustomLookAndFeel::get(*this).defaultNodeColour.toString());

    node.midiNoteData = midiNotes.getChildWithName(ValueTreeIdentifiers::MidiNoteData);
    node.nodeColour   = juce::Colour::fromString(colourText.toString());

    node.bindToTree();
    node.setDisplayMode(displayMode);
    node.setInterceptsMouseClicks(activeTool != CanvasTool::Paint, activeTool != CanvasTool::Paint && activeTool != CanvasTool::Span);

    ornamentMarks.refresh(node);
}

void NodeCanvas::nodeAdded(ArsNovaUI::CanvasNode& node)
{
    const juce::ValueTree nodeTree = node.nodeValueTree;

    const int owningEncapsulatorId = nodeTree.getProperty(ValueTreeIdentifiers::EncapsulatorId, -1);

    if (nodeTree.getType() == ValueTreeIdentifiers::EncapsulatorData) {
        encapsulationView.collapse(node.nodeId);
    }

    if (auto* const owningEncapsulator = dynamic_cast<Encapsulator*>(nodeManager.find(owningEncapsulatorId))) {
        if (owningEncapsulator->isExpanded) {
            encapsulationView.refreshMembership(owningEncapsulatorId);
        }
        else {
            encapsulationView.collapse(owningEncapsulatorId);
        }
    }

    if (!gridOriginSet && nodeTree.getType() == ValueTreeIdentifiers::RootNodeData) {
        const NodeInfo::Position rootPosition = graphState.getNodePosition(node.nodeId);

        gridOrigin    = { static_cast<float>(rootPosition.xPosition), static_cast<float>(rootPosition.yPosition) };
        gridSpacing   = ArrowInfo::pixelsPerGridSpace;
        gridOriginSet = true;
    }
}

void NodeCanvas::nodeRemoving(ArsNovaUI::CanvasNode& node)
{
    const int encapsulatorId = node.nodeValueTree.getProperty(ValueTreeIdentifiers::EncapsulatorId, -1);

    if (auto* const encapsulator = dynamic_cast<Encapsulator*>(&node)) {
        encapsulationView.showMembers(encapsulator->memberNodeIds);
    }

    encapsulationView.refreshMembership(encapsulatorId);
}

void NodeCanvas::prepareArrow(ArsNovaUI::CanvasArrow& arrow)
{
    static_cast<Arrow&>(arrow).valueEditor->bindEditor(arrow.arrowTree, ValueTreeIdentifiers::CountLimit);
}

void NodeCanvas::connectionCreated(ArsNovaUI::CanvasArrow& arrow)
{
    const Node* const startNode = static_cast<const Node*>(arrow.startNode);

    if (startNode->nodeType == NodeInfo::Type::TraversalFlag) {
        static_cast<Arrow&>(arrow).sourceHovered = startNode->isHovered;

        arrow.initHoverState(startNode->isHovered);
    }
}

void NodeCanvas::updateApplied(const AsyncUpdate& update)
{
    Node* const node = static_cast<Node*>(nodeManager.find(update.nodeId));

    if (node == nullptr) {
        return;
    }

    if (update.type == AsyncUpdateType::NodeColourChanged) {
        const juce::var colourText = node->nodeValueTree.getProperty(ValueTreeIdentifiers::NodeColour, ArsNovaUI::CustomLookAndFeel::get(*this).defaultNodeColour.toString());

        node->nodeColour = juce::Colour::fromString(colourText.toString());

        node->repaint();

        if (const auto* const encapsulator = dynamic_cast<const Encapsulator*>(node)) {
            encapsulationView.recolourGroup(*encapsulator);
        }
    }

    if (update.type == AsyncUpdateType::NodeMarkChanged) {
        ornamentMarks.refresh(*node);
    }
}

void NodeCanvas::setActiveTool(CanvasTool tool)
{
    const CanvasTool previousTool = activeTool;

    activeTool       = tool;
    spanAnchorNodeId = -1;

    setInterceptsClicks(tool != CanvasTool::Paint, tool != CanvasTool::Paint && tool != CanvasTool::Span);
    clearOutlines();

    if (previousTool == CanvasTool::NotePreview) {
        processor.traversalSession.previewRequests.push({ RTPreviewRequest::Kind::Stop });
    }

    if (onActiveToolChanged) {
        onActiveToolChanged(tool);
    }

    repaint();

    switch (tool) {
        case CanvasTool::None:
            setMouseCursor(juce::MouseCursor::NormalCursor);
            break;

        case CanvasTool::Paint:
            valueField.updateBrushCursor();
            valueField.refresh();
            break;

        case CanvasTool::Span:
            setMouseCursor(spanCursor());
            break;

        case CanvasTool::NotePreview:
            setMouseCursor(quaverCursor());
            break;

        case CanvasTool::Ornament:
            setMouseCursor(juce::MouseCursor::CrosshairCursor);
            break;
    }
}

void NodeCanvas::applyViewTransform(const juce::AffineTransform& newViewTransform, float zoom)
{
    viewTransform       = newViewTransform;
    modelTransform      = newViewTransform.inverted();
    valueField.viewZoom = zoom;

    valueField.updateBrushCursor();

    valueField.refresh();
}

void NodeCanvas::setProcessorPlayback(bool isPlaying)
{
    start = isPlaying;

    processor.isPlaying.store(start);

    if (isPlaying) {
        equipRootTraversals();

        resumeAllProgress();
    }
    else {
        pauseAllProgress();
    }

    rtGraphBuilder.handleUpdateNowIfNeeded();
}

void NodeCanvas::setDisplayMode(NodeInfo::DisplayMode mode)
{
    displayMode = mode;

    for (auto& [nodeId, node] : nodeManager.all()) {
        static_cast<Node*>(node.get())->setDisplayMode(mode);
    }
}

void NodeCanvas::setInterceptsClicks(bool shouldIntercept, bool shouldChildrenIntercept)
{
    for (auto& [nodeId, node] : nodeManager.all()) {
        if (node != nullptr) {
            node->setInterceptsMouseClicks(shouldIntercept, shouldChildrenIntercept);
        }
    }
}

void NodeCanvas::resetTrail(int trailId)
{
    for (ArsNovaUI::CanvasArrow* const arrow : arrowManager.all()) {
        if (arrow != nullptr) {
            static_cast<Arrow*>(arrow)->resetProgress(trailId);
        }
    }
}

void NodeCanvas::fadeFlagArrows(juce::Point<float> cursor)
{
    for (ArsNovaUI::CanvasArrow* const arrow : arrowManager.all()) {
        if (arrow->startNode == nullptr || arrow->endNode == nullptr) {
            continue;
        }

        if (static_cast<const Node*>(arrow->startNode)->nodeType != NodeInfo::Type::TraversalFlag) {
            continue;
        }

        Arrow* const flagArrow = static_cast<Arrow*>(arrow);

        const float distance = ArsNovaUI::CanvasHitTester::distanceToSegment(cursor, arrow->startNode->getNodeCentre().toFloat(), arrow->endNode->getNodeCentre().toFloat());

        const bool nearby = distance < flagProximityRadius;

        if (nearby != flagArrow->proximityHovered) {
            flagArrow->proximityHovered = nearby;

            flagArrow->setHoverFade(flagArrow->sourceHovered || flagArrow->proximityHovered);
        }
    }
}

void NodeCanvas::clearHighlights()
{
    for (auto& [nodeId, node] : nodeManager.all()) {
        if (node != nullptr) {
            static_cast<Node*>(node.get())->setHighlightVisual(-1, false, juce::Colours::white);
        }
    }
}

void NodeCanvas::clearOutlines()
{
    for (auto& [nodeId, node] : nodeManager.all()) {
        if (node != nullptr) {
            Node* const outlinedNode = static_cast<Node*>(node.get());

            outlinedNode->isOutlined = false;

            outlinedNode->repaint();
        }
    }
}

void NodeCanvas::equipRootTraversals()
{
    for (auto& [nodeId, node] : nodeManager.all()) {
        if (auto* rootNode = dynamic_cast<RootNode*>(node.get())) {
            rootNode->equipTraversals();
        }
    }
}

void NodeCanvas::resetAllProgress()
{
    for (ArsNovaUI::CanvasArrow* const arrow : arrowManager.all()) {
        if (arrow != nullptr) {
            static_cast<Arrow*>(arrow)->resetProgress();
        }
    }
}

void NodeCanvas::pauseAllProgress()
{
    const double nowMs = juce::Time::getMillisecondCounterHiRes();

    for (ArsNovaUI::CanvasArrow* const arrow : arrowManager.all()) {
        Arrow* const trailArrow = static_cast<Arrow*>(arrow);

        if (trailArrow == nullptr || trailArrow->trailsPaused) {
            continue;
        }

        trailArrow->trailsPaused = true;
        trailArrow->pausedAtMs   = nowMs;
    }
}

void NodeCanvas::resumeAllProgress()
{
    for (ArsNovaUI::CanvasArrow* const arrow : arrowManager.all()) {
        if (arrow != nullptr) {
            static_cast<Arrow*>(arrow)->resumeProgress();
        }
    }
}

juce::Point<int> NodeCanvas::drawnPosition(const juce::ValueTree& nodeTree) const
{
    const int nodeId         = nodeTree.getProperty(ArsNovaUI::CanvasIdentifiers::Id);
    const int encapsulatorId = nodeTree.getProperty(ValueTreeIdentifiers::EncapsulatorId, -1);

    const juce::ValueTree encapsulator = graphState.getNode(encapsulatorId);

    auto* const owningEncapsulator = dynamic_cast<Encapsulator*>(nodeManager.find(encapsulatorId));

    const bool isDrawnAtOwner = encapsulator.isValid() && (owningEncapsulator == nullptr || ! owningEncapsulator->isExpanded);

    juce::Point<int> position = ArsNovaUI::EditCanvas::drawnPosition(nodeTree);

    if (isDrawnAtOwner) {
        position = ArsNovaUI::EditCanvas::drawnPosition(encapsulator);
    }

    return position + encapsulationView.collapsedSpanShift(nodeId);
}

std::unique_ptr<ArsNovaUI::CanvasNode> NodeCanvas::makeNode(const juce::ValueTree& nodeTree)
{
    const juce::Identifier treeType = nodeTree.getType();

    juce::UndoManager& undoManager = document.undoManager;

    std::unique_ptr<Node> node;

    if (treeType == ValueTreeIdentifiers::RootNodeData) {
        node = std::make_unique<RootNode>(undoManager, graphState);
    }
    else if (treeType == ValueTreeIdentifiers::NodeData) {
        node = std::make_unique<Node>(undoManager);
        node->takesArrows = true;
    }
    else if (treeType == ValueTreeIdentifiers::AlternativeNodeData) {
        node = std::make_unique<Node>(undoManager);
        node->isAlternativeNode = true;
    }
    else if (treeType == ValueTreeIdentifiers::TraversalFlagData) {
        node = std::make_unique<TraversalFlagNode>(undoManager, graphState);
    }
    else if (treeType == ValueTreeIdentifiers::ModulatorData
          || treeType == ValueTreeIdentifiers::ModulatorRootData) {
        node = std::make_unique<Modulator>(undoManager);
    }
    else if (treeType == ValueTreeIdentifiers::AlternativeModulatorData) {
        node = std::make_unique<Modulator>(undoManager);
        node->isAlternativeNode = true;
    }
    else if (treeType == ValueTreeIdentifiers::EncapsulatorData) {
        node = std::make_unique<Encapsulator>(undoManager, graphState, *this);
    }

    return node;
}
juce::MouseCursor NodeCanvas::spanCursor() const
{
    juce::Image cursorImage(juce::Image::ARGB, spanCursorSize, spanCursorSize, true);

    juce::Graphics cursorGraphics(cursorImage);

    cursorGraphics.setColour(juce::Colours::black);
    cursorGraphics.drawRect(cursorImage.getBounds().reduced(1), 1);

    return juce::MouseCursor(cursorImage, spanCursorSize / 2, spanCursorSize / 2);
}

juce::MouseCursor NodeCanvas::quaverCursor() const
{
    juce::Image cursorImage(juce::Image::ARGB, quaverCursorSize, quaverCursorSize, true);

    juce::Graphics cursorGraphics(cursorImage);

    const auto square = cursorImage.getBounds().toFloat().reduced(1.0f);

    const float side     = square.getWidth();
    const float headX    = square.getX() + side * 0.34f;
    const float headY    = square.getBottom() - side * 0.2f;
    const float stemX    = headX + side * 0.19f;
    const float stemTopY = square.getY() + side * 0.04f;

    juce::Path head;
    juce::Path stem;
    juce::Path flag;

    cursorGraphics.setColour(juce::Colours::black);

    head.addEllipse(juce::Rectangle<float>(side * 0.44f, side * 0.3f).withCentre({ headX, headY }));
    head.applyTransform(juce::AffineTransform::rotation(-0.35f, headX, headY));

    cursorGraphics.fillPath(head);

    stem.addLineSegment({ stemX, headY - side * 0.04f, stemX, stemTopY }, juce::jmax(1.0f, side * 0.08f));

    cursorGraphics.fillPath(stem);

    flag.startNewSubPath(stemX, stemTopY);
    flag.cubicTo(stemX + side * 0.08f, stemTopY + side * 0.18f, stemX + side * 0.36f, stemTopY + side * 0.24f, stemX + side * 0.26f, stemTopY + side * 0.56f);
    flag.cubicTo(stemX + side * 0.28f, stemTopY + side * 0.34f, stemX + side * 0.12f, stemTopY + side * 0.3f, stemX, stemTopY + side * 0.26f);
    flag.closeSubPath();

    cursorGraphics.fillPath(flag);

    return juce::MouseCursor(cursorImage, quaverCursorSize / 2, quaverCursorSize / 2);
}

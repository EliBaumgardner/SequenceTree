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
#include "../../Plugin/PluginProcessor.h"

#include <cmath>

NodeCanvas::NodeCanvas(SequenceTreeAudioProcessor& processor, GraphState& graphState, GraphDocument& graphDocument, RTGraphBuilder& rtGraphBuilder,
                       ArsNovaUI::CustomLookAndFeel& lookAndFeel)
    : EditCanvas(graphDocument),
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
    EditCanvas::paint(graphics);

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

    EditCanvas::rebuildFromNodeMap(stateTree);

    for (const juce::ValueTree& nodeTree : stateTree) {
        if (nodeTree.getType() == ValueTreeIdentifiers::RootNodeData) {
            rootNodeMap[static_cast<int>(nodeTree.getProperty(ValueTreeIdentifiers::Id))] = nodeTree;
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

    EditCanvas::handleAsyncUpdate();

    if (activeTool == CanvasTool::Paint && hasQueuedUpdates) {
        valueField.refresh();
    }
}

void NodeCanvas::nodeCreated(Node& node)
{
    const juce::ValueTree midiNotes = node.nodeValueTree.getChildWithName(ValueTreeIdentifiers::MidiNotesData);

    const juce::var colourText = node.nodeValueTree.getProperty(ValueTreeIdentifiers::NodeColour, ArsNovaUI::CustomLookAndFeel::get(*this).defaultNodeColour.toString());

    node.midiNoteData = midiNotes.getChildWithName(ValueTreeIdentifiers::MidiNoteData);
    node.nodeColour   = juce::Colour::fromString(colourText.toString());

    node.bindToTree();
    node.setDisplayMode(nodeManager.displayMode);
    node.setInterceptsMouseClicks(activeTool != CanvasTool::Paint, activeTool != CanvasTool::Paint && activeTool != CanvasTool::Span);

    ornamentMarks.refresh(node);
}

void NodeCanvas::nodeAdded(Node& node)
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

void NodeCanvas::nodeRemoving(Node& node)
{
    const int encapsulatorId = node.nodeValueTree.getProperty(ValueTreeIdentifiers::EncapsulatorId, -1);

    if (auto* const encapsulator = dynamic_cast<Encapsulator*>(&node)) {
        encapsulationView.showMembers(encapsulator->memberNodeIds);
    }

    encapsulationView.refreshMembership(encapsulatorId);
}

void NodeCanvas::prepareArrow(Arrow& arrow)
{
    arrow.valueEditor->bindEditor(arrow.arrowTree, ValueTreeIdentifiers::CountLimit);
}

void NodeCanvas::updateApplied(const AsyncUpdate& update)
{
    Node* const node = nodeManager.find(update.nodeId);

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

    if (update.type == AsyncUpdateType::OrnamentChanged) {
        ornamentMarks.refresh(*node);
    }
}

void NodeCanvas::setActiveTool(CanvasTool tool)
{
    const CanvasTool previousTool = activeTool;

    activeTool       = tool;
    spanAnchorNodeId = -1;

    nodeManager.setInterceptsClicks(tool != CanvasTool::Paint, tool != CanvasTool::Paint && tool != CanvasTool::Span);
    nodeManager.clearOutlines();

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
        nodeManager.equipRootTraversals();

        arrowManager.resumeAllProgress();
    }
    else {
        arrowManager.pauseAllProgress();
    }

    rtGraphBuilder.handleUpdateNowIfNeeded();
}

juce::Point<int> NodeCanvas::drawnPosition(const juce::ValueTree& nodeTree) const
{
    const int nodeId         = nodeTree.getProperty(ValueTreeIdentifiers::Id);
    const int encapsulatorId = nodeTree.getProperty(ValueTreeIdentifiers::EncapsulatorId, -1);

    const juce::ValueTree encapsulator = graphState.getNode(encapsulatorId);

    auto* const owningEncapsulator = dynamic_cast<Encapsulator*>(nodeManager.find(encapsulatorId));

    const bool isDrawnAtOwner = encapsulator.isValid() && (owningEncapsulator == nullptr || ! owningEncapsulator->isExpanded);

    juce::Point<int> position = EditCanvas::drawnPosition(nodeTree);

    if (isDrawnAtOwner) {
        position = EditCanvas::drawnPosition(encapsulator);
    }

    return position + encapsulationView.collapsedSpanShift(nodeId);
}

std::unique_ptr<Node> NodeCanvas::makeNode(const juce::ValueTree& nodeTree)
{
    const juce::Identifier treeType = nodeTree.getType();

    juce::UndoManager& undoManager = document.undoManager;

    std::unique_ptr<Node> node;

    if (treeType == ValueTreeIdentifiers::RootNodeData) {
        node = std::make_unique<RootNode>(undoManager, graphState);
    }
    else if (treeType == ValueTreeIdentifiers::NodeData) {
        node = std::make_unique<Node>(undoManager);
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

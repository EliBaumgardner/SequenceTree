#include "NodeCanvas.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../Node/Node.h"
#include "../Node/Arrow.h"
#include "../Node/Encapsulator.h"
#include "../../Graph/GraphState.h"
#include "../../Graph/RTGraphBuilder.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../../Plugin/PluginProcessor.h"

#include <cmath>

NodeCanvas::NodeCanvas(const ApplicationContext& context) : applicationContext(context)
{
    setWantsKeyboardFocus(true);
    setLookAndFeel(applicationContext.lookAndFeel);
}

NodeCanvas::~NodeCanvas()
{
    clearCanvas();
}

void NodeCanvas::paint(juce::Graphics& graphics)
{
    CustomLookAndFeel::get(*this).drawCanvas(graphics, *this);

    if (paintMode && valueField.image.isValid()) {
        graphics.setImageResamplingQuality(juce::Graphics::mediumResamplingQuality);
        graphics.drawImage(valueField.image, getLocalBounds().toFloat());
    }

    if (!selectionBounds.isEmpty()) {
        const Theme& theme = CustomLookAndFeel::get(*this);

        graphics.addTransform(viewTransform);

        graphics.setColour(theme.selectionBoxColour.withAlpha(0.15f));
        graphics.fillRect(selectionBounds);

        graphics.setColour(theme.selectionBoxColour);
        graphics.drawRect(selectionBounds, 1);
    }
}

void NodeCanvas::clearCanvas()
{
    arrowManager.clear();
    nodeManager.clear();

    gridOriginSet = false;
    gridVisible   = false;
}

void NodeCanvas::childrenChanged()
{
    for (juce::Component* child : getChildren()) {
        child->setTransform(viewTransform);
    }
}

void NodeCanvas::enqueueAsyncUpdate(const AsyncUpdate& update)
{
    asyncUpdates.push_back(update);
    triggerAsyncUpdate();
}

void NodeCanvas::setProcessorPlayback(bool isPlaying)
{
    start = isPlaying;

    applicationContext.processor->isPlaying.store(start);

    if (isPlaying) {
        nodeManager.equipRootTraversals();
        arrowManager.resumeAllProgress();
    }
    else {
        arrowManager.pauseAllProgress();
    }

    applicationContext.rtGraphBuilder->handleUpdateNowIfNeeded();
}

void NodeCanvas::rebuildFromNodeMap(const juce::ValueTree& stateTree)
{
    asyncUpdates.clear();
    cancelPendingUpdate();

    clearCanvas();

    std::unordered_map<int, juce::ValueTree> rootNodeMap;
    std::vector<NodePair>                    nodePairs;

    for (int nodeIndex = 0; nodeIndex < stateTree.getNumChildren(); ++nodeIndex) {
        const juce::ValueTree nodeValueTree = stateTree.getChild(nodeIndex);
        const juce::ValueTree childIds      = nodeValueTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);
        const int             nodeId        = nodeValueTree.getProperty(ValueTreeIdentifiers::Id);

        if (nodeValueTree.getType() == ValueTreeIdentifiers::RootNodeData) {
            rootNodeMap[nodeId] = nodeValueTree;
        }

        for (int childIndex = 0; childIndex < childIds.getNumChildren(); ++childIndex) {
            nodePairs.push_back({ nodeId, childIds.getChild(childIndex).getProperty(ValueTreeIdentifiers::Id) });
        }

        nodeManager.instantiateFromTree(nodeValueTree);
    }

    for (auto [parentNodeId, childNodeId] : nodePairs) {
        Node* const parentNode = nodeManager.find(parentNodeId);
        Node* const childNode  = nodeManager.find(childNodeId);
        Node*       startNode  = parentNode;
        Node*       endNode    = childNode;

        if (parentNode == nullptr || childNode == nullptr) {
            continue;
        }

        if (childNode->nodeValueTree.getType() == ValueTreeIdentifiers::AlternativeNodeData
            || childNode->nodeValueTree.getType() == ValueTreeIdentifiers::AlternativeModulatorData) {
            startNode = childNode;
            endNode   = parentNode;
        }

        if (startNode->nodeArrows.count(endNode->nodeId) > 0) {
            continue;
        }

        arrowManager.connect(startNode, endNode);
        arrowManager.refreshFor(endNode);
    }

    for (auto& [nodeId, node] : nodeManager.all()) {
        arrowManager.rebuildDanglingForNode(nodeId);
    }

    encapsulationView.collapseAll();

    if (!gridOriginSet && !rootNodeMap.empty()) {
        const NodePosition rootPosition = applicationContext.graphState->getNodePosition(rootNodeMap.begin()->first);

        gridOrigin    = { static_cast<float>(rootPosition.xPosition), static_cast<float>(rootPosition.yPosition) };
        gridSpacing   = ArrowInfo::pixelsPerGridSpace;
        gridOriginSet = true;
    }
}

void NodeCanvas::handleAsyncUpdate()
{
    drainer.drainAll();

    std::vector<AsyncUpdate> pendingUpdates;

    pendingUpdates.swap(asyncUpdates);

    for (const AsyncUpdate& asyncUpdate : pendingUpdates) {
        const int nodeId = asyncUpdate.nodeId;

        switch (asyncUpdate.type) {
            case AsyncUpdateType::NodeAdded:
                nodeManager.add(nodeId);
                break;
            case AsyncUpdateType::NodeRemoved:
                nodeManager.remove(nodeId);
                break;
            case AsyncUpdateType::NodeMoved:
                nodeManager.setPosition(nodeId);
                break;
            case AsyncUpdateType::ValueChanged:
            case AsyncUpdateType::ArrowDurationChanged:
                if (Node* const changedNode = nodeManager.find(nodeId)) {
                    arrowManager.refreshFor(changedNode);
                }
                break;
            case AsyncUpdateType::DanglingArrowsChanged:
                arrowManager.rebuildDanglingForNode(nodeId);
                break;
            case AsyncUpdateType::ArrowAdded:
                arrowManager.handleArrowAdded(nodeId, asyncUpdate.rootNodeId);
                break;
            case AsyncUpdateType::ArrowRemoved:
                arrowManager.remove(arrowManager.find(nodeId, asyncUpdate.rootNodeId));
                break;
            case AsyncUpdateType::ArrowInfoChanged:
                arrowManager.handleArrowInfoChanged(nodeId, asyncUpdate.rootNodeId);
                break;
            case AsyncUpdateType::NodeColourChanged:
                if (Node* const recolouredNode = nodeManager.find(nodeId)) {
                    const juce::var colourText = recolouredNode->nodeValueTree.getProperty(ValueTreeIdentifiers::NodeColour, Node::defaultNodeColour.toString());

                    recolouredNode->nodeColour = juce::Colour::fromString(colourText.toString());

                    recolouredNode->repaint();

                    if (const auto* const encapsulator = dynamic_cast<const Encapsulator*>(recolouredNode)) {
                        encapsulationView.recolourGroup(*encapsulator);
                    }
                }
                break;
            case AsyncUpdateType::None:
                break;
        }
    }

    if (paintMode && !pendingUpdates.empty()) {
        valueField.refresh();
    }
}

void NodeCanvas::setPaintMode(bool enabled)
{
    paintMode = enabled;

    nodeManager.setInterceptsClicks(!enabled, !enabled && !spanMode);

    if (enabled) {
        valueField.updateBrushCursor();
        valueField.refresh();
    }
    else {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void NodeCanvas::setSpanMode(bool enabled)
{
    spanMode         = enabled;
    spanAnchorNodeId = -1;

    nodeManager.setInterceptsClicks(!paintMode, !paintMode && !enabled);
    nodeManager.clearOutlines();

    if (! enabled) {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        return;
    }

    juce::Image cursorImage(juce::Image::ARGB, spanCursorSize, spanCursorSize, true);
    juce::Graphics cursorGraphics(cursorImage);

    cursorGraphics.setColour(juce::Colours::black);
    cursorGraphics.drawRect(cursorImage.getBounds().reduced(1), 1);

    setMouseCursor(juce::MouseCursor(cursorImage, spanCursorSize / 2, spanCursorSize / 2));
}

void NodeCanvas::setQuaverMode(QuaverMode mode)
{
    quaverMode = mode;

    if (mode == QuaverMode::Off) {
        applicationContext.processor->traversalSession.previewRequests.push({ RTPreviewRequest::Kind::Stop });
        setMouseCursor(juce::MouseCursor::NormalCursor);
        return;
    }

    juce::Image    cursorImage(juce::Image::ARGB, quaverCursorSize, quaverCursorSize, true);
    juce::Graphics cursorGraphics(cursorImage);
    const auto     square   = cursorImage.getBounds().toFloat().reduced(1.0f);
    const float    side     = square.getWidth();
    const float    headX    = square.getX() + side * 0.34f;
    const float    headY    = square.getBottom() - side * 0.2f;
    const float    stemX    = headX + side * 0.19f;
    const float    stemTopY = square.getY() + side * 0.04f;
    juce::Path     head;
    juce::Path     stem;
    juce::Path     flag;

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

    setMouseCursor(juce::MouseCursor(cursorImage, quaverCursorSize / 2, quaverCursorSize / 2));
}

void NodeCanvas::showGrid()
{
    if (gridOriginSet) {
        gridVisible = true;

        repaint();
    }
}

void NodeCanvas::hideGrid()
{
    if (gridVisible) {
        gridVisible = false;

        repaint();
    }
}

juce::Point<int> NodeCanvas::snapPointToGrid(juce::Point<int> point) const
{
    if (!gridOriginSet) {
        return point;
    }

    const float      snapThreshold = 5.0f;
    const float      snappedX      = gridOrigin.x + std::round((static_cast<float>(point.x) - gridOrigin.x) / gridSpacing) * gridSpacing;
    const float      snappedY      = gridOrigin.y + std::round((static_cast<float>(point.y) - gridOrigin.y) / gridSpacing) * gridSpacing;
    juce::Point<int> result        = point;

    if (std::abs(static_cast<float>(point.x) - snappedX) < snapThreshold) {
        result.x = static_cast<int>(snappedX);
    }

    if (std::abs(static_cast<float>(point.y) - snappedY) < snapThreshold) {
        result.y = static_cast<int>(snappedY);
    }

    return result;
}

void NodeCanvas::cancelPendingUpdatesFor(int nodeId)
{
    std::erase_if(asyncUpdates, [nodeId](const AsyncUpdate& update) {
        return update.nodeId == nodeId
            && update.type != AsyncUpdateType::NodeRemoved;
    });
}

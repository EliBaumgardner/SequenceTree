#include <juce_gui_basics/juce_gui_basics.h>

#include "../Theme/CustomLookAndFeel.h"
#include "../../Graph/ValueTreeState.h"
#include "../../Plugin/PluginProcessor.h"
#include "../Node/NodeTextEditor.h"
#include "../Node/NodeArrow.h"
#include "../../Graph/ValueTreeIdentifiers.h"

#include "../Node/Node.h"
#include "../Node/RootNode.h"
#include "NodeCanvas.h"
#include "../../Audio/EventManager.h"
#include "../../Graph/RTGraphBuilder.h"

#include "../Node/Modulator.h"


// Canvas Related Functions //
NodeCanvas::NodeCanvas(ApplicationContext& context) : applicationContext(context)
{
    setPaintingIsUnclipped(true);
    setLookAndFeel(applicationContext.lookAndFeel);
}

NodeCanvas::~NodeCanvas()
{
    stopTimer();
    clearCanvas();
}


void NodeCanvas::paint(juce::Graphics& g)
{
    CustomLookAndFeel::get(*this).drawCanvas(g, *this);


    if (paintMode && valueField.isValid()) {
        g.setImageResamplingQuality(juce::Graphics::mediumResamplingQuality);
        g.drawImage(valueField, getLocalBounds().toFloat());
    }
}

void NodeCanvas::enqueueAsyncUpdate(const AsyncUpdate& update)
{
    asyncUpdates.push_back(update);
    triggerAsyncUpdate();
}

void NodeCanvas::handleAsyncUpdate() {
    drainHighlightFifo();
    drainArrowResetFifo();
    drainProgressFifo();
    drainCountFifo();

    for (auto& asyncUpdate  : asyncUpdates) {
        int nodeId = asyncUpdate.nodeId;

        AsyncUpdateType updateType = asyncUpdate.type;

        if (updateType == AsyncUpdateType::NodeAdded) {
            addNodeToCanvas(nodeId);
        }
        else if (updateType == AsyncUpdateType::NodeRemoved) {
            removeNodeFromCanvas(nodeId);

            int rootNodeId = asyncUpdate.rootNodeId;
            if (rootNodeId != nodeId) {

                juce::ValueTree rootTree = ValueTreeState::getNode(rootNodeId);
                if (rootTree.isValid()) {
                    applicationContext.rtGraphBuilder->makeRTGraph(rootTree);
                }
            } else {

                auto emptyGraph = std::make_shared<RTGraph>();
                emptyGraph->graphID = rootNodeId;
                applicationContext.processor->setNewGraph(emptyGraph);
            }
        }
        else if (updateType == AsyncUpdateType::NodeMoved) {
            setNodePosition(nodeId);
            applicationContext.rtGraphBuilder->updateDurationMap(nodeId);
        }
        else if (updateType == AsyncUpdateType::DurationOnly) {
            applicationContext.rtGraphBuilder->updateDurationMap(nodeId);
        }
    }

    const bool fieldNeedsRefresh = ! asyncUpdates.empty();
    asyncUpdates.clear();

    if (paintMode && fieldNeedsRefresh) {
        refreshValueField();
    }
}

void NodeCanvas::drainProgressFifo()
{
    auto& bridge = applicationContext.processor->eventManager.bridge;
    const auto scope = bridge.progressFifo.read(bridge.progressFifo.getNumReady());

    auto apply = [this](const AudioUIBridge::ProgressCommand& cmd)
    {
        auto parentIt = nodeMap.find(cmd.parentNodeId);
        if (parentIt == nodeMap.end()) {
            return;
        }

        auto arrowIt = parentIt->second->nodeArrows.find(cmd.childNodeId);
        if (arrowIt == parentIt->second->nodeArrows.end() || arrowIt->second == nullptr) {
            return;
        }

        arrowIt->second->startProgress(cmd.durationMs);
    };

    for (int i = 0; i < scope.blockSize1; ++i)
        apply(bridge.progressBuffer[static_cast<size_t>(scope.startIndex1 + i)]);
    for (int i = 0; i < scope.blockSize2; ++i)
        apply(bridge.progressBuffer[static_cast<size_t>(scope.startIndex2 + i)]);
}

void NodeCanvas::drainArrowResetFifo() {
    auto& bridge = applicationContext.processor->eventManager.bridge;
    const auto scope = bridge.arrowResetFifo.read(bridge.arrowResetFifo.getNumReady());

    auto apply = [this](const AudioUIBridge::ResetCommand cmd) {
            resetGraphArrowProgress(cmd.rootId);
    };

    for (int i = 0; i < scope.blockSize1; ++i)
        apply(bridge.arrowResetBuffer[static_cast<size_t>(scope.startIndex1 + i)]);
    for (int i = 0; i < scope.blockSize2; ++i)
        apply(bridge.arrowResetBuffer[static_cast<size_t>(scope.startIndex2 + i)]);
}

void NodeCanvas::resetAllArrowProgress()
{
    for (NodeArrow* arrow : nodeArrows)
        if (arrow != nullptr) {
            arrow->resetProgress();
        }
}

void NodeCanvas::resetGraphArrowProgress(int graphId)
{
    for (NodeArrow* arrow : nodeArrows)
    {
        if (arrow == nullptr || arrow->parentNode == nullptr) {
            continue;
        }

        const int parentId = arrow->parentNode->getComponentID().getIntValue();
        const juce::ValueTree arrowRoot = ValueTreeState::getRootNode(parentId);
        if (! arrowRoot.isValid()) {

            continue;
        }

        if (static_cast<int>(arrowRoot.getProperty(ValueTreeIdentifiers::Id)) == graphId) {
            arrow->resetProgress();
        }
    }
}

void NodeCanvas::drainCountFifo()
{
    auto& bridge = applicationContext.processor->eventManager.bridge;
    const auto scope = bridge.countFifo.read(bridge.countFifo.getNumReady());

    auto apply = [this](const AudioUIBridge::CountCommand& cmd)
    {
        auto it = nodeMap.find(cmd.nodeId);
        if (it == nodeMap.end()) {
            return;
        }

        Node* node = it->second;
        node->displayCurrentCount = cmd.currentCount;
        node->displayCountLimit   = juce::jmax(1, cmd.countLimit);
        node->repaint();
    };

    for (int i = 0; i < scope.blockSize1; ++i)
        apply(bridge.countBuffer[static_cast<size_t>(scope.startIndex1 + i)]);
    for (int i = 0; i < scope.blockSize2; ++i)
        apply(bridge.countBuffer[static_cast<size_t>(scope.startIndex2 + i)]);
}

void NodeCanvas::drainHighlightFifo()
{
    auto& bridge = applicationContext.processor->eventManager.bridge;
    const auto scope = bridge.highlightFifo.read(bridge.highlightFifo.getNumReady());

    for (int i = 0; i < scope.blockSize1; ++i)
    {
        auto& cmd = bridge.highlightBuffer[static_cast<size_t>(scope.startIndex1 + i)];
        auto it = nodeMap.find(cmd.nodeId);
        if (it != nodeMap.end()) {
            it->second->setHighlightVisual(cmd.shouldHighlight);
        }
    }
    for (int i = 0; i < scope.blockSize2; ++i)
    {
        auto& cmd = bridge.highlightBuffer[static_cast<size_t>(scope.startIndex2 + i)];
        auto it = nodeMap.find(cmd.nodeId);
        if (it != nodeMap.end()) {
            it->second->setHighlightVisual(cmd.shouldHighlight);
        }
    }
}

Node* NodeCanvas::instantiateNodeFromTree(const juce::ValueTree& nodeValueTree)
{
    jassert(nodeValueTree.isValid());

    const juce::Identifier treeType = nodeValueTree.getType();
    const int nodeId = nodeValueTree.getProperty(ValueTreeIdentifiers::Id);

    std::unique_ptr<Node> node;
    if (treeType == ValueTreeIdentifiers::RootNodeData) {
        node = std::make_unique<RootNode>(applicationContext);
    }
    else if (treeType == ValueTreeIdentifiers::NodeData) {
        node = std::make_unique<Node>(applicationContext);
    }
    else if (treeType == ValueTreeIdentifiers::AlternativeNodeData) {
        node = std::make_unique<Node>(applicationContext);
        node.get()->isAlternativeNode = true;
    }
    else if (treeType == ValueTreeIdentifiers::ModulatorData
          || treeType == ValueTreeIdentifiers::ModulatorRootData) {
        node = std::make_unique<Modulator>(applicationContext);
    }

    jassert(node);

    juce::ValueTree midiNotes = nodeValueTree.getChildWithName(ValueTreeIdentifiers::MidiNotesData);

    node->setComponentID(std::to_string(nodeId));
    node->nodeValueTree = nodeValueTree;
    node->midiNoteData  = midiNotes.getChildWithName(ValueTreeIdentifiers::MidiNoteData);
    node->setDisplayMode(NodeDisplayMode::Pitch);

    node->onSelected = [this](Node* n, bool sel) {
        if (applicationContext.onNodeSelected) {
            applicationContext.onNodeSelected(n, sel);
        }
    };

    addAndMakeVisible(node.get());
    node->setInterceptsMouseClicks(!paintMode, !paintMode);
    Node* raw = node.release();
    nodeMap[nodeId] = raw;

    setNodePosition(nodeId);

    return raw;
}

void NodeCanvas::addNodeToCanvas(int nodeId)
{
    juce::ValueTree nodeChildTree = ValueTreeState::getNode(nodeId);
    juce::ValueTree nodeParentTree = ValueTreeState::getNodeParent(nodeId);

    jassert(nodeChildTree.isValid());

    Node* childNode = instantiateNodeFromTree(nodeChildTree);

    if (nodeParentTree.isValid()) {
        int parentNodeId = nodeParentTree.getProperty(ValueTreeIdentifiers::Id);
        auto parentNodePair = nodeMap.find(parentNodeId);

        if (parentNodePair != nodeMap.end()) {
            Node* parentNode = parentNodePair->second;

            Node* startNode = parentNode;
            Node* endNode = childNode;

            if (nodeChildTree.getType() == ValueTreeIdentifiers::AlternativeNodeData) {
                startNode = childNode;
                endNode   = parentNode;
            }

            endNode->nodeColour = startNode->nodeColour;
            addLinePoints(startNode, endNode);
            updateLinePoints(endNode);
        }
    }

    if (!gridOriginSet && nodeChildTree.getType() == ValueTreeIdentifiers::RootNodeData) {
        NodePosition pos = ValueTreeState::getNodePosition(nodeId);
        gridOrigin    = { (float)pos.xPosition, (float)pos.yPosition };
        gridSpacing   = 50.0f;
        gridOriginSet = true;
    }

    applicationContext.rtGraphBuilder->makeRTGraph(nodeChildTree);
}

void NodeCanvas::removeNodeFromCanvas(int nodeId)
{
    auto nodePair = nodeMap.find(nodeId);
    if (nodePair == nodeMap.end()) {
        return;
    }

    Node* node = nodePair->second;
    removeLinePoints(node);
    removeChildComponent(node);
    delete node;
    nodeMap.erase(nodeId);
}

void NodeCanvas::setNodePosition(int nodeId)
{
    auto nodePair = nodeMap.find(nodeId);
    if (nodePair == nodeMap.end()) {
        return;
    }

    juce::ValueTree nodeValueTree = ValueTreeState::getNode(nodeId);
    if (!nodeValueTree.isValid()) {
        return;
    }

    NodePosition nodePosition = ValueTreeState::getNodePosition(nodeId);

    int xPosition = nodePosition.xPosition;
    int yPosition = nodePosition.yPosition;
    int radius    = nodePosition.radius;

    auto node = nodePair->second;

    if (node->nodeType == NodeType::Root) {
        const int rw = RootNode::loopLimitRectangleWidth;
        node->setSize(radius * 2 + rw, radius * 2);
        node->setTopLeftPosition(xPosition - radius - rw, yPosition - radius);
    }
    else {
        node->setSize(radius * 2, radius * 2);
        node->setCentrePosition(xPosition, yPosition);
    }

    updateLinePoints(node);

}

void NodeCanvas::moveDescendants(juce::ValueTree nodeValueTree, int deltaX, int deltaY)
{
    juce::ValueTree nodeValueTreeChildren = nodeValueTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

    for (int i = 0; i < nodeValueTreeChildren.getNumChildren(); i++) {
        juce::ValueTree childIdTree = nodeValueTreeChildren.getChild(i);
        int childId = childIdTree.getProperty(ValueTreeIdentifiers::Id);
        juce::ValueTree childNodeTree = ValueTreeState::getNode(childId);

        NodePosition childPosition = ValueTreeState::getNodePosition(childId);
        childPosition.xPosition += deltaX;
        childPosition.yPosition += deltaY;

        ValueTreeState::setNodePosition(childNodeTree, childPosition, applicationContext.undoManager);
        moveDescendants(childNodeTree, deltaX, deltaY);
    }
}

void NodeCanvas::addLinePoints(Node* parentNode, Node* childNode)
{

    int parentNodeId = parentNode->getComponentID().getIntValue();
    int childNodeId  = childNode->getComponentID().getIntValue();

    juce::ValueTree parentMidiNotesData = ValueTreeState::getMidiNotes(parentNodeId);
    juce::ValueTree parentMidiNoteData  = parentMidiNotesData.getChildWithName(ValueTreeIdentifiers::MidiNoteData);

    auto arrow = std::make_unique<NodeArrow>(parentNode, childNode, applicationContext);

    parentNode->nodeArrows[childNodeId] = arrow.get();
    addAndMakeVisible(arrow.get());

    arrow->toBack();
    arrow->setInterceptsMouseClicks(false,false);

    if (parentMidiNoteData.isValid() && childNode->nodeType != NodeType::Root) {
        arrow->bindToProperty(parentMidiNoteData, ValueTreeIdentifiers::MidiDuration);
    }

    nodeArrows.add(arrow.release());
}

void NodeCanvas::removeLinePoints(Node* node)
{
    for (int i = nodeArrows.size() - 1; i >= 0; i--)
    {
        NodeArrow* nodeArrow = nodeArrows[i];
        if (nodeArrow->parentNode != node && nodeArrow->childNode != node) {
            continue;
        }

        const int childNodeId = nodeArrow->childNode->getComponentID().getIntValue();
        nodeArrow->parentNode->nodeArrows.erase(childNodeId);

        nodeArrows.remove(i);
    }
}

void NodeCanvas::updateLinePoints(Node* movedNode)
{
    for (NodeArrow* arrow : nodeArrows)
    {
        Node* parentNode = arrow->parentNode;
        Node* childNode  = arrow->childNode;
        NodeDisplayMode mode = movedNode->mode;

        if (parentNode != movedNode && childNode != movedNode) {
            continue;
        }

        parentNode->nodeTextEditor->formatDisplay(mode);
        childNode->nodeTextEditor->formatDisplay(mode);
        arrow->setArrowBounds(movedNode);
    }
}

// processor-related Functions //

void NodeCanvas::setProcessorPlayblack(bool isPlaying)
{
    start = isPlaying;
    applicationContext.processor->isPlaying.store(start);

    for(auto& [graphID,graph] : applicationContext.rtGraphBuilder->rtGraphs) {
        graph.get()->traversalRequested = start;
        applicationContext.processor->setNewGraph(graph);
    }

    if (! isPlaying) {
        resetAllArrowProgress();
    }
}

void NodeCanvas::setSelectionMode(NodeDisplayMode mode) const {

    for (auto& [nodeId, node] : nodeMap)
    {
        node->setDisplayMode(mode);
    }
}

void NodeCanvas::triggerArrowSnapForNode(int nodeId)
{
    auto nodePair = nodeMap.find(nodeId);
    if (nodePair == nodeMap.end()) {
        return;
    }
    Node* node = nodePair->second;

    for (NodeArrow* arrow : nodeArrows)
    {
        if (arrow->childNode == node) {
            arrow->triggerSnapAnimation();
            return;
        }
    }
}

void NodeCanvas::showSnapGhostArrow(Node* from, Node* to)
{
    if (snapGhostArrow != nullptr) {
        if (snapGhostArrow->parentNode == from && snapGhostArrow->childNode == to) {
            return;
        }
        removeChildComponent(snapGhostArrow);
        delete snapGhostArrow;
        snapGhostArrow = nullptr;
    }

    snapGhostArrow = new NodeArrow(from, to, applicationContext);
    snapGhostArrow->isGhost = true;
    snapGhostArrow->setInterceptsMouseClicks(false, false);
    addAndMakeVisible(snapGhostArrow);
    snapGhostArrow->toBack();
    snapGhostArrow->setArrowBounds(from);
    snapGhostArrow->triggerSnapAnimation();
}

void NodeCanvas::hideSnapGhostArrow()
{
    if (snapGhostArrow != nullptr) {
        removeChildComponent(snapGhostArrow);
        delete snapGhostArrow;
        snapGhostArrow = nullptr;
    }
}

void NodeCanvas::clearCanvas()
{
    hideSnapGhostArrow();
    nodeArrows.clear();

    for (auto& [nodeId, node] : nodeMap) {
        removeChildComponent(node);
        delete node;
    }
    nodeMap.clear();
    gridOriginSet = false;
    showGrid = false;
}

void NodeCanvas::setValueTreeState(const juce::ValueTree& stateTree)
{
    asyncUpdates.clear();
    cancelPendingUpdate();

    clearCanvas();

    std::unordered_map<int,juce::ValueTree> rootNodeMap;

    struct NodePair {
        int parentNodeId;
        int childNodeId;
    };

    std::vector<NodePair> nodePairs;

    if (stateTree.getNumChildren() == 0) { DBG("stateTree is empty"); }

    for (int i = 0 ; i < stateTree.getNumChildren(); i++) {

        juce::ValueTree nodeValueTree = stateTree.getChild(i);
        int nodeId = nodeValueTree.getProperty(ValueTreeIdentifiers::Id);

        if (nodeValueTree.getType() == ValueTreeIdentifiers::RootNodeData) {
            rootNodeMap[nodeId] = nodeValueTree;
        }

        juce::ValueTree nodeValueTreeChildren = nodeValueTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);
        for (int j = 0; j < nodeValueTreeChildren.getNumChildren(); j++) {
            int childId = nodeValueTreeChildren.getChild(j).getProperty(ValueTreeIdentifiers::Id);
            nodePairs.push_back({ nodeId, childId });
        }

        instantiateNodeFromTree(nodeValueTree);
    }

    for (auto [parentNodeId,childNodeId] : nodePairs) {

        auto parentNodePair = nodeMap.find(parentNodeId);
        auto childNodePair = nodeMap.find(childNodeId);

        jassert(parentNodePair != nodeMap.end());
        jassert(childNodePair != nodeMap.end());

        Node* parentNode = parentNodePair->second;
        Node* childNode = childNodePair->second;

        Node* startNode = parentNode;
        Node* endNode   = childNode;

        if (childNode->nodeValueTree.getType() == ValueTreeIdentifiers::AlternativeNodeData) {
            startNode = childNode;
            endNode   = parentNode;
        }

        endNode->nodeColour = startNode->nodeColour;
        addLinePoints(startNode, endNode);
        updateLinePoints(endNode);
    }

    if (!gridOriginSet && !rootNodeMap.empty()) {
        auto it = rootNodeMap.begin();
        int firstRootId = it->first;
        NodePosition pos = ValueTreeState::getNodePosition(firstRootId);
        gridOrigin    = { (float)pos.xPosition, (float)pos.yPosition };
        gridSpacing   = 50.0f;
        gridOriginSet = true;
    }

    for (auto& [id, rootNodeValueTree] : rootNodeMap) {
        if (rootNodeValueTree.isValid()) {
            applicationContext.rtGraphBuilder->makeRTGraph(rootNodeValueTree);
        }
    }

}

void NodeCanvas::setPaintMode(bool paintMode) {

    this->paintMode = paintMode;

    for (auto& [id, node] : nodeMap) {
        if (node != nullptr) {
            node->setInterceptsMouseClicks(!paintMode, !paintMode);
        }
    }

    if (paintMode) {
        updateBrushCursor();
        refreshValueField();
    }
    else {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void NodeCanvas::renderValueField() {

    if (getWidth() <= 0 || getHeight() <= 0) {
        return;
    }

    const juce::Identifier valueId = paintLayerValueId();

    const int   kFieldScale = 2;
    const float glowRadius   = 110.0f;

    const int fieldW = juce::jmax(1, getWidth()  / kFieldScale);
    const int fieldH = juce::jmax(1, getHeight() / kFieldScale);

    const float fieldRadius  = glowRadius / (float) kFieldScale;
    const float fieldRadius2 = fieldRadius * fieldRadius;

    const size_t numCells = (size_t) fieldW * (size_t) fieldH;
    fieldWeightedSum.assign(numCells, 0.0f);
    fieldTotalWeight.assign(numCells, 0.0f);
    fieldCoverageProd.assign(numCells, 1.0f);
    float* weightedSum  = fieldWeightedSum.data();
    float* totalWeight  = fieldTotalWeight.data();
    float* coverageProd = fieldCoverageProd.data();

    for (auto& [id, node] : nodeMap) {
        if (node == nullptr) {
            continue;
        }

        juce::ValueTree notes = ValueTreeState::getMidiNotes(id);
        juce::ValueTree note  = notes.getChild(0);

        if (! note.isValid()) {
            continue;
        }

        const int   value  = (int) note.getProperty(valueId);
        const float factor = juce::jlimit(0.0f, 1.0f, value / 127.0f);

        const auto  centre = node->getBounds().getCentre().toFloat();
        const float cx = centre.x / (float) kFieldScale;
        const float cy = centre.y / (float) kFieldScale;

        const int x0 = juce::jmax(0,          (int) std::floor(cx - fieldRadius));
        const int x1 = juce::jmin(fieldW - 1, (int) std::ceil (cx + fieldRadius));
        const int y0 = juce::jmax(0,          (int) std::floor(cy - fieldRadius));
        const int y1 = juce::jmin(fieldH - 1, (int) std::ceil (cy + fieldRadius));

        for (int y = y0; y <= y1; ++y) {
            const float dy  = (float) y - cy;
            const float dy2 = dy * dy;
            float* wsRow = weightedSum  + (size_t) y * fieldW;
            float* twRow = totalWeight  + (size_t) y * fieldW;
            float* cpRow = coverageProd + (size_t) y * fieldW;

            for (int x = x0; x <= x1; ++x) {
                const float dx = (float) x - cx;
                const float d2 = dx * dx + dy2;

                if (d2 >= fieldRadius2) {
                    continue;
                }

                const float t = 1.0f - std::sqrt(d2) / fieldRadius;
                const float w = t * t;

                wsRow[x] += w * factor;
                twRow[x] += w;
                cpRow[x] *= (1.0f - w);
            }
        }
    }

    if (valueField.getWidth() != fieldW || valueField.getHeight() != fieldH) {
        valueField = juce::Image(juce::Image::ARGB, fieldW, fieldH, false);
    }

    juce::Image::BitmapData pixels(valueField, juce::Image::BitmapData::writeOnly);
    const juce::Colour bg = CustomLookAndFeel::get(*this).canvasColour.brighter();

    for (int y = 0; y < fieldH; ++y) {
        const float*  wsRow = weightedSum  + (size_t) y * fieldW;
        const float*  twRow = totalWeight  + (size_t) y * fieldW;
        const float*  cpRow = coverageProd + (size_t) y * fieldW;
        juce::uint8*  line  = pixels.getLinePointer(y);

        for (int x = 0; x < fieldW; ++x) {
            juce::Colour out = bg;

            const float tw = twRow[x];
            if (tw > 0.0f) {
                const float fieldFactor = wsRow[x] / tw;
                const float coverage    = juce::jlimit(0.0f, 1.0f, 1.0f - cpRow[x]);
                out = bg.interpolatedWith(mapFieldColour(fieldFactor), coverage);
            }

            auto* px = (juce::PixelARGB*) (line + x * pixels.pixelStride);
            px->setARGB(out.getAlpha(), out.getRed(), out.getGreen(), out.getBlue());
        }
    }
}

void NodeCanvas::refreshValueField() {

    if (! paintMode) {
        return;
    }

    renderValueField();
    repaint();
}

void NodeCanvas::setViewZoom(float z) {
    viewZoom = z;
    updateBrushCursor();
}

void NodeCanvas::updateBrushCursor() {

    if (!paintMode) {
        return;
    }

    const int size    = juce::jmax(1, (int)(brushRadius * viewZoom * 2.0f));
    const int hotspot = size / 2;

    juce::Image img(juce::Image::ARGB, size, size, true);
    juce::Graphics g(img);
    g.setColour(juce::Colours::black);
    g.drawEllipse(img.getBounds().toFloat().reduced(1.0f), 1.0f);

    setMouseCursor(juce::MouseCursor(img, hotspot, hotspot));
}

void NodeCanvas::setBrushColour(juce::Colour colour) {
    brushColour = colour;
}

void NodeCanvas::setBrushRadius(float radius) {
    brushRadius = radius;
    updateBrushCursor();
}

void NodeCanvas::setActivePaintLayer(int index) {
    activePaintLayer = juce::jlimit(0, numPaintLayers - 1, index);

    if (paintMode) {
        renderValueField();
    }
    repaint();
}

void NodeCanvas::ensurePaintBuffers() {

    const size_t expected = (size_t) getWidth() * (size_t) getHeight();

    if (paintDensity[activePaintLayer].size() != expected) {
        paintDensity[activePaintLayer].assign(expected, 0.0f);
    }

    if (strokeMask.size() != expected) {
        strokeMask.assign(expected, 0.0f);
    }
}

void NodeCanvas::seedStrokeDensityFromNodes() {

    const int w = getWidth();
    const int h = getHeight();

    if (w <= 0 || h <= 0) {
        return;
    }

    const juce::Identifier valueId = paintLayerValueId();
    std::vector<float>& density = paintDensity[activePaintLayer];

    for (auto& [id, node] : nodeMap) {
        if (node == nullptr) {
            continue;
        }

        juce::ValueTree notes = ValueTreeState::getMidiNotes(id);
        juce::ValueTree note  = notes.getChild(0);

        if (! note.isValid()) {
            continue;
        }

        const int   value = (int) note.getProperty(valueId);
        const float seed  = juce::jlimit(0.0f, 1.0f, value / 127.0f);

        const auto  centre = node->getNodeCentre().toFloat();
        const float nodeR  = node->getBounds().getHeight() * 0.5f;
        const float nodeR2 = nodeR * nodeR;

        const int x0 = juce::jmax(0,     (int) std::floor(centre.x - nodeR));
        const int x1 = juce::jmin(w - 1, (int) std::ceil (centre.x + nodeR));
        const int y0 = juce::jmax(0,     (int) std::floor(centre.y - nodeR));
        const int y1 = juce::jmin(h - 1, (int) std::ceil (centre.y + nodeR));

        for (int y = y0; y <= y1; ++y) {
            const float ddy = (float) y - centre.y;
            for (int x = x0; x <= x1; ++x) {
                const float ddx = (float) x - centre.x;
                if (ddx * ddx + ddy * ddy > nodeR2) {
                    continue;
                }
                density[(size_t) y * (size_t) w + (size_t) x] = seed;
            }
        }
    }
}

void NodeCanvas::accumulateStroke(juce::Point<float> from, juce::Point<float> to, bool rearm) {

    const int w = getWidth();
    const int h = getHeight();

    if (w <= 0 || h <= 0) {
        return;
    }

    ensurePaintBuffers();

    const float r = brushRadius;
    if (r <= 0.0f) {
        return;
    }

    const int x0 = juce::jmax(0,     (int) std::floor(juce::jmin(from.x, to.x) - r - 1.0f));
    const int x1 = juce::jmin(w - 1, (int) std::ceil (juce::jmax(from.x, to.x) + r + 1.0f));
    const int y0 = juce::jmax(0,     (int) std::floor(juce::jmin(from.y, to.y) - r - 1.0f));
    const int y1 = juce::jmin(h - 1, (int) std::ceil (juce::jmax(from.y, to.y) + r + 1.0f));

    if (x1 < x0 || y1 < y0) {
        return;
    }

    const float dx = to.x - from.x;
    const float dy = to.y - from.y;
    const float segLen2 = dx * dx + dy * dy;

    std::vector<float>& density = paintDensity[activePaintLayer];

    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {

            const float px = (float) x - from.x;
            const float py = (float) y - from.y;

            float t = segLen2 > 0.0f ? (px * dx + py * dy) / segLen2 : 0.0f;
            t = juce::jlimit(0.0f, 1.0f, t);

            const float ox = px - t * dx;
            const float oy = py - t * dy;
            const float dist = std::sqrt(ox * ox + oy * oy);

            if (dist >= r) {
                continue;
            }

            const float falloff  = 1.0f - dist / r;
            const float coverage = falloff * falloff;

            const size_t index = (size_t) y * (size_t) w + (size_t) x;

            if (rearm) {
                strokeMask[index] *= (1.0f - dwellRearm);
            }

            if (coverage <= strokeMask[index]) {
                continue;
            }

            const float delta = brushFlow * (coverage - strokeMask[index]);
            strokeMask[index] = coverage;

            density[index] = juce::jlimit(0.0f, 1.0f,
                                          brushErase ? density[index] - delta
                                                     : density[index] + delta);
        }
    }
}

void NodeCanvas::paintStroke(juce::Point<float> canvasPos, bool isStart, bool erase) {

    if (isStart) {
        brushStrokeActive = true;
        brushErase = erase;
        ensurePaintBuffers();
        std::fill(strokeMask.begin(), strokeMask.end(), 0.0f);
        seedStrokeDensityFromNodes();
        strokePrevPoint = canvasPos;
        startTimerHz(dwellTimerHz);
    }

    brushCurrentPoint = canvasPos;
    accumulateStroke(strokePrevPoint, canvasPos);
    applyPaintToNodes(strokePrevPoint, canvasPos);
    strokePrevPoint = canvasPos;
}

void NodeCanvas::timerCallback() {

    if (!brushStrokeActive) {
        stopTimer();
        return;
    }

    accumulateStroke(brushCurrentPoint, brushCurrentPoint, true);
    applyPaintToNodes(brushCurrentPoint, brushCurrentPoint);
}

juce::Colour NodeCanvas::mapFieldColour(float factor) const {
    return brushColour.withMultipliedBrightness(juce::jlimit(0.0f, 1.0f, factor));
}

juce::Identifier NodeCanvas::paintLayerValueId() const {

    if (activePaintLayer == 1) {
        return ValueTreeIdentifiers::MidiDuration;
    }
    if (activePaintLayer == 2) {
        return ValueTreeIdentifiers::MidiVelocity;
    }
    return ValueTreeIdentifiers::MidiPitch;
}

void NodeCanvas::applyPaintToNodes(juce::Point<float> from, juce::Point<float> to) {

    const int w = getWidth();
    const int h = getHeight();

    if (w <= 0 || h <= 0 || nodeMap.empty()) {
        return;
    }

    const std::vector<float>& density = paintDensity[activePaintLayer];
    if (density.size() != (size_t) w * (size_t) h) {
        return;
    }

    const juce::Identifier valueId = paintLayerValueId();

    const float r       = brushRadius;
    const float dx      = to.x - from.x;
    const float dy      = to.y - from.y;
    const float segLen2 = dx * dx + dy * dy;

    for (auto& [id, node] : nodeMap) {
        if (node == nullptr) {
            continue;
        }

        const auto  centre = node->getNodeCentre().toFloat();
        const float nodeR  = node->getBounds().getHeight() * 0.5f;

        const float px = centre.x - from.x;
        const float py = centre.y - from.y;

        float t = segLen2 > 0.0f ? (px * dx + py * dy) / segLen2 : 0.0f;
        t = juce::jlimit(0.0f, 1.0f, t);

        const float ox = px - t * dx;
        const float oy = py - t * dy;

        const float reach = r + nodeR;
        if (ox * ox + oy * oy > reach * reach) {
            continue;
        }

        const float nodeR2 = nodeR * nodeR;

        const int x0 = juce::jmax(0,     (int) std::floor(centre.x - nodeR));
        const int x1 = juce::jmin(w - 1, (int) std::ceil (centre.x + nodeR));
        const int y0 = juce::jmax(0,     (int) std::floor(centre.y - nodeR));
        const int y1 = juce::jmin(h - 1, (int) std::ceil (centre.y + nodeR));

        float sample = brushErase ? 1.0f : 0.0f;
        bool  found  = false;

        for (int y = y0; y <= y1; ++y) {
            const float ddy = (float) y - centre.y;
            for (int x = x0; x <= x1; ++x) {
                const float ddx = (float) x - centre.x;
                if (ddx * ddx + ddy * ddy > nodeR2) {
                    continue;
                }
                const float dv = density[(size_t) y * (size_t) w + (size_t) x];
                sample = brushErase ? juce::jmin(sample, dv) : juce::jmax(sample, dv);
                found  = true;
            }
        }

        if (! found) {
            continue;
        }

        const int value = juce::jlimit(0, 127, (int) std::round(sample * 127.0f));

        juce::ValueTree notes = ValueTreeState::getMidiNotes(id);
        juce::ValueTree note  = notes.getChild(0);

        if (! note.isValid()) {
            continue;
        }

        if ((int) note.getProperty(valueId) != value) {
            note.setProperty(valueId, value, nullptr);
        }
    }
}

void NodeCanvas::endStroke() {

    if (!brushStrokeActive) {
        return;
    }

    brushStrokeActive = false;
    stopTimer();
}

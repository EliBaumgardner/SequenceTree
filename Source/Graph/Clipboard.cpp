#include "Clipboard.h"
#include "NodeFactory.h"
#include "TypedProperties.h"
#include "ValueTreeIdentifiers.h"
#include <ArsNovaUI/Canvas/CanvasIdentifiers.h>
#include "GraphState.h"

#include <algorithm>

void Clipboard::copy(std::span<const int> selectedIds)
{
    const std::vector<int> ids = withEncapsulatedMembers(selectedIds);

    juce::ValueTree copied { ValueTreeIdentifiers::SelectionClipboard };

    for (const int nodeId : ids) {
        const juce::ValueTree node = graphState.getNode(nodeId);

        if (! node.isValid() || node.getType() == ValueTreeIdentifiers::EncapsulatorData) {
            continue;
        }

        juce::ValueTree copiedNode = node.createCopy();

        NodeProperties copiedProperties(copiedNode);

        copiedProperties.encapsulatorId.resetToDefault(nullptr);

        copied.addChild(copiedNode, -1, nullptr);
    }

    for (const juce::ValueTree& encapsulator : encapsulatorsCovering(ids)) {
        copied.addChild(encapsulator.createCopy(), -1, nullptr);
    }

    contents = copied;

    copiedChordMemberIds      .clear();
    copiedIdsWithParentOutside.clear();
    copiedRootTraversals      .clear();

    for (const juce::ValueTree& node : clipboardNodes()) {
        const NodeProperties properties(node);

        const int nodeId     = node.getProperty(ArsNovaUI::CanvasIdentifiers::Id);
        const int rootNodeId = properties.rootNodeId;

        if (wasChordMember(node)) {
            copiedChordMemberIds.insert(nodeId);
        }

        if (hasParentOutsideCopy(nodeId)) {
            copiedIdsWithParentOutside.insert(nodeId);
        }

        const juce::ValueTree rootTraversals =
            graphState.getNode(rootNodeId).getChildWithName(ValueTreeIdentifiers::TraversalChildrenIds);

        if (rootTraversals.isValid()) {
            copiedRootTraversals[rootNodeId] = rootTraversals.createCopy();
        }
    }
}

std::vector<int> Clipboard::pasteAt(juce::Point<int> canvasPoint)
{
    undoManager.beginNewTransaction();

    const PasteLayout      layout = buildPasteLayout();
    const juce::Point<int> offset = canvasPoint - pastedCentre(layout);

    insertClipboardNodes(layout, offset);
    connectClipboardNodes(layout);
    restoreDanglingArrows(layout);

    std::vector<int> pastedIds = createPastedEncapsulators(layout);

    for (const auto& [originalId, newId] : layout.idMap) {
        pastedIds.push_back(newId);
    }

    return pastedIds;
}

void Clipboard::insertClipboardNodes(const PasteLayout& layout, juce::Point<int> offset) const
{
    for (const juce::ValueTree& source : pastedSources(layout)) {
        const int originalId   = source.getProperty(ArsNovaUI::CanvasIdentifiers::Id);
        const int newId        = layout.idMap.at(originalId);
        const int pastedRootId = layout.rootIdOf.at(originalId);

        juce::ValueTree node = buildPastedNode(source, layout.promotedToRoot.count(originalId) > 0);

        NodeProperties properties(node);

        node.setProperty(ArsNovaUI::CanvasIdentifiers::Id, newId, nullptr);

        properties.rootNodeId.setValue(pastedRootId, nullptr);

        const juce::Point<int> copiedPosition { node.getProperty(ArsNovaUI::CanvasIdentifiers::XPosition), node.getProperty(ArsNovaUI::CanvasIdentifiers::YPosition) };
        const juce::Point<int> pastedPosition = copiedPosition + offset;

        node.setProperty(ArsNovaUI::CanvasIdentifiers::XPosition, pastedPosition.x, nullptr);
        node.setProperty(ArsNovaUI::CanvasIdentifiers::YPosition, pastedPosition.y, nullptr);

        node.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds).removeAllChildren(nullptr);

        const juce::ValueTree danglingArrows = node.getChildWithName(ArsNovaUI::CanvasIdentifiers::DanglingArrows);
        if (danglingArrows.isValid()) {
            node.removeChild(danglingArrows, nullptr);
        }

        graphState.nodeMap.addChild(node, -1, &undoManager);
    }
}

void Clipboard::connectClipboardNodes(const PasteLayout& layout) const
{
    for (const juce::ValueTree& source : pastedSources(layout)) {
        const int sourceId = source.getProperty(ArsNovaUI::CanvasIdentifiers::Id);
        const int parentId = layout.idMap.at(sourceId);

        const juce::ValueTree childrenIds = source.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds);

        for (int child = 0; child < childrenIds.getNumChildren(); ++child) {
            const juce::ValueTree childId = childrenIds.getChild(child);
            const int  originalChildId    = childId.getProperty(ArsNovaUI::CanvasIdentifiers::Id);
            const auto copiedChild        = layout.idMap.find(originalChildId);

            if (copiedChild != layout.idMap.end()) {
                graphState.connectNodes(parentId, copiedChild->second, &undoManager);

                ArrowBindingOps::setArrowInfo(graphState.getConnection(parentId, copiedChild->second), ArrowBindingOps::getArrowInfo(childId), &undoManager);
            }
        }
    }
}

void Clipboard::restoreDanglingArrows(const PasteLayout& layout) const
{
    for (const juce::ValueTree& source : pastedSources(layout)) {
        const juce::ValueTree danglingArrows = source.getChildWithName(ArsNovaUI::CanvasIdentifiers::DanglingArrows);
        if (!danglingArrows.isValid()) {
            continue;
        }

        const int sourceId = source.getProperty(ArsNovaUI::CanvasIdentifiers::Id);
        const int pastedId = layout.idMap.at(sourceId);

        juce::ValueTree node = graphState.getNode(pastedId);
        if (node.isValid()) {
            node.addChild(danglingArrows.createCopy(), -1, &undoManager);
        }
    }
}

void Clipboard::addRootTraversals(juce::ValueTree node, int originalRootId) const
{
    juce::ValueTree traversals = node.getChildWithName(ValueTreeIdentifiers::TraversalChildrenIds);

    if (!traversals.isValid()) {
        traversals = juce::ValueTree(ValueTreeIdentifiers::TraversalChildrenIds);
        node.addChild(traversals, -1, nullptr);
    }

    if (traversals.getNumChildren() > 0) {
        return;
    }

    const auto inheritedTraversals = copiedRootTraversals.find(originalRootId);

    if (inheritedTraversals != copiedRootTraversals.end() && inheritedTraversals->second.getNumChildren() > 0) {
        for (int i = 0; i < inheritedTraversals->second.getNumChildren(); ++i) {
            traversals.addChild(inheritedTraversals->second.getChild(i).createCopy(), -1, nullptr);
        }
        return;
    }

    graphState.traversals.addTraversalData(TraversalState::defaultTraversalId, &undoManager);

    juce::ValueTree traversalId { ValueTreeIdentifiers::TraversalId };

    TraversalReferenceProperties reference(traversalId);

    reference.traversalId.setValue(TraversalState::defaultTraversalId, nullptr);

    traversals.addChild(traversalId, -1, nullptr);
}

bool Clipboard::wasChordMember(const juce::ValueTree& source) const
{
    if (source.getType() != ValueTreeIdentifiers::NodeData) {
        return false;
    }

    const juce::ValueTree parent = graphState.getNodeParent(source.getProperty(ArsNovaUI::CanvasIdentifiers::Id));

    if (!parent.isValid()) {
        return false;
    }

    const int sourceX  = source.getProperty(ArsNovaUI::CanvasIdentifiers::XPosition);
    const int sourceY  = source.getProperty(ArsNovaUI::CanvasIdentifiers::YPosition);
    const int parentX  = parent.getProperty(ArsNovaUI::CanvasIdentifiers::XPosition);
    const int parentY  = parent.getProperty(ArsNovaUI::CanvasIdentifiers::YPosition);
    const int sourceId = source.getProperty(ArsNovaUI::CanvasIdentifiers::Id);
    const int parentId = parent.getProperty(ArsNovaUI::CanvasIdentifiers::Id);
    const int deltaX   = sourceX - parentX;
    const int deltaY   = sourceY - parentY;

    const juce::ValueTree connection = graphState.getConnection(parentId, sourceId);

    const ArrowInfo::Settings connectionInfo = ArrowBindingOps::getArrowInfo(connection);

    return ArrowInfo::Settings::durationFromDelta(connectionInfo, deltaX, deltaY) == 0;
}

bool Clipboard::hasParentOutsideCopy(int nodeId) const
{
    const juce::ValueTree parent = graphState.getNodeParent(nodeId);

    return !parent.isValid() || !contents.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, parent.getProperty(ArsNovaUI::CanvasIdentifiers::Id)).isValid();
}

namespace {
    bool isRootNodeType(const juce::ValueTree& node)
    {
        return node.getType() == ValueTreeIdentifiers::RootNodeData
            || node.getType() == ValueTreeIdentifiers::ModulatorRootData;
    }

    bool canBecomeRoot(const juce::ValueTree& node)
    {
        return node.getType() == ValueTreeIdentifiers::NodeData
            || node.getType() == ValueTreeIdentifiers::ModulatorData;
    }

    juce::Identifier rootTypeFor(const juce::ValueTree& node)
    {
        if (node.getType() == ValueTreeIdentifiers::ModulatorData) {
            return ValueTreeIdentifiers::ModulatorRootData;
        }

        return ValueTreeIdentifiers::RootNodeData;
    }
}

int Clipboard::chooseComponentHead(const PasteLayout& layout, const std::set<int>& component) const
{
    int firstPromotable = -1;

    for (const juce::ValueTree& node : pastedSources(layout)) {
        const int nodeId = node.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

        if (component.count(nodeId) == 0 || !canBecomeRoot(node)) {
            continue;
        }

        if (copiedIdsWithParentOutside.count(nodeId) > 0) {
            return nodeId;
        }

        if (firstPromotable < 0) {
            firstPromotable = nodeId;
        }
    }

    return firstPromotable;
}

Clipboard::PasteLayout Clipboard::buildPasteLayout() const
{
    PasteLayout layout;

    layout.parentOf  = mapClipboardParents();
    layout.discarded = findDiscardedOrphans(layout.parentOf);

    for (auto link = layout.parentOf.begin(); link != layout.parentOf.end(); ) {
        const bool linksDiscarded = layout.discarded.count(link->first)  > 0
                                 || layout.discarded.count(link->second) > 0;

        if (linksDiscarded) {
            link = layout.parentOf.erase(link);
        }
        else {
            link = std::next(link);
        }
    }

    layout.promotedToRoot = findOrphansToPromote(layout);

    for (const int head : findRootlessHeads(layout)) {
        layout.promotedToRoot.insert(head);
    }

    layout.idMap          = allocatePastedIds(layout);
    layout.rootIdOf       = resolvePastedRootIds(layout);

    return layout;
}

juce::ValueTree Clipboard::buildPastedNode(const juce::ValueTree& source, bool promoteToRoot) const
{
    juce::Identifier nodeType = source.getType();

    if (promoteToRoot) {
        nodeType = rootTypeFor(source);
    }

    juce::ValueTree node { nodeType };

    node.copyPropertiesFrom(source, nullptr);

    for (int i = 0; i < source.getNumChildren(); ++i) {
        node.addChild(source.getChild(i).createCopy(), -1, nullptr);
    }

    if (promoteToRoot && node.getType() == ValueTreeIdentifiers::RootNodeData) {
        NodeProperties properties(node);

        const NodeProperties sourceProperties(source);

        properties.loopLimit.setValue(GraphState::defaultRootLoopLimit, nullptr);

        addRootTraversals(node, sourceProperties.rootNodeId);
    }

    return node;
}

juce::Point<int> Clipboard::pastedCentre(const PasteLayout& layout) const
{
    juce::Rectangle<int> extent;
    bool                 started = false;

    for (const juce::ValueTree& node : pastedSources(layout)) {
        juce::Rectangle<int> nodeExtent {
            static_cast<int>(node.getProperty(ArsNovaUI::CanvasIdentifiers::XPosition)),
            static_cast<int>(node.getProperty(ArsNovaUI::CanvasIdentifiers::YPosition)),
            1, 1
        };

        if (started) {
            nodeExtent = extent.getUnion(nodeExtent);
        }

        extent  = nodeExtent;
        started = true;
    }

    return extent.getCentre();
}

std::vector<int> Clipboard::withEncapsulatedMembers(std::span<const int> nodeIds) const
{
    std::vector<int> ids;
    std::set<int>    gathered;

    for (const int nodeId : nodeIds) {
        if (gathered.insert(nodeId).second) {
            ids.push_back(nodeId);
        }

        for (const int memberNodeId : graphState.encapsulation.memberIds(nodeId)) {
            if (gathered.insert(memberNodeId).second) {
                ids.push_back(memberNodeId);
            }
        }
    }

    return ids;
}

std::vector<int> Clipboard::createPastedEncapsulators(const PasteLayout& layout) const
{
    std::vector<int> encapsulatorIds;

    for (int i = 0; i < contents.getNumChildren(); ++i) {
        const juce::ValueTree source = contents.getChild(i);

        if (source.getType() != ValueTreeIdentifiers::EncapsulatorData) {
            continue;
        }

        const juce::ValueTree memberIds = source.getChildWithName(ValueTreeIdentifiers::EncapsulatedIds);

        std::vector<int> pastedMemberIds;

        for (int member = 0; member < memberIds.getNumChildren(); ++member) {
            const int  originalMemberId = memberIds.getChild(member).getProperty(ArsNovaUI::CanvasIdentifiers::Id);
            const auto pastedMember     = layout.idMap.find(originalMemberId);

            if (pastedMember != layout.idMap.end()) {
                pastedMemberIds.push_back(pastedMember->second);
            }
        }

        const bool everyMemberPasted = ! pastedMemberIds.empty()
                                    && static_cast<int>(pastedMemberIds.size()) == memberIds.getNumChildren();

        if (! everyMemberPasted) {
            continue;
        }

        const juce::ValueTree encapsulator = NodeFactory::createEncapsulator(graphState, pastedMemberIds, &undoManager);

        encapsulatorIds.push_back(encapsulator.getProperty(ArsNovaUI::CanvasIdentifiers::Id));
    }

    return encapsulatorIds;
}

std::vector<juce::ValueTree> Clipboard::encapsulatorsCovering(std::span<const int> nodeIds) const
{
    const std::set<int> covered { nodeIds.begin(), nodeIds.end() };

    std::set<int>                visitedEncapsulatorIds;
    std::vector<juce::ValueTree> encapsulators;

    for (const int nodeId : nodeIds) {
        const juce::ValueTree node = graphState.getNode(nodeId);

        const NodeProperties properties(node);

        int encapsulatorId = properties.encapsulatorId;

        if (node.getType() == ValueTreeIdentifiers::EncapsulatorData) {
            encapsulatorId = nodeId;
        }

        if (encapsulatorId < 0 || ! visitedEncapsulatorIds.insert(encapsulatorId).second) {
            continue;
        }

        const std::vector<int> memberNodeIds = graphState.encapsulation.memberIds(encapsulatorId);

        bool coversAllMembers = ! memberNodeIds.empty();

        for (const int memberNodeId : memberNodeIds) {
            if (covered.count(memberNodeId) == 0) {
                coversAllMembers = false;
            }
        }

        if (coversAllMembers) {
            encapsulators.push_back(graphState.getNode(encapsulatorId));
        }
    }

    return encapsulators;
}

std::vector<juce::ValueTree> Clipboard::clipboardNodes() const
{
    std::vector<juce::ValueTree> nodes;

    for (int i = 0; i < contents.getNumChildren(); ++i) {
        const juce::ValueTree node = contents.getChild(i);

        if (node.getType() != ValueTreeIdentifiers::EncapsulatorData) {
            nodes.push_back(node);
        }
    }

    return nodes;
}

std::vector<juce::ValueTree> Clipboard::pastedSources(const PasteLayout& layout) const
{
    std::vector<juce::ValueTree> sources;

    for (const juce::ValueTree& node : clipboardNodes()) {
        const int nodeId = node.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

        if (layout.discarded.count(nodeId) == 0) {
            sources.push_back(node);
        }
    }

    return sources;
}

std::map<int,int> Clipboard::mapClipboardParents() const
{
    const std::vector<juce::ValueTree> nodes = clipboardNodes();

    std::set<int> clipboardIds;
    for (const juce::ValueTree& node : nodes) {
        const int nodeId = node.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

        clipboardIds.insert(nodeId);
    }

    std::map<int,int> parentOf;

    for (const juce::ValueTree& parent : nodes) {
        const int parentId = parent.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

        const juce::ValueTree childrenIds = parent.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds);

        for (int child = 0; child < childrenIds.getNumChildren(); ++child) {
            const int childId = childrenIds.getChild(child).getProperty(ArsNovaUI::CanvasIdentifiers::Id);

            if (clipboardIds.count(childId) > 0 && parentOf.count(childId) == 0) {
                parentOf[childId] = parentId;
            }
        }
    }

    return parentOf;
}

std::map<int,int> Clipboard::allocatePastedIds(const PasteLayout& layout) const
{
    std::map<int,int> idMap;

    for (const juce::ValueTree& node : pastedSources(layout)) {
        const int originalId = node.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

        ++graphState.nodeIdIncrement;

        idMap[originalId] = graphState.nodeIdIncrement;
    }

    return idMap;
}

std::map<int,int> Clipboard::resolvePastedRootIds(const PasteLayout& layout) const
{
    std::map<int,int> rootIdOf;

    for (const juce::ValueTree& node : pastedSources(layout)) {
        const int nodeId = node.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

        juce::ValueTree ancestor = node;
        std::set<int>   visited;

        while (true) {
            const int ancestorId = ancestor.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

            if (isRootNodeType(ancestor) || layout.promotedToRoot.count(ancestorId) > 0) {
                rootIdOf[nodeId] = layout.idMap.at(ancestorId);
                break;
            }

            const auto parent = layout.parentOf.find(ancestorId);

            if (parent == layout.parentOf.end() || !visited.insert(ancestorId).second) {
                const NodeProperties ancestorProperties(ancestor);

                rootIdOf[nodeId] = ancestorProperties.rootNodeId;

                break;
            }

            ancestor = contents.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, parent->second);
        }
    }

    return rootIdOf;
}

std::set<int> Clipboard::findDiscardedOrphans(const std::map<int,int>& parentOf) const
{
    const std::vector<juce::ValueTree> nodes = clipboardNodes();

    std::set<int> discarded;

    for (bool foundMore = true; foundMore; ) {
        foundMore = false;

        for (const juce::ValueTree& node : nodes) {
            const int nodeId = node.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

            if (discarded.count(nodeId) > 0 || isRootNodeType(node)) {
                continue;
            }

            const auto parent = parentOf.find(nodeId);
            const bool orphaned = parent == parentOf.end() || discarded.count(parent->second) > 0;

            if (orphaned && (!canBecomeRoot(node) || copiedChordMemberIds.count(nodeId) > 0)) {
                discarded.insert(nodeId);
                foundMore = true;
            }
        }
    }

    return discarded;
}

std::set<int> Clipboard::findOrphansToPromote(const PasteLayout& layout) const
{
    std::set<int> promoted;

    for (const juce::ValueTree& node : clipboardNodes()) {
        const int nodeId = node.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

        if (layout.discarded.count(nodeId) > 0 || isRootNodeType(node)) {
            continue;
        }

        if (layout.parentOf.count(nodeId) == 0) {
            promoted.insert(nodeId);
        }
    }

    return promoted;
}

std::set<int> Clipboard::findRootlessHeads(const PasteLayout& layout) const
{
    std::map<int, std::vector<int>> adjacency;

    for (const auto& [childId, parentId] : layout.parentOf) {
        adjacency[childId].push_back(parentId);
        adjacency[parentId].push_back(childId);
    }

    std::set<int> visited;
    std::set<int> heads;

    for (const juce::ValueTree& node : pastedSources(layout)) {
        const int startId = node.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

        if (!visited.insert(startId).second) {
            continue;
        }

        std::set<int>    component { startId };
        std::vector<int> frontier  { startId };

        while (!frontier.empty()) {
            const int current = frontier.back();
            frontier.pop_back();

            for (const int neighbour : adjacency[current]) {
                if (visited.insert(neighbour).second) {
                    component.insert(neighbour);
                    frontier.push_back(neighbour);
                }
            }
        }

        const bool alreadyRooted = std::ranges::any_of(component,
            [this, &layout](int memberId) {
                return layout.promotedToRoot.count(memberId) > 0
                    || isRootNodeType(contents.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, memberId));
            });

        if (alreadyRooted) {
            continue;
        }

        const int head = chooseComponentHead(layout, component);

        if (head >= 0) {
            heads.insert(head);
        }
    }

    return heads;
}

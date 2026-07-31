#include "SelectionOps.h"
#include "../UI/Canvas/NodeCanvas.h"
#include "../UI/Node/Node.h"
#include "../UI/Node/NodeFactory.h"
#include "../Graph/ValueTreeIdentifiers.h"
#include "../Graph/ValueTreeState.h"
#include "../Graph/ArrowDuration.h"

#include <algorithm>

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
        return node.getType() == ValueTreeIdentifiers::ModulatorData
             ? ValueTreeIdentifiers::ModulatorRootData
             : ValueTreeIdentifiers::RootNodeData;
    }
}

std::vector<int> SelectionOps::selectedNodeIds() const
{
    std::vector<int> ids;

    for (auto& [nodeId, node] : applicationContext.canvas->nodeManager.all()) {
        if (node->isSelected) {
            ids.push_back(nodeId);
        }
    }

    return ids;
}

bool SelectionOps::hasSelection() const
{
    return !selectedNodeIds().empty();
}

void SelectionOps::copySelection()
{
    const std::vector<int> ids = selectedNodeIds();
    if (ids.empty()) {
        return;
    }

    ValueTreeState& state = *applicationContext.valueTreeState;

    juce::ValueTree copied { ValueTreeIdentifiers::SelectionClipboard };

    for (const int nodeId : ids) {
        const juce::ValueTree node = state.getNode(nodeId);

        if (node.isValid()) {
            copied.addChild(node.createCopy(), -1, nullptr);
        }
    }

    clipboard = copied;
}

void SelectionOps::deleteSelection()
{
    const std::vector<int> ids = selectedNodeIds();
    if (ids.empty()) {
        return;
    }

    ValueTreeState& state          = *applicationContext.valueTreeState;
    juce::UndoManager* undoManager = applicationContext.undoManager;

    undoManager->beginNewTransaction();

    for (const int nodeId : ids) {
        const juce::ValueTree node = state.getNode(nodeId);
        if (!node.isValid()) {
            continue;
        }

        const bool ownsNodeTree = isRootNodeType(node);

        state.removeNode(nodeId, undoManager);

        if (ownsNodeTree) {
            state.removeNodeTree(nodeId, undoManager);
        }
    }
}

std::map<int,int> SelectionOps::mapClipboardParents() const
{
    std::set<int> clipboardIds;
    for (int i = 0; i < clipboard.getNumChildren(); ++i) {
        clipboardIds.insert((int) clipboard.getChild(i).getProperty(ValueTreeIdentifiers::Id));
    }

    std::map<int,int> parentOf;

    for (int i = 0; i < clipboard.getNumChildren(); ++i) {
        const juce::ValueTree parent = clipboard.getChild(i);
        const int parentId = parent.getProperty(ValueTreeIdentifiers::Id);

        const juce::ValueTree childrenIds = parent.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

        for (int child = 0; child < childrenIds.getNumChildren(); ++child) {
            const int childId = childrenIds.getChild(child).getProperty(ValueTreeIdentifiers::Id);

            if (clipboardIds.count(childId) > 0 && parentOf.count(childId) == 0) {
                parentOf[childId] = parentId;
            }
        }
    }

    return parentOf;
}

bool SelectionOps::wasChordMember(const juce::ValueTree& source) const
{
    if (source.getType() != ValueTreeIdentifiers::NodeData) {
        return false;
    }

    const juce::ValueTree parent =
        applicationContext.valueTreeState->getNodeParent(source.getProperty(ValueTreeIdentifiers::Id));

    if (!parent.isValid()) {
        return false;
    }

    const int deltaX = (int) source.getProperty(ValueTreeIdentifiers::XPosition)
                     - (int) parent.getProperty(ValueTreeIdentifiers::XPosition);
    const int deltaY = (int) source.getProperty(ValueTreeIdentifiers::YPosition)
                     - (int) parent.getProperty(ValueTreeIdentifiers::YPosition);

    const bool parentIsAlternative = parent.getType() == ValueTreeIdentifiers::AlternativeNodeData;

    return ArrowDuration::fromDelta(deltaX, deltaY, parentIsAlternative) == 0;
}

std::set<int> SelectionOps::findDiscardedOrphans(const std::map<int,int>& parentOf) const
{
    std::set<int> discarded;

    for (bool foundMore = true; foundMore; ) {
        foundMore = false;

        for (int i = 0; i < clipboard.getNumChildren(); ++i) {
            const juce::ValueTree node = clipboard.getChild(i);
            const int nodeId = node.getProperty(ValueTreeIdentifiers::Id);

            if (discarded.count(nodeId) > 0 || isRootNodeType(node)) {
                continue;
            }

            const auto parent = parentOf.find(nodeId);
            const bool orphaned = parent == parentOf.end() || discarded.count(parent->second) > 0;

            if (orphaned && (!canBecomeRoot(node) || wasChordMember(node))) {
                discarded.insert(nodeId);
                foundMore = true;
            }
        }
    }

    return discarded;
}

std::set<int> SelectionOps::findOrphansToPromote(const PasteLayout& layout) const
{
    std::set<int> promoted;

    for (int i = 0; i < clipboard.getNumChildren(); ++i) {
        const juce::ValueTree node = clipboard.getChild(i);
        const int nodeId = node.getProperty(ValueTreeIdentifiers::Id);

        if (layout.discarded.count(nodeId) > 0 || isRootNodeType(node)) {
            continue;
        }

        if (layout.parentOf.count(nodeId) == 0) {
            promoted.insert(nodeId);
        }
    }

    return promoted;
}

bool SelectionOps::isInClipboard(int nodeId) const
{
    return clipboard.getChildWithProperty(ValueTreeIdentifiers::Id, nodeId).isValid();
}

bool SelectionOps::hasParentOutsideCopy(int nodeId) const
{
    const juce::ValueTree parent = applicationContext.valueTreeState->getNodeParent(nodeId);

    return !parent.isValid() || !isInClipboard(parent.getProperty(ValueTreeIdentifiers::Id));
}

int SelectionOps::chooseComponentHead(const PasteLayout& layout, const std::set<int>& component) const
{
    int firstPromotable = -1;

    for (const juce::ValueTree& node : pastedSources(layout)) {
        const int nodeId = node.getProperty(ValueTreeIdentifiers::Id);

        if (component.count(nodeId) == 0 || !canBecomeRoot(node)) {
            continue;
        }

        if (hasParentOutsideCopy(nodeId)) {
            return nodeId;
        }

        if (firstPromotable < 0) {
            firstPromotable = nodeId;
        }
    }

    return firstPromotable;
}

std::set<int> SelectionOps::findRootlessHeads(const PasteLayout& layout) const
{
    std::map<int, std::vector<int>> adjacency;

    for (const auto& [childId, parentId] : layout.parentOf) {
        adjacency[childId].push_back(parentId);
        adjacency[parentId].push_back(childId);
    }

    std::set<int> visited;
    std::set<int> heads;

    for (const juce::ValueTree& node : pastedSources(layout)) {
        const int startId = node.getProperty(ValueTreeIdentifiers::Id);

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

        const bool alreadyRooted = std::any_of(component.begin(), component.end(),
            [this, &layout](int memberId) {
                return layout.promotedToRoot.count(memberId) > 0
                    || isRootNodeType(clipboard.getChildWithProperty(ValueTreeIdentifiers::Id, memberId));
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

std::vector<juce::ValueTree> SelectionOps::pastedSources(const PasteLayout& layout) const
{
    std::vector<juce::ValueTree> sources;

    for (int i = 0; i < clipboard.getNumChildren(); ++i) {
        const juce::ValueTree node = clipboard.getChild(i);

        if (layout.discarded.count((int) node.getProperty(ValueTreeIdentifiers::Id)) == 0) {
            sources.push_back(node);
        }
    }

    return sources;
}

std::map<int,int> SelectionOps::allocatePastedIds(const PasteLayout& layout) const
{
    ValueTreeState& state          = *applicationContext.valueTreeState;
    juce::UndoManager* undoManager = applicationContext.undoManager;

    std::map<int,int> idMap;

    for (const juce::ValueTree& node : pastedSources(layout)) {
        const int originalId = node.getProperty(ValueTreeIdentifiers::Id);

        if (isRootNodeType(node) || layout.promotedToRoot.count(originalId) > 0) {
            juce::ValueTree nodeTree = state.addNodeTree(undoManager);
            const int newId = nodeTree.getProperty(ValueTreeIdentifiers::Id);

            juce::ValueTree rootNodeId { ValueTreeIdentifiers::NodeId };
            rootNodeId.setProperty(ValueTreeIdentifiers::Id, newId, undoManager);

            nodeTree.setProperty(ValueTreeIdentifiers::RootNodeId, newId, undoManager);
            nodeTree.addChild(rootNodeId, -1, undoManager);

            idMap[originalId] = newId;
        }
        else {
            state.setNodeIdIncrement(state.getNodeIdIncrement() + 1);
            idMap[originalId] = state.getNodeIdIncrement();
        }
    }

    return idMap;
}

std::map<int,int> SelectionOps::resolvePastedRootIds(const PasteLayout& layout) const
{
    std::map<int,int> rootIdOf;

    for (const juce::ValueTree& node : pastedSources(layout)) {
        const int nodeId = node.getProperty(ValueTreeIdentifiers::Id);

        juce::ValueTree ancestor = node;
        std::set<int>   visited;

        while (true) {
            const int ancestorId = ancestor.getProperty(ValueTreeIdentifiers::Id);

            if (isRootNodeType(ancestor) || layout.promotedToRoot.count(ancestorId) > 0) {
                rootIdOf[nodeId] = layout.idMap.at(ancestorId);
                break;
            }

            const auto parent = layout.parentOf.find(ancestorId);

            if (parent == layout.parentOf.end() || !visited.insert(ancestorId).second) {
                rootIdOf[nodeId] = ancestor.getProperty(ValueTreeIdentifiers::RootNodeId);
                break;
            }

            ancestor = clipboard.getChildWithProperty(ValueTreeIdentifiers::Id, parent->second);
        }
    }

    return rootIdOf;
}

SelectionOps::PasteLayout SelectionOps::buildPasteLayout() const
{
    PasteLayout layout;

    layout.parentOf  = mapClipboardParents();
    layout.discarded = findDiscardedOrphans(layout.parentOf);

    for (auto link = layout.parentOf.begin(); link != layout.parentOf.end(); ) {
        const bool linksDiscarded = layout.discarded.count(link->first)  > 0
                                 || layout.discarded.count(link->second) > 0;

        link = linksDiscarded ? layout.parentOf.erase(link) : std::next(link);
    }

    layout.promotedToRoot = findOrphansToPromote(layout);

    for (const int head : findRootlessHeads(layout)) {
        layout.promotedToRoot.insert(head);
    }

    layout.idMap          = allocatePastedIds(layout);
    layout.rootIdOf       = resolvePastedRootIds(layout);

    return layout;
}

juce::Point<int> SelectionOps::pastedCentre(const PasteLayout& layout) const
{
    juce::Rectangle<int> extent;
    bool                 started = false;

    for (const juce::ValueTree& node : pastedSources(layout)) {
        const juce::Rectangle<int> nodeExtent {
            (int) node.getProperty(ValueTreeIdentifiers::XPosition),
            (int) node.getProperty(ValueTreeIdentifiers::YPosition),
            1, 1
        };

        extent  = started ? extent.getUnion(nodeExtent) : nodeExtent;
        started = true;
    }

    return extent.getCentre();
}

juce::ValueTree SelectionOps::buildPastedNode(const juce::ValueTree& source, bool promoteToRoot) const
{
    juce::ValueTree node { promoteToRoot ? rootTypeFor(source) : source.getType() };

    node.copyPropertiesFrom(source, nullptr);

    for (int i = 0; i < source.getNumChildren(); ++i) {
        node.addChild(source.getChild(i).createCopy(), -1, nullptr);
    }

    if (promoteToRoot && node.getType() == ValueTreeIdentifiers::RootNodeData) {
        node.setProperty(ValueTreeIdentifiers::LoopLimit, ValueTreeState::defaultRootLoopLimit, nullptr);
        addRootTraversals(node, source.getProperty(ValueTreeIdentifiers::RootNodeId));
    }

    return node;
}

void SelectionOps::addRootTraversals(juce::ValueTree node, int originalRootId) const
{
    ValueTreeState& state = *applicationContext.valueTreeState;

    juce::ValueTree traversals = node.getChildWithName(ValueTreeIdentifiers::TraversalChildrenIds);

    if (!traversals.isValid()) {
        traversals = juce::ValueTree(ValueTreeIdentifiers::TraversalChildrenIds);
        node.addChild(traversals, -1, nullptr);
    }

    if (traversals.getNumChildren() > 0) {
        return;
    }

    const juce::ValueTree originalRoot      = state.getNode(originalRootId);
    const juce::ValueTree inheritedTraversals =
        originalRoot.getChildWithName(ValueTreeIdentifiers::TraversalChildrenIds);

    if (inheritedTraversals.getNumChildren() > 0) {
        for (int i = 0; i < inheritedTraversals.getNumChildren(); ++i) {
            traversals.addChild(inheritedTraversals.getChild(i).createCopy(), -1, nullptr);
        }
        return;
    }

    if (state.traversalMap.getNumChildren() == 0) {
        state.createTraversalData(defaultTraversalId, applicationContext.undoManager);
    }

    juce::ValueTree traversalId { ValueTreeIdentifiers::TraversalId };
    traversalId.setProperty(ValueTreeIdentifiers::TraversalId, defaultTraversalId, nullptr);

    traversals.addChild(traversalId, -1, nullptr);
}

void SelectionOps::insertClipboardNodes(const PasteLayout& layout, juce::Point<int> offset) const
{
    ValueTreeState& state          = *applicationContext.valueTreeState;
    juce::UndoManager* undoManager = applicationContext.undoManager;

    for (const juce::ValueTree& source : pastedSources(layout)) {
        const int originalId = source.getProperty(ValueTreeIdentifiers::Id);
        const int newId      = layout.idMap.at(originalId);

        juce::ValueTree node = buildPastedNode(source, layout.promotedToRoot.count(originalId) > 0);

        node.setProperty(ValueTreeIdentifiers::Id, newId, nullptr);
        node.setProperty(ValueTreeIdentifiers::RootNodeId, layout.rootIdOf.at(originalId), nullptr);

        node.setProperty(ValueTreeIdentifiers::XPosition,
                         (int) node.getProperty(ValueTreeIdentifiers::XPosition) + offset.x, nullptr);
        node.setProperty(ValueTreeIdentifiers::YPosition,
                         (int) node.getProperty(ValueTreeIdentifiers::YPosition) + offset.y, nullptr);

        node.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds).removeAllChildren(nullptr);

        const juce::ValueTree danglingArrows = node.getChildWithName(ValueTreeIdentifiers::DanglingArrows);
        if (danglingArrows.isValid()) {
            node.removeChild(danglingArrows, nullptr);
        }

        state.nodeMap.addChild(node, -1, undoManager);
    }
}

void SelectionOps::connectClipboardNodes(const PasteLayout& layout) const
{
    ValueTreeState& state          = *applicationContext.valueTreeState;
    juce::UndoManager* undoManager = applicationContext.undoManager;

    for (const juce::ValueTree& source : pastedSources(layout)) {
        const int parentId = layout.idMap.at((int) source.getProperty(ValueTreeIdentifiers::Id));

        const juce::ValueTree childrenIds = source.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

        for (int child = 0; child < childrenIds.getNumChildren(); ++child) {
            const int originalChildId = childrenIds.getChild(child).getProperty(ValueTreeIdentifiers::Id);
            const auto copiedChild    = layout.idMap.find(originalChildId);

            if (copiedChild != layout.idMap.end()) {
                state.connectNodes(parentId, copiedChild->second, undoManager);
            }
        }
    }
}

void SelectionOps::restoreDanglingArrows(const PasteLayout& layout) const
{
    ValueTreeState& state          = *applicationContext.valueTreeState;
    juce::UndoManager* undoManager = applicationContext.undoManager;

    for (const juce::ValueTree& source : pastedSources(layout)) {
        const juce::ValueTree danglingArrows = source.getChildWithName(ValueTreeIdentifiers::DanglingArrows);
        if (!danglingArrows.isValid()) {
            continue;
        }

        juce::ValueTree node = state.getNode(layout.idMap.at((int) source.getProperty(ValueTreeIdentifiers::Id)));
        if (node.isValid()) {
            node.addChild(danglingArrows.createCopy(), -1, undoManager);
        }
    }
}

void SelectionOps::selectPastedNodes(const PasteLayout& layout) const
{
    std::vector<int> pastedIds;
    for (const auto& [originalId, newId] : layout.idMap) {
        pastedIds.push_back(newId);
    }

    juce::Component::SafePointer<NodeCanvas> canvas { applicationContext.canvas };

    juce::MessageManager::callAsync([canvas, pastedIds]()
    {
        if (canvas == nullptr) {
            return;
        }

        for (auto& [nodeId, node] : canvas->nodeManager.all()) {
            if (node->isSelected) {
                node->setSelectVisual(false);
            }
        }

        for (const int nodeId : pastedIds) {
            if (Node* const node = canvas->nodeManager.find(nodeId)) {
                node->setSelectVisual(true);
            }
        }
    });
}

void SelectionOps::pasteAt(juce::Point<int> canvasPoint)
{
    if (!hasClipboard()) {
        return;
    }

    applicationContext.undoManager->beginNewTransaction();

    const PasteLayout      layout = buildPasteLayout();
    const juce::Point<int> offset = canvasPoint - pastedCentre(layout);

    insertClipboardNodes(layout, offset);
    connectClipboardNodes(layout);
    restoreDanglingArrows(layout);

    selectPastedNodes(layout);
}

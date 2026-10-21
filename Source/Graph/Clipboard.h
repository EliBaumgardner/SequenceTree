#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include <juce_graphics/juce_graphics.h>

#include <map>
#include <set>
#include <span>
#include <vector>

/*
    The copy-and-paste buffer for nodes. Copying only records trees; pasting is most of the work,
    because a pasted node needs a new id, its arrows must point at the new ids, and each pasted
    node must be decided to be a root or a child. contents holds a copy of each copied node's tree,
    with its properties, notes and arrows, and a copy of every encapsulation group the copied nodes
    belong to.

    copy fills contents from a list of node ids, first adding every member of any group one of the
    ids is in, and records what pasting depends on: which nodes were chord members, which had a
    parent outside the copy, and the traversals each copied tree's root equipped. pasteAt inserts
    the copies as new nodes centred on a canvas point, as one undo transaction. Each pasted node
    gets a new id and the arrows between copied nodes are reconnected to the new ids. A node whose
    parent was not copied becomes a root, or is left out when its type cannot be a root or it was a
    chord member. Each copied group is recreated around its new members, and pasteAt returns the
    new ids.

    SelectionGestures owns it, calls copy with the selected ids and selects the ids pasteAt
    returns. It only reads and writes GraphState, never the canvas.
*/

class GraphState;

class Clipboard
{
public:

    Clipboard(GraphState& graphState, juce::UndoManager& undoManager) : graphState(graphState), undoManager(undoManager) {}

    void copy(std::span<const int> selectedIds);

    std::vector<int> pasteAt(juce::Point<int> canvasPoint);

    juce::ValueTree contents;

private:

    struct PasteLayout;

    void insertClipboardNodes (const PasteLayout& layout, juce::Point<int> offset) const;
    void connectClipboardNodes(const PasteLayout& layout) const;
    void restoreDanglingArrows(const PasteLayout& layout) const;

    void addRootTraversals(juce::ValueTree node, int originalRootId) const;

    bool wasChordMember      (const juce::ValueTree& source) const;
    bool hasParentOutsideCopy(int nodeId) const;

    int chooseComponentHead(const PasteLayout& layout, const std::set<int>& component) const;

    PasteLayout buildPasteLayout() const;

    juce::ValueTree buildPastedNode(const juce::ValueTree& source, bool promoteToRoot) const;

    juce::Point<int> pastedCentre(const PasteLayout& layout) const;

    std::vector<int> withEncapsulatedMembers  (std::span<const int> nodeIds) const;
    std::vector<int> createPastedEncapsulators(const PasteLayout& layout) const;

    std::vector<juce::ValueTree> encapsulatorsCovering(std::span<const int> nodeIds) const;
    std::vector<juce::ValueTree> clipboardNodes       () const;
    std::vector<juce::ValueTree> pastedSources        (const PasteLayout& layout) const;

    std::map<int,int> mapClipboardParents () const;
    std::map<int,int> allocatePastedIds   (const PasteLayout& layout) const;
    std::map<int,int> resolvePastedRootIds(const PasteLayout& layout) const;

    std::set<int> findDiscardedOrphans(const std::map<int,int>& parentOf) const;
    std::set<int> findOrphansToPromote(const PasteLayout& layout) const;
    std::set<int> findRootlessHeads   (const PasteLayout& layout) const;

    struct PasteLayout
    {
        std::map<int,int> parentOf;
        std::set<int>     discarded;
        std::set<int>     promotedToRoot;
        std::map<int,int> idMap;
        std::map<int,int> rootIdOf;
    };

    GraphState&        graphState;
    juce::UndoManager& undoManager;

    std::set<int>                  copiedChordMemberIds;
    std::set<int>                  copiedIdsWithParentOutside;
    std::map<int, juce::ValueTree> copiedRootTraversals;
};

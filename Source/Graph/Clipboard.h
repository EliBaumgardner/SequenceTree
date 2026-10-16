#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include <juce_graphics/juce_graphics.h>

#include <map>
#include <set>
#include <span>
#include <vector>

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

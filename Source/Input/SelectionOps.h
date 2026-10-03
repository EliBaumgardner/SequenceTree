#pragma once

#include "../Util/ApplicationContext.h"

#include <map>
#include <set>
#include <span>
#include <vector>

class Node;

class SelectionOps
{
public:

    explicit SelectionOps(const ApplicationContext& context) : applicationContext(context) {}

    void copySelection   ();
    void deleteSelection ();
    void pasteAt         (juce::Point<int> canvasPoint);
    bool hasClipboard () const { return clipboard.getNumChildren() > 0; }
    void selectAll          () const;
    void clearAll           () const;
    void deselectAllExcept  (const Node& keptNode) const;
    bool hasSelection () const;

private:

    struct PasteLayout
    {
        std::map<int,int> parentOf;
        std::set<int>     discarded;
        std::set<int>     promotedToRoot;
        std::map<int,int> idMap;
        std::map<int,int> rootIdOf;
    };

    std::vector<int> selectionWithEncapsulatedMembers () const;
    std::vector<int> selectedNodeIds                 () const;
    std::vector<juce::ValueTree> encapsulatorsCovering (std::span<const int> nodeIds) const;
    std::vector<juce::ValueTree> clipboardNodes () const;
    bool wasChordMember      (const juce::ValueTree& source) const;
    bool hasParentOutsideCopy(int nodeId) const;
    bool isInClipboard       (int nodeId) const;
    PasteLayout buildPasteLayout () const;
    std::map<int,int> mapClipboardParents   () const;
    std::set<int>     findDiscardedOrphans  (const std::map<int,int>& parentOf) const;
    std::set<int>     findOrphansToPromote  (const PasteLayout& layout) const;
    std::set<int>     findRootlessHeads     (const PasteLayout& layout) const;
    std::vector<juce::ValueTree> pastedSources  (const PasteLayout& layout) const;
    int chooseComponentHead (const PasteLayout& layout, const std::set<int>& component) const;
    std::map<int,int> allocatePastedIds     (const PasteLayout& layout) const;
    std::map<int,int> resolvePastedRootIds  (const PasteLayout& layout) const;
    juce::Point<int> pastedCentre (const PasteLayout& layout) const;
    void insertClipboardNodes  (const PasteLayout& layout, juce::Point<int> offset) const;
    juce::ValueTree buildPastedNode      (const juce::ValueTree& source, bool promoteToRoot) const;
    void            addRootTraversals    (juce::ValueTree node, int originalRootId) const;
    void connectClipboardNodes (const PasteLayout& layout) const;
    void restoreDanglingArrows (const PasteLayout& layout) const;
    void selectPastedNodes (const PasteLayout& layout, std::span<const int> encapsulatorIds) const;
    std::vector<int> createPastedEncapsulators (const PasteLayout& layout) const;

    const ApplicationContext& applicationContext;

    juce::ValueTree clipboard;

    std::set<int>                  copiedChordMemberIds;
    std::set<int>                  copiedIdsWithParentOutside;
    std::map<int, juce::ValueTree> copiedRootTraversals;
};

#pragma once

#include "../Util/ApplicationContext.h"

#include <map>
#include <set>
#include <vector>

class SelectionOps
{
public:

    explicit SelectionOps(ApplicationContext& context) : applicationContext(context) {}

    void copySelection   ();
    void deleteSelection ();
    void pasteAt         (juce::Point<int> canvasPoint);

    bool hasSelection () const;
    bool hasClipboard () const { return clipboard.getNumChildren() > 0; }

private:

    struct PasteLayout
    {
        std::map<int,int> parentOf;
        std::set<int>     discarded;
        std::set<int>     promotedToRoot;
        std::map<int,int> idMap;
        std::map<int,int> rootIdOf;
    };

    std::vector<int> selectedNodeIds () const;

    PasteLayout buildPasteLayout () const;

    std::map<int,int> mapClipboardParents   () const;
    std::set<int>     findDiscardedOrphans  (const std::map<int,int>& parentOf) const;
    std::set<int>     findOrphansToPromote  (const PasteLayout& layout) const;
    std::set<int>     findRootlessHeads     (const PasteLayout& layout) const;
    std::map<int,int> allocatePastedIds     (const PasteLayout& layout) const;
    std::map<int,int> resolvePastedRootIds  (const PasteLayout& layout) const;

    bool wasChordMember      (const juce::ValueTree& source) const;
    bool isInClipboard       (int nodeId) const;
    bool hasParentOutsideCopy(int nodeId) const;

    int chooseComponentHead (const PasteLayout& layout, const std::set<int>& component) const;

    std::vector<juce::ValueTree> pastedSources (const PasteLayout& layout) const;

    juce::Point<int> pastedCentre (const PasteLayout& layout) const;

    juce::ValueTree buildPastedNode      (const juce::ValueTree& source, bool promoteToRoot) const;
    void            addRootTraversals    (juce::ValueTree node, int originalRootId) const;

    void insertClipboardNodes  (const PasteLayout& layout, juce::Point<int> offset) const;
    void connectClipboardNodes (const PasteLayout& layout) const;
    void restoreDanglingArrows (const PasteLayout& layout) const;
    void selectPastedNodes     (const PasteLayout& layout) const;

    ApplicationContext& applicationContext;

    juce::ValueTree clipboard;
};

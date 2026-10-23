#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/*
    The ornament-tool features of the canvas: putting an ornament on a node and taking it off.
    An ornament is attached by setting the node's OrnamentUid property. respondToClick handles a
    click with the ornament tool: on a plain node or root it applies the active ornament, unless
    that ornament is still a draft, and a click anywhere else switches the tool off.
    respondToMarkClick handles a right-click on an ornament's mark: with Shift it removes the
    ornament at once, without Shift it offers a "delete ornament" menu item. NodeController routes
    to it. Edits are made in an undo transaction.
*/

class NodeCanvas;
class OrnamentLibraries;
class OrnamentMarkView;

class OrnamentApplier
{
public:

    OrnamentApplier(NodeCanvas& nodeCanvas, OrnamentLibraries& ornamentLibraries, juce::UndoManager& undoManager);

    void respondToClick    (juce::Component* clickedComponent);
    void respondToMarkClick(const juce::MouseEvent& event, OrnamentMarkView& markView);

private:

    NodeCanvas&        nodeCanvas;
    OrnamentLibraries& ornamentLibraries;
    juce::UndoManager& undoManager;
};

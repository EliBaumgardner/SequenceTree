#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <ArsNovaUI/PopupWindow.h>
#include "../Util/ArrowInfo.h"

/*
    The right-click menu of an arrow on the graph canvas, and the arrow settings it changes. The
    menu offers three items: one opens the allowed-traversals window (AllowedTraversalsMenu) on the
    arrow's tree, one makes an arrow between two trees a traversal arrow or a plain one, and, on
    an arrow into a modulator root, one turns sync on or off. Type and sync are fields of the
    connection's ArrowInfo::Settings, written through ArrowBindingOps in an undo transaction. The
    connection belongs to whichever end lists the other among its children, which
    GraphDocument::ownershipOf answers.

    NodeController owns it and opens it on a plain right-click near an arrow.
*/

class Arrow;
class NodeCanvas;
class GraphState;
class GraphDocument;

class ArrowMenu
{
public:

    ArrowMenu(NodeCanvas& nodeCanvas, GraphState& graphState, GraphDocument& graphDocument, juce::UndoManager& undoManager);

    void showArrowContextMenu(Arrow* arrow);

private:

    void setArrowType(const Arrow* arrow, ArrowInfo::Type arrowType);
    void setArrowSync(const Arrow* arrow, bool shouldSync);

    bool canBeTraversalArrow    (const Arrow* arrow) const;
    bool connectsToModulatorRoot(const Arrow* arrow) const;

    juce::ValueTree connectionTreeFor(const Arrow* arrow) const;

    NodeCanvas&                    nodeCanvas;
    GraphState&                    graphState;
    GraphDocument&                 graphDocument;
    juce::UndoManager&             undoManager;
    ArsNovaUI::PopupWindowLauncher allowedTraversalsLauncher { "Allowed Traversals" };
};

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

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

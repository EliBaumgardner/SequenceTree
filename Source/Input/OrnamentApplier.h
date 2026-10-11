#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

class NodeCanvas;
class OrnamentLibraries;
class OrnamentMarkView;

class OrnamentApplier
{
public:

    OrnamentApplier(NodeCanvas& nodeCanvas, OrnamentLibraries& ornamentLibraries, juce::UndoManager& undoManager);

    void setActive         (bool shouldBeActive);
    void respondToClick    (juce::Component* clickedComponent);
    void respondToMarkClick(const juce::MouseEvent& event, OrnamentMarkView& markView);

    std::function<void(bool)> onActiveChanged;

    bool isActive = false;

private:

    NodeCanvas&        nodeCanvas;
    OrnamentLibraries& ornamentLibraries;
    juce::UndoManager& undoManager;
};

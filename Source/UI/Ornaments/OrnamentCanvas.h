#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "OrnamentNode.h"
#include "../Canvas/EditCanvas.h"

/*
    The EditCanvas for an ornament's figure (see FigureDocument): an OrnamentNode for each figure
    node and an Arrow for each connection and for the dangling arrow, over lines at the grid
    spacing and a line at zero. Its grid is always shown with its origin at zero, so positions
    snap to the lines the figure's rows are counted from. The node that is the host, standing
    for the note itself, is drawn with an inner rim, and refreshHost sets the rim again on every
    node when the host changes.

    No owner pumps it: updateFrames applies the queued updates once per display frame, and a
    rebuild its owner asks for through rebuildRequested waits for that frame too, so no node is
    built before the window has its look-and-feel. OrnamentBuildPanel owns it, sets its pan and
    zoom from the port it sits in and attaches its tree listener to the figure;
    OrnamentFigureController routes the mouse on it.
*/

class FigureDocument;

class OrnamentCanvas : public EditCanvas
{
public:

    explicit OrnamentCanvas(FigureDocument& document);

    void paint(juce::Graphics& graphics) override;

    void rebuildFromNodeMap(const juce::ValueTree& stateTree) override;
    void nodeCreated       (Node& node) override;
    void prepareArrow      (Arrow& arrow) override;

    void refreshHost();

    std::unique_ptr<Node> makeNode(const juce::ValueTree& nodeTree) override;

    static constexpr int initialViewColumns = 4;

    bool rebuildRequested = false;

    juce::VBlankAttachment updateFrames;
};
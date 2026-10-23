#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>

#include "../../Graph/RTData.h"
#include <ArsNovaUI/Editors/LabeledEditor.h>
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

/*
    The list of switches that decides which traversals may follow one arrow. It has a row for
    every traversal key in the graph (a traversal id and an instance number, see TraversalState),
    each with an on or off switch. A key is on unless the arrow's connection tree lists it in its
    DisabledTraversalIds child. Turning a switch off adds an entry for the key to that child, and
    turning it on removes the entry, each as its own undo transaction.

    ArrowGestures opens it in a pop-up from the arrow's context menu and sizes it with
    getIdealHeight. RTGraphBuilder copies the disabled keys into the arrow's RTConnection for the
    audio thread.

    The menu is a pop-up root with no parent, so it takes the CustomLookAndFeel in its constructor
    and sets it on itself.
*/

class GraphState;

class AllowedTraversalsMenu : public juce::Component
{
public:

    AllowedTraversalsMenu(ArsNovaUI::CustomLookAndFeel& lookAndFeel, GraphState& graphState, juce::UndoManager& undoManager, juce::ValueTree connection);

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    int getIdealHeight() const;

    static constexpr int defaultWidth = 160;
    static constexpr int rowHeight    = 26;
    static constexpr int toggleWidth  = 40;
    static constexpr int contentInset = 8;

private:

    bool isTraversalEnabled(const TraversalKey& key) const;
    void setTraversalEnabled(const TraversalKey& key, bool enabled);

    class ToggleButton : public juce::Component
    {
    public:

        void paint(juce::Graphics& graphics) override;

        void mouseDown(const juce::MouseEvent& event) override;

        bool                      isOn = true;
        std::function<void(bool)> onToggle;
    };

    struct TraversalRow
    {
        TraversalKey                             key;
        std::unique_ptr<ArsNovaUI::CaptionLabel> label;
        std::unique_ptr<ToggleButton>            toggle;
    };

    juce::UndoManager& undoManager;
    juce::ValueTree    connection;

    std::vector<TraversalRow> rows;
};

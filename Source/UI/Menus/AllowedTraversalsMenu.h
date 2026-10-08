#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>

#include "../../Graph/RTData.h"
#include "../Editors/LabeledEditor.h"

class CustomLookAndFeel;
class GraphState;

class AllowedTraversalsMenu : public juce::Component
{
public:

    AllowedTraversalsMenu(CustomLookAndFeel& lookAndFeel, GraphState& graphState, juce::UndoManager& undoManager, juce::ValueTree connection);

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    int getIdealHeight() const;

    static constexpr int defaultWidth = 160;
    static constexpr int rowHeight    = 26;
    static constexpr int toggleWidth  = 40;
    static constexpr int contentInset = 8;

private:

    class ToggleButton : public juce::Component
    {
    public:

        void paint(juce::Graphics& graphics) override;

        void mouseDown(const juce::MouseEvent& event) override;

        bool                      isOn = true;
        std::function<void(bool)> onToggle;
    };

    bool isTraversalEnabled(const TraversalKey& key) const;
    void setTraversalEnabled(const TraversalKey& key, bool enabled);

    struct TraversalRow
    {
        TraversalKey                  key;
        std::unique_ptr<CaptionLabel> label;
        std::unique_ptr<ToggleButton> toggle;
    };

    juce::UndoManager& undoManager;
    juce::ValueTree    connection;

    std::vector<TraversalRow> rows;
};

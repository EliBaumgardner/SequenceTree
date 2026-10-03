#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "ItemSelector.h"
#include "../Editors/LabeledEditor.h"
#include "ColourSelector.h"
#include "../Buttons/IconButton.h"
#include "TraversalRulesWindow.h"
#include "../PopupWindow.h"
#include "../Bars/Bar.h"

class TraversalMenu : public juce::Component, private juce::ValueTree::Listener
{
public:

    explicit TraversalMenu(const ApplicationContext& context);
    ~TraversalMenu() override;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    void selectTraversal(int traversalId);

    ItemSelector displayMenu;

    LabeledEditor multiplierField;
    LabeledEditor channelField;
    LabeledEditor transposeField;
    LabeledEditor velocityField;

    CaptionLabel   colourLabel;
    ColourSelector colourSelector;

    IconButton editTraversalRulesButton;

    PopupWindowLauncher traversalRulesLauncher {
        "Traversal Rules",
        [this]() {
            auto content = std::make_unique<TraversalRulesWindow>(applicationContext);

            content->setSize(TraversalRulesWindow::defaultWidth, TraversalRulesWindow::defaultHeight);

            return content;
        }
    };

private:

    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int childIndex) override;

    const ApplicationContext& applicationContext;

    Bar topBar;

    juce::ValueTree currentTraversalData;
};

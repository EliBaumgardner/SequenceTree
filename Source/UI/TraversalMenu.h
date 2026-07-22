//
// Created by Eli Baumgardner on 5/23/26.
//

#ifndef SEQUENCETREE_TRAVERSALMENU_H
#define SEQUENCETREE_TRAVERSALMENU_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

#include "TraversalDisplayMenu.h"
#include "Node/ValueEditor.h"
#include "ColourSelector.h"
#include "Buttons/IconButton.h"
#include "TraversalRulesMenu.h"
#include "PopupWindow.h"
#include "ResizablePanel.h"

class TraversalMenuListener;

class TraversalMenu : public ResizablePanel {

public:

    TraversalMenu(ApplicationContext& context, bool showResizer = true);
    ~TraversalMenu() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void selectTraversal(int traversalId);

    static constexpr int resizerWidth = 15;
    static constexpr int minMenuWidth = resizerWidth;

    TraversalDisplayMenu displayMenu;

    juce::Label multiplierLabel;
    ValueEditor multiplierEditor;

    juce::Label channelLabel;
    ValueEditor channelEditor;

    juce::Label transposeLabel;
    ValueEditor transposeEditor;

    juce::Label velocityLabel;
    ValueEditor velocityEditor;

    juce::Label colourLabel;
    ColourSelector colourSelector;

    std::unique_ptr<IconButton> editTraversalRulesButton;

    PopupWindowLauncher traversalRulesLauncher {
        "Traversal Rules",
        [this]() {
            auto content = std::make_unique<TraversalRulesMenu>(applicationContext);
            content->setSize(TraversalRulesMenu::defaultWidth, TraversalRulesMenu::defaultHeight);

            return content;
        }
    };

private:

    juce::ValueTree currentTraversalData;

    int minimumWidth() const override { return minMenuWidth; }

    std::unique_ptr<TraversalMenuListener> menuListener;
};

#endif //SEQUENCETREE_TRAVERSALMENU_H

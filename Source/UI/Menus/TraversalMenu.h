//
// Created by Eli Baumgardner on 5/23/26.
//

#ifndef SEQUENCETREE_TRAVERSALMENU_H
#define SEQUENCETREE_TRAVERSALMENU_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

#include "ItemSelector.h"
#include "../Editors/ValueEditor.h"
#include "ColourSelector.h"
#include "../Buttons/IconButton.h"
#include "TraversalRulesWindow.h"
#include "../PopupWindow.h"
#include "../Bar.h"

class TraversalMenuListener;

class TraversalMenu : public juce::Component {

public:

    explicit TraversalMenu(ApplicationContext& context);
    ~TraversalMenu() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void selectTraversal(int traversalId);
    void addTraversalToMenu(int traversalId);

    ItemSelector displayMenu;

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
            auto content = std::make_unique<TraversalRulesWindow>(applicationContext);
            content->setSize(TraversalRulesWindow::defaultWidth, TraversalRulesWindow::defaultHeight);

            return content;
        }
    };

private:

    ApplicationContext& applicationContext;

    Bar topBar;

    juce::ValueTree currentTraversalData;

    std::unique_ptr<TraversalMenuListener> menuListener;
};

#endif //SEQUENCETREE_TRAVERSALMENU_H

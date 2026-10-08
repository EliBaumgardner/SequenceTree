#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../Bars/Bar.h"
#include "../PanelResizer.h"
#include "../PopupWindow.h"
#include "../Buttons/ButtonPane.h"
#include "../Theme/CustomLookAndFeel.h"
#include "SettingsMenu.h"

class MenuBar;
class GraphState;
class TraversalRuleState;
class AudioSnapshotPublisher;
class TraversalMenu;
class NodeMenu;

class MenuArea : public juce::Component
{
public:
    MenuArea(NodeCanvas& nodeCanvas, GraphState& graphState, TraversalRuleState& traversalRuleState, AudioSnapshotPublisher& snapshots,
             juce::PropertiesFile& interfaceSettings, juce::ValueTree colourPresets, juce::UndoManager& undoManager);
    ~MenuArea() override;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    static constexpr float resizerWidthRatio = 0.4f;
    static constexpr float menuBarWidthRatio = 1.12f;

    PanelResizer resizer;

private:

    enum class ActivePanel { None, Traversal, Node };

    void togglePanel(ActivePanel panel);

    Bar topBar;

    NodeCanvas&           nodeCanvas;
    juce::PropertiesFile& interfaceSettings;
    juce::ValueTree       colourPresets;
    juce::UndoManager&    undoManager;

    ButtonPane settingsPane;

    PopupWindowLauncher settingsLauncher {
        "Settings",
        [this]() {
            auto content = std::make_unique<SettingsMenu>(CustomLookAndFeel::get(*this), interfaceSettings, colourPresets, undoManager);

            content->setSize(SettingsMenu::defaultWidth, SettingsMenu::defaultHeight);

            return content;
        }
    };

    std::unique_ptr<MenuBar>       menuBar;
    std::unique_ptr<TraversalMenu> traversalMenu;
    std::unique_ptr<NodeMenu>      nodeMenu;

    ActivePanel activePanel = ActivePanel::None;
};

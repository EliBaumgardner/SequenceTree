#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <ArsNovaUI/Bars/Bar.h>
#include <ArsNovaUI/PanelResizer.h>
#include <ArsNovaUI/PopupWindow.h>
#include <ArsNovaUI/Buttons/ButtonPane.h>
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include <ArsNovaUI/Menus/SettingsMenu.h>
#include <ArsNovaUI/ScaledView.h>

class MenuBar;
class NodeCanvas;
class GraphState;
class TraversalRuleState;
class AudioSnapshotPublisher;
class TraversalMenu;
class NodeMenu;

class MenuArea : public juce::Component
{
public:

    MenuArea(NodeCanvas& nodeCanvas, GraphState& graphState, TraversalRuleState& traversalRuleState, AudioSnapshotPublisher& snapshots,
             juce::PropertiesFile& interfaceSettings, ArsNovaUI::ScaledView& scaledView, juce::ValueTree colourPresets, juce::UndoManager& undoManager);

    ~MenuArea() override;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    int narrowestHeight() const;

    static constexpr float resizerWidthRatio = 0.4f;
    static constexpr float menuBarWidthRatio = 1.12f;

    ArsNovaUI::PanelResizer resizer;

private:

    enum class ActivePanel;

    void togglePanel(ActivePanel panel);

    enum class ActivePanel { None, Traversal, Node };

    ArsNovaUI::Bar topBar;

    NodeCanvas&            nodeCanvas;
    juce::PropertiesFile&  interfaceSettings;
    ArsNovaUI::ScaledView& scaledView;
    juce::ValueTree        colourPresets;
    juce::UndoManager&     undoManager;

    ArsNovaUI::ButtonPane settingsPane;

    ArsNovaUI::PopupWindowLauncher settingsLauncher {
        "Settings",
        [this]() {
            auto content = std::make_unique<ArsNovaUI::SettingsMenu>(ArsNovaUI::CustomLookAndFeel::get(*this), interfaceSettings, scaledView, colourPresets, undoManager);

            const juce::Point<int> size = content->narrowestSize();

            content->setSize(size.x, size.y);

            return content;
        }
    };

    std::unique_ptr<MenuBar>       menuBar;
    std::unique_ptr<TraversalMenu> traversalMenu;
    std::unique_ptr<NodeMenu>      nodeMenu;

    ActivePanel activePanel = ActivePanel::None;
};

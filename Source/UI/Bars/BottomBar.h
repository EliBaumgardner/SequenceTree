#pragma once

#include "Bar.h"
#include "../Buttons/IconButton.h"
#include "../Buttons/ButtonPane.h"
#include "../Editors/ValueEditor.h"
#include "../Buttons/PaintToolSettings.h"
#include "../Menus/ArrowWindow.h"
#include "../PopupWindow.h"

class BottomBar : public Bar
{
public:
    explicit BottomBar(const ApplicationContext& context);

    void applyDisplayMode(NodeDisplayMode mode);

private:

    void resized() override;

    static constexpr float paintPanelWidthRatio    = 0.26f;
    static constexpr float toolWidthRatio          = 0.03f;
    static constexpr float quaverPaneWidthRatio    = 0.105f;
    static constexpr float quaverButtonWidthRatio  = 0.28f;
    static constexpr float countsLabelWidthRatio   = 0.42f;
    static constexpr float countsEditorWidthRatio  = 0.30f;

    PopupWindowLauncher arrowWindowLauncher {
        "Arrows",
        [this]() {
            auto content = std::make_unique<ArrowWindow>(applicationContext);
            content->setSize(ArrowWindow::defaultWidth, ArrowWindow::defaultHeight);

            return content;
        }
    };

    std::unique_ptr<PaintToolSettings> paintPanel;
    std::unique_ptr<IconButton> arrowButton;
    std::unique_ptr<IconButton> spanTool;

    ButtonPane  quaverPane   { applicationContext };
    juce::Label countsLabel;
    ValueEditor countsEditor { applicationContext };

    IconButton* quaverTool = nullptr;
};

#pragma once

#include "../Util/PluginModules.h"
#include "../Util/ApplicationContext.h"
#include "Buttons/IconButton.h"
#include "Buttons/PaintToolSettings.h"
#include "Menus/ArrowWindow.h"
#include "PopupWindow.h"

class BottomBar : public juce::Component
{
public:
    explicit BottomBar(ApplicationContext& context);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:

    ApplicationContext& applicationContext;

    static constexpr int cellGap = 10;

    void togglePaintMode();

    void showPaintSettings();
    void applyPaintSetting(PaintToolSettings::PaintSetting setting);

    PopupWindowLauncher paintSettingsLauncher {
        "Paint Brush Settings",
        [this]() {
            auto content = std::make_unique<PaintToolSettings>(applicationContext);
            content->setSize(PaintToolSettings::defaultWidth, PaintToolSettings::defaultHeight);

            return content;
        },
        juce::Colours::black
    };

    PopupWindowLauncher arrowWindowLauncher {
        "Arrows",
        [this]() {
            auto content = std::make_unique<ArrowWindow>(applicationContext);
            content->setSize(ArrowWindow::defaultWidth, ArrowWindow::defaultHeight);

            return content;
        }
    };

    std::unique_ptr<IconButton> paintTool;
    std::unique_ptr<IconButton> arrowButton;
};

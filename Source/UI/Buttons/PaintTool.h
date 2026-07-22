//
// Created by Eli Baumgardner on 6/10/26.
//

#ifndef SEQUENCETREE_PAINTTOOL_H
#define SEQUENCETREE_PAINTTOOL_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Util/ApplicationContext.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../Buttons/PaintToolSettings.h"
#include "../PopupWindow.h"
#include "../Canvas/NodeCanvas.h"


class PaintTool : public juce::Component {

public:

    ApplicationContext& context;

    PopupWindowLauncher settingsLauncher {
        "Paint Brush Settings",
        [this]() {
            auto content = std::make_unique<PaintToolSettings>(this->context);
            content->setSize(PaintToolSettings::defaultWidth, PaintToolSettings::defaultHeight);

            return content;
        },
        juce::Colours::black
    };

    bool isSelected = false;


    PaintTool(ApplicationContext& context) : context(context) {
        setLookAndFeel(context.lookAndFeel);
        settingsLauncher.createIfNeeded();

        context.onDisplayModeChanged = [this](NodeDisplayMode mode) {
            auto* settings = settingsLauncher.getContentAs<PaintToolSettings>();
            if (settings == nullptr) { return; }

            switch (mode) {
                case NodeDisplayMode::Pitch:
                    settings->displayMenu->selectedOption = "Pitch";
                    settings->setPaintMode(PaintToolSettings::PaintSetting::Pitch);
                    settings->displayMenu->resized();
                    break;
                case NodeDisplayMode::Velocity:
                    settings->displayMenu->selectedOption = "Velocity";
                    settings->setPaintMode(PaintToolSettings::PaintSetting::Velocity);
                    settings->displayMenu->resized();
                    break;
                default:
                    break;
            }
        };
    }

    void paint(juce::Graphics& g) override {
        CustomLookAndFeel::get(*this).drawPaintTool(g, *this);
    }

    void mouseDown(const juce::MouseEvent &e) override {

        if (e.mods.isLeftButtonDown()) {
            isSelected = !isSelected;

            if (isSelected) {
                context.canvas->setPaintMode(true);
                if (auto* settings = settingsLauncher.getContentAs<PaintToolSettings>()) {
                    settings->setPaintMode(settings->paintSetting);
                }
            } else {
                context.canvas->setPaintMode(false);
            }
        }

        if (e.mods.isRightButtonDown()) {
            if (auto* settings = settingsLauncher.getContentAs<PaintToolSettings>()) {
                switch (context.currentDisplayMode) {
                    case NodeDisplayMode::Pitch:
                        settings->displayMenu->selectedOption = "Pitch";
                        settings->setPaintMode(PaintToolSettings::PaintSetting::Pitch);
                        settings->displayMenu->resized();
                        break;
                    case NodeDisplayMode::Velocity:
                        settings->displayMenu->selectedOption = "Velocity";
                        settings->setPaintMode(PaintToolSettings::PaintSetting::Velocity);
                        settings->displayMenu->resized();
                        break;
                    default:
                        settings->displayMenu->selectedOption = "Pitch";
                        settings->setPaintMode(PaintToolSettings::PaintSetting::Pitch);
                        settings->displayMenu->resized();
                        break;
                }
            }

            settingsLauncher.show();
        }

    }
};


#endif //SEQUENCETREE_PAINTTOOL_H
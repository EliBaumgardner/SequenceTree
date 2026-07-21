//
// Created by Eli Baumgardner on 6/10/26.
//

#ifndef SEQUENCETREE_PAINTTOOL_H
#define SEQUENCETREE_PAINTTOOL_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Util/ApplicationContext.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../Buttons/PaintToolSettings.h"
#include "../Canvas/NodeCanvas.h"


class PaintTool : public juce::Component {

public:

    ApplicationContext& context;

    std::unique_ptr<PaintToolSettingsWindow> settingsWindow;

    bool isSelected = false;


    PaintTool(ApplicationContext& context) : context(context) {
        setLookAndFeel(context.lookAndFeel);
        settingsWindow = std::make_unique<PaintToolSettingsWindow>(context);
        settingsWindow->centreWithSize(100, 200);

        context.onDisplayModeChanged = [this](NodeDisplayMode mode) {
            auto* settings = dynamic_cast<PaintToolSettings*>(settingsWindow->getContentComponent());
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
                if (auto* settings = dynamic_cast<PaintToolSettings*>(settingsWindow->getContentComponent())) {
                    settings->setPaintMode(settings->paintSetting);
                }
            } else {
                context.canvas->setPaintMode(false);
            }
        }

        if (e.mods.isRightButtonDown()) {
            if (auto* settings = dynamic_cast<PaintToolSettings*>(settingsWindow->getContentComponent())) {
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

            settingsWindow->setVisible(true);
            settingsWindow->toFront(true);
        }

    }
};


#endif //SEQUENCETREE_PAINTTOOL_H
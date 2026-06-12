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
    }

    void paint(juce::Graphics& g){
        CustomLookAndFeel::get(*this).drawPaintTool(g, *this);
    }

    void mouseDown(const juce::MouseEvent &e) override {

        if (e.mods.isLeftButtonDown()) {
            isSelected = !isSelected;

            if (isSelected) {
                context.canvas->setPaintMode(true);
            }
            else {
                context.canvas->setPaintMode(false);
            }
        }

        if (e.mods.isRightButtonDown()) {

            if (settingsWindow == nullptr) {
                settingsWindow = std::make_unique<PaintToolSettingsWindow>(context);
                settingsWindow->centreWithSize(100, 200);
            }

            settingsWindow->setVisible(true);
            settingsWindow->toFront(true);
        }

    }
};


#endif //SEQUENCETREE_PAINTTOOL_H
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

    std::unique_ptr<juce::DialogWindow::LaunchOptions> window;
    std::unique_ptr<PaintToolSettings> settings;

    bool isSelected = false;


    PaintTool(ApplicationContext& context) : context(context) {
        setLookAndFeel(context.lookAndFeel);

        settings = std::make_unique<PaintToolSettings>(context);

        window = std::make_unique<juce::DialogWindow::LaunchOptions>();
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
            isSelected = true;

            std::cout<<"paint tool button pressed"<<std::endl;

            window.get()->content.setNonOwned(settings.get());
            window.get()->content->setSize(100,200);
            window.get()->dialogTitle = "Paint Brush Settings";
            window.get()->dialogBackgroundColour = juce::Colours::black;
            window.get()->escapeKeyTriggersCloseButton = true;
            window.get()->useNativeTitleBar = true;
            window.get()->resizable = true;

            window.get()->launchAsync();
        }

    }
};


#endif //SEQUENCETREE_PAINTTOOL_H
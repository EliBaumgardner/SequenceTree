//
// Created by Eli Baumgardner on 6/10/26.
//

#ifndef SEQUENCETREE_PAINTTOOLSETTINGS_H
#define SEQUENCETREE_PAINTTOOLSETTINGS_H

#include <juce_graphics/juce_graphics.h>
#include "../../Util/ApplicationContext.h"
#include "../Theme/CustomLookAndFeel.h"
#include "DisplayMenu.h"
#include "ColourSelector.h"
#include "../Canvas/NodeCanvas.h"
#include "ValueSlider.h"


class PaintToolDisplayMenu : public DisplayMenu {
public:


    PaintToolDisplayMenu(ApplicationContext& context) : DisplayMenu(context) {
        menu.clear();

        menu.addItem(1,"Pitch");
        menu.addItem(2,"Velocity");
        menu.addItem(3,"Duration");
    }

    void paint(juce::Graphics& g) {
        CustomLookAndFeel::get(*this).drawDisplayMenu(g, *this);
    };
};


class PaintToolSettings : public juce::Component {

public:

    static constexpr float minBrushFlow = 0.002f;
    static constexpr float maxBrushFlow = 0.25f;

    enum class PaintSetting {Pitch, Duration, Velocity};

    struct ColourVariablePair {
        juce::Colour colour;
        PaintSetting setting;
    };

    ColourVariablePair pitchPair    { juce::Colours::white, PaintSetting::Pitch };
    ColourVariablePair velocityPair { juce::Colours::white, PaintSetting::Velocity };
    ColourVariablePair durationPair { juce::Colours::white, PaintSetting::Duration };

    PaintSetting paintSetting = PaintSetting::Pitch;

    ColourVariablePair& currentPair() {
        switch (paintSetting) {
            case PaintSetting::Velocity: return velocityPair;
            case PaintSetting::Duration: return durationPair;
            case PaintSetting::Pitch:
            default:                     return pitchPair;
        }
    }

    ApplicationContext& context;

    juce::TooltipWindow tooltipWindow { this, 800 };

    std::unique_ptr<PaintToolDisplayMenu> displayMenu;
    std::unique_ptr<ColourSelector>       colourSelector;
    std::unique_ptr<ValueSlider>          valueSlider;
    std::unique_ptr<ValueSlider>          flowSlider;


    PaintToolSettings(ApplicationContext& context) : context(context) {
        setLookAndFeel(context.lookAndFeel);

        displayMenu = std::make_unique<PaintToolDisplayMenu>(context);
        colourSelector = std::make_unique<ColourSelector>(context);
        colourSelector->requiresNode = false;
        valueSlider = std::make_unique<ValueSlider>();
        flowSlider  = std::make_unique<ValueSlider>();


        pitchPair.setting = PaintSetting::Pitch;
        velocityPair.setting = PaintSetting::Velocity;
        durationPair.setting = PaintSetting::Duration;

        displayMenu->button.onClick = [this] {
            displayMenu->button.isSelected = true;
            repaint();

            displayMenu->menu.showMenuAsync(juce::PopupMenu::Options(), [this] (int result) {
                displayMenu->button.isSelected = false;

                switch (result) {
                    case 1: displayMenu->selectedOption  = "Pitch";
                        paintSetting = PaintSetting::Pitch;
                        break;
                    case 2: displayMenu->selectedOption  = "Velocity";
                        paintSetting = PaintSetting::Velocity;
                        break;
                    case 3: displayMenu->selectedOption  = "Duration";
                        paintSetting = PaintSetting::Duration;
                        break;
                }

                setPaintMode(paintSetting);

                displayMenu->resized();
                resized();
            });
        };

        colourSelector->onColourPicked = [this](juce::Colour c) {
            if (this->context.canvas != nullptr) {
                this->context.canvas->setBrushColour(c);
            }

            currentPair().colour = c;
        };

        valueSlider->valueChanged = [this] {
            if (this->context.canvas != nullptr) {
                float value  = (float)valueSlider->boundValue.getValue();
                float radius = juce::jmap(value, 0.0f, 1.0f, 1.0f, 200.0f);
                this->context.canvas->setBrushRadius(radius);
            }
        };

        if (context.canvas != nullptr) {
            valueSlider->boundValue = juce::jmap(context.canvas->brushRadius, 1.0f, 200.0f, 0.0f, 1.0f);
        }

        flowSlider->valueChanged = [this] {
            if (this->context.canvas != nullptr) {
                float value = (float)flowSlider->boundValue.getValue();
                this->context.canvas->brushFlow = juce::jmap(value, 0.0f, 1.0f, minBrushFlow, maxBrushFlow);
            }
        };

        if (context.canvas != nullptr) {
            float flow = juce::jlimit(minBrushFlow, maxBrushFlow, context.canvas->brushFlow);
            flowSlider->boundValue = juce::jmap(flow, minBrushFlow, maxBrushFlow, 0.0f, 1.0f);
        }

        displayMenu->setTooltip("Paint mode");
        colourSelector->setTooltip("Brush colour");
        valueSlider->setTooltip("Brush size");
        flowSlider->setTooltip("Brush rate");

        addAndMakeVisible(displayMenu.get());
        addAndMakeVisible(colourSelector.get());
        addAndMakeVisible(valueSlider.get());
        addAndMakeVisible(flowSlider.get());

    };

    void paint(juce::Graphics& g) {
        CustomLookAndFeel::get(*this).drawPaintToolSettings(g,*this);
    };

    void resized() {

        int height = getLocalBounds().getHeight();
        int displayMenuHeight = height * 0.2f;

        auto bounds = getLocalBounds();

        displayMenu->setBounds(bounds.removeFromTop(displayMenuHeight));
        colourSelector->setBounds(bounds.removeFromTop(displayMenuHeight));
        valueSlider->setBounds(bounds.removeFromTop(displayMenuHeight));
        flowSlider->setBounds(bounds.removeFromTop(displayMenuHeight));
    };

    void setPaintMode(PaintSetting setting) {

        paintSetting = setting;

        juce::Colour saved = currentPair().colour;

        colourSelector->colour = saved;
        colourSelector->repaint();

        if (context.canvas != nullptr) {
            context.canvas->setBrushColour(saved);
            context.canvas->setActivePaintLayer((int)setting);
        }
    }


};

class PaintToolSettingsWindow : public juce::DocumentWindow {

public:

    PaintToolSettingsWindow(ApplicationContext& context)
        : juce::DocumentWindow("Paint Brush Settings", juce::Colours::black,
                               juce::DocumentWindow::closeButton, true)
    {
        auto* content = new PaintToolSettings(context);
        content->setSize(100, 200);

        setContentOwned(content, true);
        setResizable(true, true);
    }

    void closeButtonPressed() override {
        setVisible(false);
    }
};

#endif SEQUENCETREE_PAINTTOOLSETTINGS_H
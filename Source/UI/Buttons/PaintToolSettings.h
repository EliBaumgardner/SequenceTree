//
// Created by Eli Baumgardner on 6/10/26.
//

#ifndef SEQUENCETREE_PAINTTOOLSETTINGS_H
#define SEQUENCETREE_PAINTTOOLSETTINGS_H

#include <juce_graphics/juce_graphics.h>
#include "../../Util/ApplicationContext.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../Menus/ItemSelector.h"
#include "../Menus/ColourSelector.h"
#include "../Canvas/NodeCanvas.h"
#include "ValueSlider.h"


class PaintToolSettings : public juce::Component {

public:

    static constexpr float minBrushFlow = 0.002f;
    static constexpr float maxBrushFlow = 0.25f;

    static constexpr int defaultWidth  = 100;
    static constexpr int defaultHeight = 200;

    enum class PaintSetting {Pitch, Duration, Velocity};

    static int itemIdFor(PaintSetting setting) { return (int) setting + 1; }

    struct ColourVariablePair {
        juce::Colour colour;
        PaintSetting setting;
    };

    ColourVariablePair pitchPair    { juce::Colours::red,   PaintSetting::Pitch };
    ColourVariablePair velocityPair { juce::Colours::green, PaintSetting::Velocity };
    ColourVariablePair durationPair { juce::Colours::blue,  PaintSetting::Duration };

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

    std::unique_ptr<ItemSelector>         displayMenu;
    std::unique_ptr<ColourSelector>       colourSelector;
    std::unique_ptr<ValueSlider>          valueSlider;
    std::unique_ptr<ValueSlider>          flowSlider;


    PaintToolSettings(ApplicationContext& context) : context(context) {
        setLookAndFeel(context.lookAndFeel);

        displayMenu = std::make_unique<ItemSelector>(context);
        colourSelector = std::make_unique<ColourSelector>(context);
        colourSelector->requiresNode = false;
        valueSlider = std::make_unique<ValueSlider>();
        flowSlider  = std::make_unique<ValueSlider>();


        pitchPair.setting = PaintSetting::Pitch;
        velocityPair.setting = PaintSetting::Velocity;
        durationPair.setting = PaintSetting::Duration;

        auto addPaintSetting = [this](PaintSetting setting, juce::String label) {
            displayMenu->addItem(itemIdFor(setting), std::move(label), [this, setting]() {
                setPaintMode(setting);
                resized();
            });
        };

        addPaintSetting(PaintSetting::Pitch,    "Pitch");
        addPaintSetting(PaintSetting::Velocity, "Velocity");
        addPaintSetting(PaintSetting::Duration, "Duration");

        displayMenu->setSelectedItem(itemIdFor(paintSetting));

        colourSelector->onColourPicked = [this](juce::Colour c) {
            currentPair().colour = c;

            if (this->context.canvas != nullptr) {
                this->context.canvas->valueField.setBrushColour(c);
                this->context.canvas->valueField.refresh();
            }
        };

        valueSlider->valueChanged = [this] {
            if (this->context.canvas != nullptr) {
                float value  = (float)valueSlider->boundValue.getValue();
                float radius = juce::jmap(value, 0.0f, 1.0f, 1.0f, 200.0f);
                this->context.canvas->valueField.setBrushRadius(radius);
            }
        };

        if (context.canvas != nullptr) {
            valueSlider->boundValue = juce::jmap(context.canvas->valueField.brushRadius, 1.0f, 200.0f, 0.0f, 1.0f);
        }

        flowSlider->valueChanged = [this] {
            if (this->context.canvas != nullptr) {
                float value = (float)flowSlider->boundValue.getValue();
                this->context.canvas->valueField.brushFlow = juce::jmap(value, 0.0f, 1.0f, minBrushFlow, maxBrushFlow);
            }
        };

        if (context.canvas != nullptr) {
            float flow = juce::jlimit(minBrushFlow, maxBrushFlow, context.canvas->valueField.brushFlow);
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
        displayMenu->setSelectedItem(itemIdFor(setting));

        juce::Colour saved = currentPair().colour;

        colourSelector->colour = saved;
        colourSelector->repaint();

        if (context.canvas != nullptr) {
            context.canvas->valueField.setBrushColour(saved);
            context.canvas->valueField.setActivePaintLayer((int)setting);
        }
    }


};

#endif //SEQUENCETREE_PAINTTOOLSETTINGS_H
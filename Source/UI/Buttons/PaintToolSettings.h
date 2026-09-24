//
// Created by Eli Baumgardner on 6/10/26.
//

#ifndef SEQUENCETREE_PAINTTOOLSETTINGS_H
#define SEQUENCETREE_PAINTTOOLSETTINGS_H

#include <juce_graphics/juce_graphics.h>
#include "../../Util/ApplicationContext.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../Menus/ColourSelector.h"
#include "../Editors/ValueEditor.h"
#include "../Canvas/NodeCanvas.h"
#include "IconButton.h"


class PaintToolSettings : public juce::Component {

public:

    static constexpr int   brushDecimalPlaces = 3;
    static constexpr float minBrushFlow = 0.002f;
    static constexpr float maxBrushFlow = 0.25f;

    static constexpr float minBrushRadius = 1.0f;
    static constexpr float maxBrushRadius = 200.0f;

    static constexpr float widthToHeightRatio = 9.0f;
    static constexpr float panelInsetRatio    = 0.1f;
    static constexpr float cellGapRatio       = 0.2f;
    static constexpr float labelWidthRatio    = 1.6f;

    enum class PaintSetting {Pitch, Duration, Velocity};

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

    const ApplicationContext& context;

    std::unique_ptr<IconButton>     paintTool;
    std::unique_ptr<ColourSelector> colourSelector;
    std::unique_ptr<ValueEditor>    sizeEditor;
    std::unique_ptr<ValueEditor>    flowEditor;

    juce::Label sizeLabel;
    juce::Label flowLabel;


    explicit PaintToolSettings(const ApplicationContext& context) : context(context) {
        setLookAndFeel(context.lookAndFeel);

        paintTool = std::make_unique<IconButton>(
            [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
                CustomLookAndFeel::get(*this).drawPaintToolIcon(g, bounds, state);
            }, context.lookAndFeel);

        colourSelector = std::make_unique<ColourSelector>(context);
        colourSelector->requiresNode = false;
        sizeEditor = std::make_unique<ValueEditor>(context);
        flowEditor = std::make_unique<ValueEditor>(context);

        pitchPair.setting    = PaintSetting::Pitch;
        velocityPair.setting = PaintSetting::Velocity;
        durationPair.setting = PaintSetting::Duration;

        paintTool->onClick = [this]() {
            paintTool->toggleSelected();

            const bool paintMode = paintTool->isSelected();
            this->context.canvas->setPaintMode(paintMode);

            if (paintMode) {
                setPaintMode(paintSetting);
            }
        };

        colourSelector->onColourPicked = [this](juce::Colour c) {
            currentPair().colour = c;

            this->context.canvas->valueField.setBrushColour(c);
            this->context.canvas->valueField.refresh();
        };

        sizeEditor->setFormat(std::make_unique<NumberFormat>(0.0, 1.0, brushDecimalPlaces));

        sizeEditor->onValueChange = [this] {
            const float value  = (float)sizeEditor->boundValue.getValue();
            const float radius = juce::jmap(value, 0.0f, 1.0f, minBrushRadius, maxBrushRadius);
            this->context.canvas->valueField.setBrushRadius(radius);
        };

        sizeEditor->boundValue = juce::jmap(context.canvas->valueField.brushRadius,
                                            minBrushRadius, maxBrushRadius, 0.0f, 1.0f);

        flowEditor->setFormat(std::make_unique<NumberFormat>(0.0, 1.0, brushDecimalPlaces));

        flowEditor->onValueChange = [this] {
            const float value = (float)flowEditor->boundValue.getValue();
            this->context.canvas->valueField.brushFlow = juce::jmap(value, 0.0f, 1.0f, minBrushFlow, maxBrushFlow);
        };

        const float flow = juce::jlimit(minBrushFlow, maxBrushFlow, context.canvas->valueField.brushFlow);
        flowEditor->boundValue = juce::jmap(flow, minBrushFlow, maxBrushFlow, 0.0f, 1.0f);

        colourSelector->setTooltip("Brush colour");
        sizeEditor->setTooltip("Brush size");
        flowEditor->setTooltip("Brush rate");

        const auto setUpLabel = [this](juce::Label& label, juce::String text) {
            label.setText(std::move(text), juce::dontSendNotification);
            label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
            label.setFont(juce::Font(juce::FontOptions(9.0f)));
            label.setJustificationType(juce::Justification::centredLeft);
            addAndMakeVisible(label);
        };

        setUpLabel(sizeLabel, "Size");
        setUpLabel(flowLabel, "Rate");

        addAndMakeVisible(paintTool.get());
        addAndMakeVisible(colourSelector.get());
        addAndMakeVisible(sizeEditor.get());
        addAndMakeVisible(flowEditor.get());

    };

    void paint(juce::Graphics& g) override {
        CustomLookAndFeel::get(*this).drawPaintToolSettings(g,*this);
    };

    void resized() override {

        const int height = getLocalBounds().getHeight();

        auto bounds = getLocalBounds().reduced(juce::roundToInt(height * panelInsetRatio));

        const int cellHeight = bounds.getHeight();
        const int cellGap    = juce::roundToInt(cellHeight * cellGapRatio);
        const int labelWidth = juce::roundToInt(cellHeight * labelWidthRatio);

        paintTool->setBounds(bounds.removeFromLeft(cellHeight));
        bounds.removeFromLeft(cellGap);

        colourSelector->setBounds(bounds.removeFromLeft(cellHeight));
        bounds.removeFromLeft(cellGap);

        const int editorWidth = (bounds.getWidth() - labelWidth * 2 - cellGap) / 2;

        sizeLabel.setBounds(bounds.removeFromLeft(labelWidth));
        sizeEditor->setBounds(bounds.removeFromLeft(editorWidth));
        bounds.removeFromLeft(cellGap);

        flowLabel.setBounds(bounds.removeFromLeft(labelWidth));
        flowEditor->setBounds(bounds.removeFromLeft(editorWidth));
    };

    void setPaintMode(PaintSetting setting) {

        paintSetting = setting;

        const juce::Colour saved = currentPair().colour;

        colourSelector->colour = saved;
        colourSelector->repaint();

        context.canvas->valueField.setBrushColour(saved);
        context.canvas->valueField.setActivePaintLayer((int)setting);
    }


};

#endif //SEQUENCETREE_PAINTTOOLSETTINGS_H

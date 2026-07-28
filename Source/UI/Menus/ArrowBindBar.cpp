//
// Created by Eli Baumgardner on 7/27/26.
//

#include "ArrowBindBar.h"
#include "../Theme/CustomLookAndFeel.h"

ArrowBindBar::ArrowBindBar(ApplicationContext& context)
    : Bar(context, { Orientation::horizontal, Background::litFromBottom }),
      fieldSelector(context)
{
    configureField(pitchField);
    configureField(durationField);

    addAndMakeVisible(fieldSelector);

    configureFieldSelector();
}

void ArrowBindBar::configureFieldSelector()
{
    fieldSelector.setTooltip("Arrow Binding");

    fieldSelector.addItem(pitchItemId,    "pitch");
    fieldSelector.addItem(durationItemId, "duration");

    fieldSelector.onItemSelected = [this](int itemId) { showField(itemId); };

    fieldSelector.setSelectedItem(pitchItemId);
    showField(pitchItemId);
}

void ArrowBindBar::configureField(BindField& field)
{
    configureAxis(field.x, "X");
    configureAxis(field.y, "Y");
}

void ArrowBindBar::configureAxis(AxisControl& axis, const juce::String& text)
{
    axis.toggle = std::make_unique<IconButton>(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawTextButton(g, bounds, state,
                                                        juce::jmax(minimumFontHeight,
                                                                   bounds.getHeight() * toggleFontRatio));
        }, applicationContext.lookAndFeel);

    axis.toggle->setText(text);
    axis.toggle->onClick = [button = axis.toggle.get()]() { button->toggleSelected(); };

    axis.editor = std::make_unique<ValueEditor>(applicationContext);
    axis.editor->enableMultiplierValue();
    axis.editor->enableAutoFitText();

    addChildComponent(*axis.toggle);
    addChildComponent(*axis.editor);
}

void ArrowBindBar::showField(int itemId)
{
    const bool showPitch = itemId == pitchItemId;

    auto setFieldVisible = [](BindField& field, bool shouldBeVisible) {
        field.x.toggle->setVisible(shouldBeVisible);
        field.x.editor->setVisible(shouldBeVisible);
        field.y.toggle->setVisible(shouldBeVisible);
        field.y.editor->setVisible(shouldBeVisible);
    };

    setFieldVisible(pitchField,    showPitch);
    setFieldVisible(durationField, ! showPitch);
}

int ArrowBindBar::scaled(int total, float ratio, int minimum)
{
    return juce::jmax(minimum, juce::roundToInt(total * ratio));
}

ArrowBindBar::Metrics ArrowBindBar::metricsFor(juce::Rectangle<int> bounds) const
{
    const int width = bounds.getWidth();

    return { scaled(width, selectorWidthRatio, minimumSelectorWidth),
             scaled(width, toggleWidthRatio,   minimumToggleWidth),
             scaled(width, editorWidthRatio,   minimumEditorWidth),
             scaled(width, axisGapRatio,       minimumGap),
             scaled(width, itemGapRatio,       minimumGap) };
}

void ArrowBindBar::layOutAxis(AxisControl& axis, juce::Rectangle<int>& bounds, const Metrics& metrics)
{
    axis.editor->setBounds(bounds.removeFromRight(metrics.editorWidth));
    bounds.removeFromRight(metrics.axisGap);

    axis.toggle->setBounds(bounds.removeFromRight(metrics.toggleWidth));
}

void ArrowBindBar::layOutField(BindField& field, juce::Rectangle<int> bounds, const Metrics& metrics)
{
    layOutAxis(field.y, bounds, metrics);
    bounds.removeFromRight(metrics.itemGap);

    layOutAxis(field.x, bounds, metrics);
}

void ArrowBindBar::resized()
{
    auto row = getContentBounds();

    const Metrics metrics = metricsFor(row);

    layOutField(pitchField,    row, metrics);
    layOutField(durationField, row, metrics);

    fieldSelector.setBounds(row.removeFromLeft(metrics.selectorWidth));
}

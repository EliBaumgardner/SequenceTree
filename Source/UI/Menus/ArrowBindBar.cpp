//
// Created by Eli Baumgardner on 7/27/26.
//

#include "ArrowBindBar.h"
#include "../Canvas/NodeCanvas.h"

ArrowBindBar::ArrowBindBar(ApplicationContext& context)
    : Bar(context, { Orientation::horizontal, Background::litFromBottom }),
      fieldSelector(context)
{
    configureField(pitchField,    durationField);
    configureField(durationField, pitchField);

    addAndMakeVisible(fieldSelector);

    configureFieldSelector();
    showCurrentBindings();
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

void ArrowBindBar::configureField(BindField& field, BindField& otherField)
{
    configureAxis(field.x, otherField.x, "X:");
    configureAxis(field.y, otherField.y, "Y:");
}

void ArrowBindBar::configureAxis(AxisControl& axis, AxisControl& otherAxis, const juce::String& text)
{
    axis.label.setText(text, juce::dontSendNotification);
    axis.label.setColour(juce::Label::textColourId, juce::Colours::lightgrey.withAlpha(0.85f));
    axis.label.setJustificationType(juce::Justification::centredRight);
    axis.label.setBorderSize(juce::BorderSize<int>(0));

    axis.editor = std::make_unique<ValueEditor>(applicationContext);
    axis.editor->enableDecimalMultiplierValue(deactivatedMultiplier, maximumMultiplier);
    axis.editor->boundValue.setValue(defaultMultiplier);

    axis.editor->onValueChange = [this, editor = axis.editor.get(), other = &otherAxis]() {
        if ((double) editor->boundValue.getValue() > deactivatedMultiplier) {
            other->editor->boundValue.setValue(deactivatedMultiplier);
        }

        publishBindings();
    };

    addChildComponent(axis.label);
    addChildComponent(*axis.editor);
}

void ArrowBindBar::showCurrentBindings()
{
    const ArrowInfo& arrowInfo = applicationContext.canvas->arrowManager.currentArrowInfo;

    showAxis(xAxis, arrowInfo.xBinding, arrowInfo.xMultiplier);
    showAxis(yAxis, arrowInfo.yBinding, arrowInfo.yMultiplier);
}

void ArrowBindBar::showAxis(AxisMember axisMember, ArrowBinding binding, double multiplier)
{
    AxisControl& pitchAxis    = pitchField.*axisMember;
    AxisControl& durationAxis = durationField.*axisMember;

    pitchAxis.editor   ->boundValue.setValue(deactivatedMultiplier);
    durationAxis.editor->boundValue.setValue(deactivatedMultiplier);

    if (binding == ArrowBinding::PitchBind) {
        pitchAxis.editor->boundValue.setValue(multiplier);
    }
    else if (binding == ArrowBinding::DurationBind) {
        durationAxis.editor->boundValue.setValue(multiplier);
    }
}

void ArrowBindBar::publishBindings()
{
    ArrowInfo& arrowInfo = applicationContext.canvas->arrowManager.currentArrowInfo;

    resolveAxis(xAxis, arrowInfo.xBinding, arrowInfo.xMultiplier);
    resolveAxis(yAxis, arrowInfo.yBinding, arrowInfo.yMultiplier);
}

void ArrowBindBar::resolveAxis(AxisMember axisMember, ArrowBinding& binding, double& multiplier) const
{
    const AxisControl& pitchAxis    = pitchField.*axisMember;
    const AxisControl& durationAxis = durationField.*axisMember;

    const double pitchMultiplier    = (double) pitchAxis.editor->boundValue.getValue();
    const double durationMultiplier = (double) durationAxis.editor->boundValue.getValue();

    if (pitchMultiplier > deactivatedMultiplier) {
        binding    = ArrowBinding::PitchBind;
        multiplier = pitchMultiplier;
        return;
    }

    if (durationMultiplier > deactivatedMultiplier) {
        binding    = ArrowBinding::DurationBind;
        multiplier = durationMultiplier;
        return;
    }

    binding    = ArrowBinding::NoBind;
    multiplier = defaultMultiplier;
}

void ArrowBindBar::showField(int itemId)
{
    const bool showPitch = itemId == pitchItemId;

    auto setFieldVisible = [](BindField& field, bool shouldBeVisible) {
        field.x.label.setVisible(shouldBeVisible);
        field.x.editor->setVisible(shouldBeVisible);
        field.y.label.setVisible(shouldBeVisible);
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
             scaled(width, controlWidthRatio,  minimumControlWidth),
             scaled(width, axisGapRatio,       minimumGap),
             scaled(width, itemGapRatio,       minimumGap),
             juce::jmax(minimumFontHeight, bounds.getHeight() * fontHeightRatio) };
}

void ArrowBindBar::layOutAxis(AxisControl& axis, juce::Rectangle<int>& bounds, const Metrics& metrics)
{
    axis.editor->setFontHeight(metrics.fontHeight);
    axis.editor->setBounds(bounds.removeFromRight(metrics.controlWidth));

    bounds.removeFromRight(metrics.axisGap);

    axis.label.setFont(juce::Font(juce::FontOptions(metrics.fontHeight)));
    axis.label.setBounds(bounds.removeFromRight(metrics.controlWidth));
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

#include "ArrowBindBar.h"
#include "../Canvas/NodeCanvas.h"

ArrowBindBar::ArrowBindBar(const ApplicationContext& context)
    : Bar(context, { Orientation::Horizontal, bindBarInsetRatio }),
      fieldSelector(context)
{
    const ArrowInfo& arrowInfo = applicationContext.canvas->arrowManager.currentArrowInfo;

    configureAxis(pitchField.x,    durationField.x, "X:");
    configureAxis(pitchField.y,    durationField.y, "Y:");
    configureAxis(durationField.x, pitchField.x,    "X:");
    configureAxis(durationField.y, pitchField.y,    "Y:");

    fieldSelector.setTooltip("Arrow Binding");

    fieldSelector.addItem(pitchItemId,    "pitch");
    fieldSelector.addItem(durationItemId, "duration");

    fieldSelector.onItemSelected = [this](int itemId) { showField(itemId); };

    addAndMakeVisible(fieldSelector);

    fieldSelector.setSelectedItem(pitchItemId);

    showField(pitchItemId);

    showAxis(xAxis, arrowInfo.xBinding, arrowInfo.xMultiplier);
    showAxis(yAxis, arrowInfo.yBinding, arrowInfo.yMultiplier);
}

void ArrowBindBar::resized()
{
    auto          row     = getContentBounds();
    const int     width   = row.getWidth();
    const Metrics metrics { juce::jmax(minimumSelectorWidth, juce::roundToInt(width * selectorWidthRatio)),
                            juce::jmax(minimumControlWidth,  juce::roundToInt(width * controlWidthRatio)),
                            juce::jmax(minimumGap,           juce::roundToInt(width * axisGapRatio)),
                            juce::jmax(minimumGap,           juce::roundToInt(width * itemGapRatio)),
                            juce::jmax(minimumFontHeight,    row.getHeight() * fontHeightRatio) };

    layOutAxis(*pitchField.y, row, metrics);
    row.removeFromRight(metrics.itemGap);
    layOutAxis(*pitchField.x, row, metrics);

    layOutAxis(*durationField.y, row, metrics);
    row.removeFromRight(metrics.itemGap);
    layOutAxis(*durationField.x, row, metrics);

    fieldSelector.setBounds(row.removeFromLeft(metrics.selectorWidth));
}

void ArrowBindBar::configureAxis(std::unique_ptr<LabeledEditor>& axis, std::unique_ptr<LabeledEditor>& otherAxis, const juce::String& text)
{
    auto multiplierFormat = std::make_unique<NumberFormat>(deactivatedMultiplier, maximumMultiplier, ValueFormat::editableDecimalPlaces);

    multiplierFormat->suffix = "x";
    axis                     = std::make_unique<LabeledEditor>(applicationContext);

    axis->label.setText(text, juce::dontSendNotification);
    axis->label.setJustificationType(juce::Justification::centredRight);

    axis->editor.setFormat(std::move(multiplierFormat));
    axis->editor.boundValue.setValue(defaultMultiplier);

    axis->editor.onValueChange = [this, editor = &axis->editor, other = &otherAxis]() {
        if (static_cast<double>(editor->boundValue.getValue()) > deactivatedMultiplier) {
            (*other)->editor.boundValue.setValue(deactivatedMultiplier);
        }

        publishBindings();
    };

    addChildComponent(*axis);
}

void ArrowBindBar::publishBindings()
{
    ArrowInfo& arrowInfo = applicationContext.canvas->arrowManager.currentArrowInfo;

    resolveAxis(xAxis, arrowInfo.xBinding, arrowInfo.xMultiplier);
    resolveAxis(yAxis, arrowInfo.yBinding, arrowInfo.yMultiplier);
}

void ArrowBindBar::resolveAxis(AxisMember axisMember, ArrowBinding& binding, double& multiplier) const
{
    const LabeledEditor& pitchAxis    = *(pitchField.*axisMember);
    const LabeledEditor& durationAxis = *(durationField.*axisMember);

    const double pitchMultiplier    = static_cast<double>(pitchAxis.editor.boundValue.getValue());
    const double durationMultiplier = static_cast<double>(durationAxis.editor.boundValue.getValue());

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
        field.x->setVisible(shouldBeVisible);
        field.y->setVisible(shouldBeVisible);
    };

    setFieldVisible(pitchField,    showPitch);
    setFieldVisible(durationField, ! showPitch);
}

void ArrowBindBar::showAxis(AxisMember axisMember, ArrowBinding binding, double multiplier)
{
    LabeledEditor& pitchAxis    = *(pitchField.*axisMember);
    LabeledEditor& durationAxis = *(durationField.*axisMember);

    pitchAxis.editor.boundValue.setValue(deactivatedMultiplier);
    durationAxis.editor.boundValue.setValue(deactivatedMultiplier);

    if (binding == ArrowBinding::PitchBind) {
        pitchAxis.editor.boundValue.setValue(multiplier);
    }
    else if (binding == ArrowBinding::DurationBind) {
        durationAxis.editor.boundValue.setValue(multiplier);
    }
}

void ArrowBindBar::layOutAxis(LabeledEditor& axis, juce::Rectangle<int>& bounds, const Metrics& metrics)
{
    axis.editor.setFontHeight(metrics.fontHeight);
    axis.label.setFont(juce::Font(juce::FontOptions(metrics.fontHeight)));

    axis.labelWidth = metrics.controlWidth;
    axis.gap        = metrics.axisGap;

    axis.setBounds(bounds.removeFromRight(metrics.controlWidth * 2 + metrics.axisGap));
}

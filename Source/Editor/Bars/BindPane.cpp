#include "BindPane.h"
#include <ArsNovaUI/Menus/ContextMenu.h>
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

BindPane::BindPane(juce::ValueTree canvasSettings)
    : canvasSettings(canvasSettings)
{
    configureAxis(xAxis, "X");
    configureAxis(yAxis, "Y");

    this->canvasSettings.addListener(this);
}

BindPane::~BindPane()
{
    canvasSettings.removeListener(this);
}

void BindPane::configureAxis(BindAxis& axis, const juce::String& text)
{
    axis.button               = &addButton(text + " Binding", [this, &axis]() { toggleAxis(axis); });
    axis.button->onRightClick = [this, &axis]() { showAxisMenu(axis); };
    axis.button->setText(text);

    axis.multiplierSlider.range  = { minimumBindMultiplier, maximumBindMultiplier, bindMultiplierInterval };
    axis.multiplierSlider.label  = "multiplier";
    axis.multiplierSlider.suffix = "x";

    axis.multiplierSlider.boundValue.referTo(canvasSettings.getPropertyAsValue(axis.multiplierProperty, nullptr));

    valueTreePropertyChanged(canvasSettings, axis.bindingProperty);
}

void BindPane::toggleAxis(BindAxis& axis)
{
    const int storedBinding = canvasSettings.getProperty(axis.bindingProperty);
    const int storedTarget  = canvasSettings.getProperty(axis.targetProperty);
    const int noBinding     = static_cast<int>(ArrowInfo::Binding::NoBind);

    int toggledBinding = noBinding;

    if (storedBinding == noBinding) {
        toggledBinding = storedTarget;
    }

    canvasSettings.setProperty(axis.bindingProperty, toggledBinding, nullptr);
}

void BindPane::showAxisMenu(BindAxis& axis)
{
    const int storedBinding = canvasSettings.getProperty(axis.bindingProperty);

    ArsNovaUI::ContextMenu menu;

    for (const BindTarget& target : bindTargets) {
        const int targetBinding = static_cast<int>(target.binding);

        const bool isChecked = storedBinding == targetBinding;

        menu.addItem(target.label, ArsNovaUI::ContextMenu::ItemKind::Toggle, [this, &axis, targetBinding]() {
            canvasSettings.setProperty(axis.targetProperty,  targetBinding, nullptr);
            canvasSettings.setProperty(axis.bindingProperty, targetBinding, nullptr);
        }, true, isChecked);
    }

    menu.addComponent(axis.multiplierSlider, multiplierSliderWidth, ArsNovaUI::Theme::popupMenuItemHeight);

    menu.show(*axis.button);
}

void BindPane::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    if (tree != canvasSettings) {
        return;
    }

    for (BindAxis* axis : { &xAxis, &yAxis }) {
        if (property == axis->bindingProperty) {
            const int storedBinding = canvasSettings.getProperty(property);

            const bool isBound = static_cast<ArrowInfo::Binding>(storedBinding) != ArrowInfo::Binding::NoBind;

            axis->button->setSelected(isBound);
        }
    }
}

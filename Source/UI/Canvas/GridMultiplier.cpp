#include "GridMultiplier.h"
#include "../../Util/ArrowInfo.h"

GridMultiplier::GridMultiplier(ArsNovaUI::EditCanvas& canvas, juce::ValueTree canvasSettings, const juce::Identifier& multiplierProperty)
    : canvas            (canvas),
      canvasSettings    (canvasSettings),
      multiplierProperty(multiplierProperty)
{
    this->canvasSettings.addListener(this);

    valueTreePropertyChanged(this->canvasSettings, this->multiplierProperty);
}

GridMultiplier::~GridMultiplier()
{
    canvasSettings.removeListener(this);
}

void GridMultiplier::valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier& property)
{
    const double multiplier = canvasSettings.getProperty(multiplierProperty, minimumMultiplier);

    if (property != multiplierProperty) {
        return;
    }

    canvas.gridSpacing = ArrowInfo::pixelsPerGridSpace * static_cast<float>(multiplier);

    canvas.repaint();
}

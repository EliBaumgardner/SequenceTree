//
// Created by Eli Baumgardner on 7/21/26.
//

#include "Theme.h"

void Theme::setColorIntensityFactor(float factor)
{
    colorIntensityFactor = juce::jlimit(0.0f, 1.0f, factor);
    updateColours();
}

juce::Colour Theme::applyIntensity(juce::Colour base) const
{
    float distance        = std::abs(colorIntensityFactor - 0.5f) * 2.0f;
    float hueShift        = (colorIntensityFactor - 0.5f) * 0.5f;
    float saturationBoost = distance * 0.35f;
    float newSaturation   = juce::jlimit(0.0f, 1.0f, base.getSaturation() + saturationBoost);

    float darkness        = 1.0f - base.getBrightness();
    float brightnessLift  = distance * (0.10f + darkness * 0.25f);
    float newBrightness   = juce::jlimit(0.0f, 1.0f, base.getBrightness() + brightnessLift);

    return base.withRotatedHue(hueShift)
               .withSaturation(newSaturation)
               .withBrightness(newBrightness);
}

void Theme::updateColours()
{
}

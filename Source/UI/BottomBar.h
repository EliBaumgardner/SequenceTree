#pragma once

#include "../Util/PluginModules.h"
#include "../Util/ApplicationContext.h"
#include "ColourSelector.h"

class BottomBar : public juce::Component
{
public:
    explicit BottomBar(ApplicationContext& context);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    ApplicationContext& applicationContext;
    ColourSelector colourSelector { applicationContext };
};

#pragma once

#include "../Util/PluginModules.h"
#include "../Util/ApplicationContext.h"
#include "Buttons/PaintTool.h"
#include "Buttons/ArrowTool.h"

class BottomBar : public juce::Component
{
public:
    explicit BottomBar(ApplicationContext& context);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:

    ApplicationContext& applicationContext;

    static constexpr int cellGap = 10;

    std::unique_ptr<PaintTool> paintTool;
    std::unique_ptr<ArrowTool> arrowTool;
};

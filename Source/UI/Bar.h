#pragma once

#include "../Util/PluginModules.h"
#include "../Util/ApplicationContext.h"

class Bar : public juce::Component
{
public:

    enum class Orientation { horizontal, vertical };
    enum class Background  { litFromTop, litFromBottom, flat };

    struct Style {
        Orientation orientation  = Orientation::horizontal;
        Background  background   = Background::litFromTop;
        int         contentInset = 4;
    };

    Bar(ApplicationContext& context, Style style);
    ~Bar() override;

    void paint(juce::Graphics& g) final;

protected:

    virtual void paintOverBar(juce::Graphics& g) {}

    juce::Rectangle<int> getContentBounds() const;
    void drawSeparator(juce::Graphics& g, int position);

    static constexpr int   contentSpacing      = 12;
    static constexpr float separatorInsetRatio = 0.22f;

    ApplicationContext& applicationContext;

private:

    void paintBackground(juce::Graphics& g);

    Style style;
};

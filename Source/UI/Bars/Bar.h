#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Util/ApplicationContext.h"
#include "../Theme/Theme.h"

class Bar : public juce::Component
{
public:

    enum class Orientation { Horizontal, Vertical };

    struct Style
    {
        Orientation orientation       = Orientation::Horizontal;
        float       contentInsetRatio = Theme::contentInsetRatio;
    };

    Bar(const ApplicationContext& context, Style style);
    ~Bar() override;

    void paint(juce::Graphics& graphics) final;

protected:

    virtual void paintOverBar(juce::Graphics& graphics)
    {
    }

    juce::Rectangle<int> getContentBounds() const;
    void drawSeparator(juce::Graphics& graphics, int position);

    static constexpr int   contentSpacing      = 12;
    static constexpr float separatorInsetRatio = 0.22f;

    const ApplicationContext& applicationContext;

private:

    Style style;
};

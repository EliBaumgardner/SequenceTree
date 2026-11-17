#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <ArsNovaUI/Bars/Bar.h>
#include <ArsNovaUI/Bars/TabBar.h>
#include <ArsNovaUI/Buttons/ButtonPane.h>

class RuleTabBar : public ArsNovaUI::Bar
{
public:

    RuleTabBar();

    static constexpr int preferredHeight = ArsNovaUI::Theme::barHeight;

    ArsNovaUI::TabBar tabStrip;

    ArsNovaUI::ButtonPane transportPane;
    ArsNovaUI::ButtonPane undoRedoPane;

private:

    void paintOverBar(juce::Graphics& graphics) override;
    void resized () override;
};

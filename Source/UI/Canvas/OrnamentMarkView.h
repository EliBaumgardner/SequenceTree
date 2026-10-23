#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <ArsNovaUI/Theme/Visuals.h>

/*
    The small component drawn above a node that carries an ornament. It holds the SymbolMark that
    says what to draw (nothing, the ornament's symbol, or a marker for an ornament that no longer
    exists), the symbol's stroke text and the node's tree, and paint draws it with the
    look-and-feel's drawSymbolMark.

    OrnamentMarks creates and positions one per ornamented node. NodeController hands a click on
    it to OrnamentApplier::respondToMarkClick, which uses nodeValueTree to find the node.
*/

class OrnamentMarkView : public juce::Component
{
public:

    void paint(juce::Graphics& graphics) override;

    ArsNovaUI::SymbolMark mark = ArsNovaUI::SymbolMark::None;
    juce::String          symbol;
    juce::ValueTree       nodeValueTree;
};

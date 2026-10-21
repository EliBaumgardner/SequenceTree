#include "OrnamentMarkView.h"
#include "../../Graph/OrnamentLibraries.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

void OrnamentMarkView::paint(juce::Graphics& graphics)
{
    ArsNovaUI::CustomLookAndFeel::get(*this).drawSymbolMark(graphics, getLocalBounds().toFloat(), mark, OrnamentLibraries::readSymbol(symbol));
}

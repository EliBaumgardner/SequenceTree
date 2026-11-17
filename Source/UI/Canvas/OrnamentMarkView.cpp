#include "OrnamentMarkView.h"
#include "../../Graph/OrnamentLibraries.h"
#include "../../Graph/OrnamentSymbol.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

void OrnamentMarkView::paint(juce::Graphics& graphics)
{
    ArsNovaUI::CustomLookAndFeel::get(*this).drawSymbolMark(graphics, getLocalBounds().toFloat(), mark, OrnamentSymbol::read(symbol));
}

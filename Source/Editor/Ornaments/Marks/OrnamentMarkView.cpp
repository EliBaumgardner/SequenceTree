#include "OrnamentMarkView.h"
#include "Processor/Document/Ornaments/OrnamentLibraries.h"
#include "Processor/Document/Ornaments/OrnamentSymbol.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

void OrnamentMarkView::paint(juce::Graphics& graphics)
{
    ArsNovaUI::CustomLookAndFeel::get(*this).drawSymbolMark(graphics, getLocalBounds().toFloat(), mark, OrnamentSymbol::read(symbol));
}

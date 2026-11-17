#pragma once

#include <ArsNovaUI/Buttons/ButtonPane.h>
#include <ArsNovaUI/Buttons/IconButton.h>
#include <juce_data_structures/juce_data_structures.h>

#include <functional>

class NodeCanvas;
class OrnamentLibraries;

class OrnamentPane : public ArsNovaUI::ButtonPane, private juce::ValueTree::Listener
{
public:

    OrnamentPane(NodeCanvas& nodeCanvas, OrnamentLibraries& ornamentLibraries);

    ~OrnamentPane() override;

    std::function<void()> onWindowRequested;

    ArsNovaUI::IconButton* button = nullptr;

private:

    void paintGlyph(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ArsNovaUI::ButtonState& state);
    void showActiveOrnamentName ();

    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;

    static constexpr float gridCellsPerHeight = 5.0f;

    OrnamentLibraries& ornamentLibraries;
};

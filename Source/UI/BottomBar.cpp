#include "BottomBar.h"
#include "Theme/CustomLookAndFeel.h"
#include "../Graph/ValueTreeIdentifiers.h"

BottomBar::BottomBar(ApplicationContext& context)
    : applicationContext(context)
{
    setLookAndFeel(applicationContext.lookAndFeel);

    addAndMakeVisible(colourSelector);

    radiusEditor.setMinimumValue(5);
    radiusEditor.setVisible(false);
    addAndMakeVisible(radiusEditor);

    applicationContext.onNodeSelected = [this](Node* node, bool selected) {
        colourSelector.setNode(selected ? node : nullptr);

        if (selected && node != nullptr) {
            radiusEditor.bindEditor(node->nodeValueTree, ValueTreeIdentifiers::Radius);
            radiusEditor.setVisible(true);
        } else {
            radiusEditor.setVisible(false);
        }

        resized();
    };
}

void BottomBar::paint(juce::Graphics& g)
{
    CustomLookAndFeel::get(*this).drawBottomBar(g, *this);

    if (radiusEditor.isVisible()) {
        g.setFont(juce::Font(juce::FontOptions(9.0f)));
        g.setColour(juce::Colours::lightgrey.withAlpha(0.7f));
        auto labelBounds = radiusEditor.getBounds().withX(radiusEditor.getX() - 12).withWidth(12);
        g.drawText("R", labelBounds, juce::Justification::centred, false);
    }
}

void BottomBar::resized()
{
    auto bounds = getLocalBounds().reduced(4);
    int  h      = bounds.getHeight();

    colourSelector.setBounds(bounds.removeFromLeft(h));

    if (radiusEditor.isVisible()) {
        bounds.removeFromLeft(16);
        radiusEditor.setBounds(bounds.removeFromLeft(28));
    }
}

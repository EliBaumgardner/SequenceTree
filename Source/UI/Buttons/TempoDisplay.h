//
// Created by Eli Baumgardner on 11/9/25.
//

#ifndef SEQUENCETREE_TEMPODISPLAY_H
#define SEQUENCETREE_TEMPODISPLAY_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Theme/CustomLookAndFeel.h"
#include "../../Util/ApplicationContext.h"
#include "../Editors/ValueEditor.h"
#include "IconButton.h"

class TempoDisplay : public juce::Component, public juce::SettableTooltipClient {

    public:

    static constexpr int contentInset = 2;

    std::unique_ptr<IconButton> syncButton;
    ValueEditor editor;

    explicit TempoDisplay(ApplicationContext& context)
        : editor(context)
    {
        setLookAndFeel(context.lookAndFeel);
        setTooltip("Tempo Multiplier");

        editor.enableDecimalMultiplierValue(RTtraversal::minimumTempoMultiplier,
                                            RTtraversal::maximumTempoMultiplier);
        editor.enableAutoFitText();

        syncButton = std::make_unique<IconButton>(
            [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
                CustomLookAndFeel::get(*this).drawSyncIcon(g, bounds, state);
            }, context.lookAndFeel);

        syncButton->setTooltip("Sync to host tempo");
        syncButton->onClick = [this]() {
            syncButton->setSelected(!syncButton->isSelected());
        };
        syncButton->setSelected(true);

        addAndMakeVisible(syncButton.get());
        addAndMakeVisible(editor);
    }

    void paint(juce::Graphics& g) override
    {
        const Theme& theme = CustomLookAndFeel::get(*this);

        g.setColour(theme.buttonBarColour);
        g.fillRoundedRectangle(editor.getBounds().toFloat(), Theme::paneCornerRadius);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(contentInset);
        const int syncSize = bounds.getHeight();
        syncButton->setBounds(bounds.removeFromRight(syncSize));
        bounds.removeFromRight(contentInset);
        editor.setBounds(bounds);
    }
};

#endif //SEQUENCETREE_TEMPODISPLAY_H

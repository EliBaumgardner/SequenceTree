//
// Created by Eli Baumgardner on 11/9/25.
//

#ifndef SEQUENCETREE_TEMPODISPLAY_H
#define SEQUENCETREE_TEMPODISPLAY_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Theme/CustomLookAndFeel.h"
#include "../../Util/ApplicationContext.h"
#include "../Theme/CustomTextEditor.h"
#include "IconButton.h"

class TempoDisplay : public juce::Component, public juce::SettableTooltipClient {

    public:

    std::unique_ptr<IconButton> syncButton;
    CustomTextEditor editor;

    TempoDisplay(ApplicationContext& context)
        : editor(context)
    {
        setLookAndFeel(context.lookAndFeel);
        setTooltip("Tempo Multiplier");

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

        editor.setText("1.0", false);
    }

    void paint(juce::Graphics& g) override
    {

    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(2);
        int syncSize = bounds.getHeight();
        syncButton->setBounds(bounds.removeFromRight(syncSize));
        bounds.removeFromRight(2);
        editor.setBounds(bounds);

        editor.refit();
    }
};

#endif //SEQUENCETREE_TEMPODISPLAY_H

//
// Created by Eli Baumgardner on 11/9/25.
//

#ifndef SEQUENCETREE_TEMPODISPLAY_H
#define SEQUENCETREE_TEMPODISPLAY_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Theme/CustomLookAndFeel.h"
#include "../../Util/ApplicationContext.h"
#include "../Theme/CustomTextEditor.h"
#include "SyncButton.h"

class TempoDisplay : public juce::Component, public juce::SettableTooltipClient {

    public:

    SyncButton syncButton;
    CustomTextEditor editor;

    TempoDisplay(ApplicationContext& context)
        : syncButton(context), editor(context)
    {
        setLookAndFeel(context.lookAndFeel);
        setTooltip("Tempo Multiplier");
        addAndMakeVisible(syncButton);
        addAndMakeVisible(editor);

        editor.setText("1.0", false);
    }

    void paint(juce::Graphics& g) override
    {
        CustomLookAndFeel::get(*this).drawTempoDisplay(g, *this);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(2);
        int syncSize = bounds.getHeight();
        syncButton.setBounds(bounds.removeFromRight(syncSize));
        bounds.removeFromRight(2);
        editor.setBounds(bounds);

        editor.refit();
    }
};

#endif //SEQUENCETREE_TEMPODISPLAY_H

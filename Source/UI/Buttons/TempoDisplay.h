//
// Created by Eli Baumgardner on 11/9/25.
//

#ifndef SEQUENCETREE_TEMPODISPLAY_H
#define SEQUENCETREE_TEMPODISPLAY_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../CustomLookAndFeel.h"
#include "../../Util/PluginContext.h"
#include "../CustomTextEditor.h"
#include "SyncButton.h"

class TempoDisplay : public juce::Component {

    public:

    SyncButton syncButton;
    CustomTextEditor editor;

    TempoDisplay()
    {
        setLookAndFeel(ComponentContext::lookAndFeel);
        addAndMakeVisible(syncButton);
        addAndMakeVisible(editor);

        editor.setText("1.0", false);
    }

    void paint(juce::Graphics& g) override
    {
        if (auto* customLookAndFeel = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel())) { customLookAndFeel->drawTempoDisplay(g, *this); }
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        int editorWidth = bounds.getWidth() * 3 / 4;
        editor.setBounds(bounds.removeFromLeft(editorWidth));
        syncButton.setBounds(bounds.reduced(1.0f));

        editor.refit();
    }
};

#endif //SEQUENCETREE_TEMPODISPLAY_H

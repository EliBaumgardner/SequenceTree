//
// Created by Eli Baumgardner on 11/9/25.
//

#ifndef SEQUENCETREE_TEMPODISPLAY_H
#define SEQUENCETREE_TEMPODISPLAY_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Theme/CustomLookAndFeel.h"
#include "../../Util/ApplicationContext.h"
#include "../Editors/ValueEditor.h"
#include "../../Graph/RTData.h"

class TempoDisplay : public juce::Component, public juce::SettableTooltipClient {

    public:

    static constexpr int contentInset = 2;

    ValueEditor editor;

    explicit TempoDisplay(const ApplicationContext& context)
        : editor(context)
    {
        setLookAndFeel(context.lookAndFeel);
        setTooltip("Tempo Multiplier");

        auto tempoFormat = std::make_unique<NumberFormat>(RTtraversal::minimumTempoMultiplier,
                                                          RTtraversal::maximumTempoMultiplier,
                                                          ValueFormat::editableDecimalPlaces);
        tempoFormat->suffix = "x";

        editor.setFormat(std::move(tempoFormat));
        editor.autoFitText = true;

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
        editor.setBounds(getLocalBounds().reduced(contentInset));
    }
};

#endif //SEQUENCETREE_TEMPODISPLAY_H

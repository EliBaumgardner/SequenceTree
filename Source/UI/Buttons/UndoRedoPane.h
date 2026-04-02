//
// Created by Eli Baumgardner on 11/9/25.
//

#ifndef SEQUENCETREE_UNDOREDOPANE_H
#define SEQUENCETREE_UNDOREDOPANE_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../CustomLookAndFeel.h"
#include "../../Util/PluginContext.h"
#include "UndoButton.h"
#include "RedoButton.h"

class UndoRedoPane : public juce::Component {

public:

    UndoButton undoButton;
    RedoButton redoButton;

    UndoRedoPane()
    {
        setLookAndFeel(ComponentContext::lookAndFeel);
        addAndMakeVisible(undoButton);
        addAndMakeVisible(redoButton);

        undoButton.onClick = [=]() {
            ComponentContext::undoManager->undo();
        };

        redoButton.onClick = [=]() {

        };

    }

    void paint(juce::Graphics& g) override
    {
        if (auto* customLookAndFeel = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel())) { customLookAndFeel->drawUndoRedoPane(g, *this); }
    }

    void resized() override
    {

        auto bounds = getLocalBounds().reduced(2.0f);
        int buttonSize = bounds.getHeight();
        int numButtons = 2;
        float totalButtonWidth = buttonSize * numButtons;

        float spacing = (bounds.getWidth() - totalButtonWidth) / (numButtons + 1);

        int x = static_cast<int>(bounds.getX() + spacing);
        undoButton.setBounds(x, bounds.getY(), buttonSize, buttonSize);

        x += buttonSize + spacing;
        redoButton.setBounds(x, bounds.getY(), buttonSize, buttonSize);
    }
};

#endif //SEQUENCETREE_UNDOREDOPANE_H

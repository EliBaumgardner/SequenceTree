//
// Created by Eli Baumgardner on 7/27/26.
//

#ifndef SEQUENCETREE_ARROWBINDBAR_H
#define SEQUENCETREE_ARROWBINDBAR_H

#include "../Bar.h"
#include "../Buttons/IconButton.h"
#include "../Node/ValueEditor.h"

class ArrowBindBar : public Bar {

public:

    explicit ArrowBindBar(ApplicationContext& context);

    static constexpr int rowHeight       = 24;
    static constexpr int preferredHeight = rowHeight * 2 + 8;

private:

    struct BindRow {
        juce::Label                  label;
        std::unique_ptr<ValueEditor> editor;
        std::unique_ptr<IconButton>  xToggle;
        std::unique_ptr<IconButton>  yToggle;
    };

    void resized() override;

    void configureRow(BindRow& row, const juce::String& labelText);
    void layOutRow(BindRow& row, juce::Rectangle<int> bounds);

    std::unique_ptr<IconButton> createToggle(const juce::String& text);

    static constexpr int toggleWidth = 20;
    static constexpr int editorWidth = 34;
    static constexpr int itemGap     = 6;

    BindRow pitchRow;
    BindRow durationRow;
};

#endif //SEQUENCETREE_ARROWBINDBAR_H

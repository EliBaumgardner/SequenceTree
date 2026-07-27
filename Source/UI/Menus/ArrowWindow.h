//
// Created by Eli Baumgardner on 7/27/26.
//

#ifndef SEQUENCETREE_ARROWWINDOW_H
#define SEQUENCETREE_ARROWWINDOW_H

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <optional>
#include <vector>

#include "../../Util/ApplicationContext.h"
#include "../Buttons/ButtonPane.h"

enum class ArrowType { Node };

class ArrowWindow : public juce::Component {

public:

    explicit ArrowWindow(ApplicationContext& context);
    ~ArrowWindow() override;

    std::function<void(std::optional<ArrowType>)> onArrowTypeChanged;

    std::optional<ArrowType> getSelectedArrowType() const;

    void paint(juce::Graphics& g) override;
    void resized() override;

    static constexpr int defaultWidth  = 220;
    static constexpr int defaultHeight = 140;

private:

    struct ArrowTypeButton {
        ArrowType         type;
        const IconButton* button;
    };

    void addArrowType(ArrowType type, const juce::String& caption, IconButton::Painter painter);

    std::optional<ArrowType> arrowTypeFor(const IconButton* button) const;

    static constexpr ButtonPane::Grid arrowGrid { 56, 56, 8, 10 };

    ButtonPane arrowTypePane;

    std::vector<ArrowTypeButton> arrowTypeButtons;
};

#endif //SEQUENCETREE_ARROWWINDOW_H

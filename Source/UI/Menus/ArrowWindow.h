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
#include "ArrowBindBar.h"

enum class ArrowType { Node, Polyphonic };

class ArrowWindow : public juce::Component {

public:

    explicit ArrowWindow(ApplicationContext& context);
    ~ArrowWindow() override;

    std::function<void(std::optional<ArrowType>)> onArrowTypeChanged;

    std::optional<ArrowType> getSelectedArrowType() const;

    void paint(juce::Graphics& g) override;
    void resized() override;

    static constexpr int defaultWidth  = 220;
    static constexpr int defaultHeight = 140 + ArrowBindBar::preferredHeight;

private:

    struct ArrowTypeButton {
        ArrowType         type;
        const IconButton* button;
    };

    void addArrowType(ArrowType type, const juce::String& caption, IconButton::Painter painter);

    std::optional<ArrowType> arrowTypeFor(const IconButton* button) const;

    static ButtonPane::Grid arrowGridFor(juce::Rectangle<int> bounds);

    static constexpr float bindBarHeightRatio = 0.2f;

    static constexpr float cellWidthRatio  = 0.25f;
    static constexpr float cellHeightRatio = 0.4f;
    static constexpr float gridGapRatio    = 0.036f;
    static constexpr float gridInsetRatio  = 0.045f;

    static constexpr int minimumCellSize = 24;
    static constexpr int minimumGridGap  = 2;

    ButtonPane   arrowTypePane;
    ArrowBindBar bindBar;

    std::vector<ArrowTypeButton> arrowTypeButtons;
};

#endif //SEQUENCETREE_ARROWWINDOW_H

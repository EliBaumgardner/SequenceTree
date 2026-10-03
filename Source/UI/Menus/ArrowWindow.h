#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <optional>
#include <vector>

#include "../../Util/ApplicationContext.h"
#include "../Buttons/ButtonPane.h"
#include "../Bars/ArrowBindBar.h"

class ArrowWindow : public juce::Component
{
public:

    explicit ArrowWindow(const ApplicationContext& context);
    ~ArrowWindow() override;

    std::function<void(std::optional<ArrowType>)> onArrowTypeChanged;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    static constexpr int defaultWidth  = 220;
    static constexpr int defaultHeight = 140 + ArrowBindBar::preferredHeight;

private:

    struct ArrowTypeButton
    {
        ArrowType         type;
        const IconButton* button;
    };

    std::optional<ArrowType> arrowTypeFor(const IconButton* button) const;
    void addArrowType(ArrowType type, const juce::String& caption, IconButton::Icon icon);

    static constexpr float bindBarHeightRatio = 0.2f;

    static constexpr float cellWidthRatio  = 0.25f;
    static constexpr float cellHeightRatio = 0.4f;
    static constexpr float gridGapRatio    = 0.036f;
    static constexpr float gridInsetRatio  = 0.045f;

    static constexpr int minimumCellSize = 24;
    static constexpr int minimumGridGap  = 2;

    const ApplicationContext& applicationContext;

    ButtonPane   arrowTypePane;
    ArrowBindBar bindBar;

    std::vector<ArrowTypeButton> arrowTypeButtons;
};

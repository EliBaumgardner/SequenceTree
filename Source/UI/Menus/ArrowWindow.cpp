#include "ArrowWindow.h"
#include "../Canvas/NodeCanvas.h"
#include "../Theme/CustomLookAndFeel.h"

ArrowWindow::ArrowWindow(const ApplicationContext& context)
    : applicationContext(context), arrowTypePane(context), bindBar(context)
{
    const ArrowType currentType = applicationContext.canvas->arrowManager.currentArrowInfo.type;

    setLookAndFeel(context.lookAndFeel);

    arrowTypePane.selection = ButtonPane::Selection::ExclusiveOrNone;

    arrowTypePane.onSelectionChanged = [this](const IconButton* selected) {
        const std::optional<ArrowType> arrowType = arrowTypeFor(selected);

        applicationContext.canvas->arrowManager.currentArrowInfo.type = arrowType.value_or(ArrowType::Node);

        if (onArrowTypeChanged) {
            onArrowTypeChanged(arrowType);
        }
    };

    addAndMakeVisible(arrowTypePane);
    addAndMakeVisible(bindBar);

    addArrowType(ArrowType::Node,       "node arrow",       &CustomLookAndFeel::drawNodeArrowIcon);
    addArrowType(ArrowType::Polyphonic, "polyphonic arrow", &CustomLookAndFeel::drawPolyphonicArrowIcon);
    addArrowType(ArrowType::Traversal,  "traversal arrow",  &CustomLookAndFeel::drawTraversalArrowIcon);

    for (const ArrowTypeButton& arrowTypeButton : arrowTypeButtons) {
        if (currentType != ArrowType::Node && arrowTypeButton.type == currentType) {
            arrowTypePane.setSelectedButton(arrowTypeButton.button);
        }
    }
}

ArrowWindow::~ArrowWindow()
{
    setLookAndFeel(nullptr);
}

void ArrowWindow::paint(juce::Graphics& graphics)
{
    const Theme& theme = CustomLookAndFeel::get(*this);

    graphics.setColour(theme.surfaceColour);
    graphics.fillRect(getLocalBounds());

    graphics.setColour(theme.borderColour);
    graphics.drawRect(getLocalBounds(), 1);
}

void ArrowWindow::resized()
{
    auto      bounds        = getLocalBounds();
    const int bindBarHeight = juce::jmax(ArrowBindBar::minimumHeight, juce::roundToInt(bounds.getHeight() * bindBarHeightRatio));

    bindBar.setBounds(bounds.removeFromBottom(bindBarHeight));

    const int cellSize = juce::jmax(minimumCellSize, juce::jmin(juce::roundToInt(bounds.getWidth()  * cellWidthRatio),
                                                                juce::roundToInt(bounds.getHeight() * cellHeightRatio)));

    arrowTypePane.gridLayout = ButtonPane::Grid { cellSize,
                                                  cellSize,
                                                  juce::jmax(minimumGridGap, juce::roundToInt(bounds.getWidth() * gridGapRatio)),
                                                  juce::jmax(minimumGridGap, juce::roundToInt(bounds.getWidth() * gridInsetRatio)) };

    arrowTypePane.setBounds(bounds);
    arrowTypePane.resized();
}

std::optional<ArrowType> ArrowWindow::arrowTypeFor(const IconButton* button) const
{
    for (const ArrowTypeButton& arrowTypeButton : arrowTypeButtons) {
        if (arrowTypeButton.button == button) {
            return arrowTypeButton.type;
        }
    }

    return std::nullopt;
}

void ArrowWindow::addArrowType(ArrowType type, const juce::String& caption, IconButton::Icon icon)
{
    IconButton& button = arrowTypePane.addButton(icon, caption);

    button.state.look = ButtonState::Look::Raised;

    button.setCaption(caption);

    arrowTypeButtons.push_back({ type, &button });
}

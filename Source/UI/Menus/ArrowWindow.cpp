//
// Created by Eli Baumgardner on 7/27/26.
//

#include "ArrowWindow.h"
#include "../Theme/CustomLookAndFeel.h"

ArrowWindow::ArrowWindow(ApplicationContext& context)
    : applicationContext(context), arrowTypePane(context), bindBar(context)
{
    setLookAndFeel(context.lookAndFeel);

    arrowTypePane.enableToggleSelection();
    arrowTypePane.allowEmptySelection();

    arrowTypePane.onSelectionChanged = [this](const IconButton* selected) {
        const std::optional<ArrowType> arrowType = arrowTypeFor(selected);

        applicationContext.currentArrowType = arrowType.value_or(ArrowType::Node);

        if (onArrowTypeChanged) {
            onArrowTypeChanged(arrowType);
        }
    };

    addAndMakeVisible(arrowTypePane);
    addAndMakeVisible(bindBar);

    addArrowType(ArrowType::Node, "node arrow",
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawNodeArrowIcon(g, bounds, state);
        });

    addArrowType(ArrowType::Polyphonic, "polyphonic arrow",
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawPolyphonicArrowIcon(g, bounds, state);
        });

    addArrowType(ArrowType::Traversal, "traversal arrow",
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawTraversalArrowIcon(g, bounds, state);
        });

    showSelectedArrowType();
}

ArrowWindow::~ArrowWindow() {
    setLookAndFeel(nullptr);
}

void ArrowWindow::addArrowType(ArrowType type, const juce::String& caption, IconButton::Painter painter) {
    IconButton& button = arrowTypePane.addButton(std::move(painter), caption);

    button.setCaption(caption);

    arrowTypeButtons.push_back({ type, &button });
}

std::optional<ArrowType> ArrowWindow::arrowTypeFor(const IconButton* button) const {
    for (const ArrowTypeButton& arrowTypeButton : arrowTypeButtons) {
        if (arrowTypeButton.button == button) {
            return arrowTypeButton.type;
        }
    }

    return std::nullopt;
}

std::optional<ArrowType> ArrowWindow::getSelectedArrowType() const {
    return arrowTypeFor(arrowTypePane.getSelectedButton());
}

void ArrowWindow::showSelectedArrowType() {
    if (applicationContext.currentArrowType == ArrowType::Node) {
        return;
    }

    for (const ArrowTypeButton& arrowTypeButton : arrowTypeButtons) {
        if (arrowTypeButton.type == applicationContext.currentArrowType) {
            arrowTypePane.setSelectedButton(arrowTypeButton.button);
            return;
        }
    }
}

void ArrowWindow::paint(juce::Graphics& g) {
    const Theme& theme = CustomLookAndFeel::get(*this);

    g.setColour(theme.baseDarkColour2);
    g.fillRect(getLocalBounds());

    g.setColour(juce::Colours::black);
    g.drawRect(getLocalBounds(), 1);
}

ButtonPane::Grid ArrowWindow::arrowGridFor(juce::Rectangle<int> bounds) {
    const int cellSize = juce::jmax(minimumCellSize,
                                    juce::jmin(juce::roundToInt(bounds.getWidth()  * cellWidthRatio),
                                               juce::roundToInt(bounds.getHeight() * cellHeightRatio)));

    return { cellSize,
             cellSize,
             juce::jmax(minimumGridGap, juce::roundToInt(bounds.getWidth() * gridGapRatio)),
             juce::jmax(minimumGridGap, juce::roundToInt(bounds.getWidth() * gridInsetRatio)) };
}

void ArrowWindow::resized() {
    auto bounds = getLocalBounds();

    const int bindBarHeight = juce::jmax(ArrowBindBar::minimumHeight,
                                         juce::roundToInt(bounds.getHeight() * bindBarHeightRatio));

    bindBar.setBounds(bounds.removeFromBottom(bindBarHeight));

    arrowTypePane.setBounds(bounds);
    arrowTypePane.useGridLayout(arrowGridFor(bounds));
}

//
// Created by Eli Baumgardner on 7/27/26.
//

#include "ArrowWindow.h"
#include "../Theme/CustomLookAndFeel.h"

ArrowWindow::ArrowWindow(ApplicationContext& context)
    : arrowTypePane(context), bindBar(context)
{
    setLookAndFeel(context.lookAndFeel);

    arrowTypePane.enableToggleSelection();
    arrowTypePane.allowEmptySelection();
    arrowTypePane.useGridLayout(arrowGrid);

    arrowTypePane.onSelectionChanged = [this](const IconButton* selected) {
        if (onArrowTypeChanged) {
            onArrowTypeChanged(arrowTypeFor(selected));
        }
    };

    addAndMakeVisible(arrowTypePane);
    addAndMakeVisible(bindBar);

    addArrowType(ArrowType::Node, "node arrow",
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawNodeArrowIcon(g, bounds, state);
        });
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

void ArrowWindow::paint(juce::Graphics& g) {
    const Theme& theme = CustomLookAndFeel::get(*this);

    g.setColour(theme.baseDarkColour2);
    g.fillRect(getLocalBounds());

    g.setColour(juce::Colours::black);
    g.drawRect(getLocalBounds(), 1);
}

void ArrowWindow::resized() {
    auto bounds = getLocalBounds();

    bindBar.setBounds(bounds.removeFromBottom(ArrowBindBar::preferredHeight));
    arrowTypePane.setBounds(bounds);
}

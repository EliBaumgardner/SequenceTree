//
// Created by Eli Baumgardner on 7/21/26.
//

#include "PopupWindow.h"

PopupWindow::PopupWindow(const juce::String& title, std::unique_ptr<juce::Component> content,
                         juce::Colour backgroundColour)
    : juce::DocumentWindow(title, backgroundColour, juce::DocumentWindow::closeButton, true)
{
    setContentOwned(content.release(), true);
    setResizable(true, true);
}

void PopupWindow::closeButtonPressed() {
    setVisible(false);
}

PopupWindowLauncher::PopupWindowLauncher(juce::String title, ContentFactory factory,
                                         juce::Colour backgroundColour)
    : windowTitle(std::move(title)),
      contentFactory(std::move(factory)),
      windowBackgroundColour(backgroundColour)
{
}

PopupWindowLauncher::PopupWindowLauncher(juce::String title, juce::Colour backgroundColour)
    : windowTitle(std::move(title)),
      windowBackgroundColour(backgroundColour)
{
}

void PopupWindowLauncher::createIfNeeded() {
    if (window == nullptr) {
        window = std::make_unique<PopupWindow>(windowTitle, contentFactory(), windowBackgroundColour);
    }
}

void PopupWindowLauncher::show() {
    createIfNeeded();
    presentWindow();
}

void PopupWindowLauncher::show(const ContentFactory& factory) {
    window = std::make_unique<PopupWindow>(windowTitle, factory(), windowBackgroundColour);
    presentWindow();
}

void PopupWindowLauncher::presentWindow() {
    window->centreWithSize(window->getWidth(), window->getHeight());
    window->setVisible(true);
    window->toFront(true);
}

void PopupWindowLauncher::close() {
    window.reset();
}

void PopupWindowLauncher::toFront() {
    if (window != nullptr) {
        window->toFront(true);
    }
}

bool PopupWindowLauncher::isShowing() const {
    return window != nullptr && window->isVisible();
}

juce::Component* PopupWindowLauncher::getContent() const {
    return window != nullptr ? window->getContentComponent() : nullptr;
}

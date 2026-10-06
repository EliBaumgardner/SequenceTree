#include "PopupWindow.h"

PopupWindow::PopupWindow(const juce::String& title, std::unique_ptr<juce::Component> content,
                         juce::Colour backgroundColour)
    : juce::DocumentWindow(title, backgroundColour, juce::DocumentWindow::closeButton, true)
{
    setUsingNativeTitleBar(true);

    setContentOwned(content.release(), true);
    setResizable(true, true);

    if (! juce::JUCEApplicationBase::isStandaloneApp()) {
        setAlwaysOnTop(true);
    }

    setResizeLimits(juce::roundToInt(getWidth() * minimumSizeRatio), juce::roundToInt(getHeight() * minimumSizeRatio),
                    juce::roundToInt(getWidth() * maximumSizeRatio), juce::roundToInt(getHeight() * maximumSizeRatio));
}

void PopupWindow::closeButtonPressed()
{
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

void PopupWindowLauncher::show()
{
    if (window == nullptr) {
        window = std::make_unique<PopupWindow>(windowTitle, contentFactory(), windowBackgroundColour);
    }

    presentWindow();
}

void PopupWindowLauncher::presentWindow()
{
    window->centreWithSize(window->getWidth(), window->getHeight());
    window->setVisible(true);
    window->toFront(true);
}

void PopupWindowLauncher::show(const ContentFactory& factory)
{
    window = std::make_unique<PopupWindow>(windowTitle, factory(), windowBackgroundColour);

    presentWindow();
}

void PopupWindowLauncher::toFront()
{
    if (window != nullptr) {
        window->toFront(true);
    }
}

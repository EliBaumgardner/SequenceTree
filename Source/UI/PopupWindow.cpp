#include "PopupWindow.h"

PopupWindow::PopupWindow(const juce::String& title, std::unique_ptr<juce::Component> content, juce::Component& origin, juce::Point<float> openingScale,
                         juce::Colour backgroundColour)
    : juce::DocumentWindow(title, backgroundColour, juce::DocumentWindow::closeButton, true),
      content(std::move(content)),
      scaledView(*this->content, origin.findParentComponentOfClass<ScaledView>()->interfaceSettings)
{
    const int                minimumWidth  = this->content->getWidth();
    const int                minimumHeight = this->content->getHeight();
    const juce::Point<float> openingZoom   = openingScale * scaledView.zoom;

    setUsingNativeTitleBar(true);

    scaledView.setSize(juce::roundToInt(minimumWidth * openingZoom.x), juce::roundToInt(minimumHeight * openingZoom.y));

    scaledView.minimumContentSize = { minimumWidth, minimumHeight };

    scaledView.onZoomChanged = [this]() {
        const float                          zoom        = scaledView.zoom;
        const juce::Point<int>               minimumSize = scaledView.minimumContentSize;
        const juce::Displays::Display* const display     = juce::Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds());
        juce::ComponentPeer* const           peer        = getPeer();
        juce::BorderSize<int>                frame;

        if (peer != nullptr) {
            frame = peer->getFrameSize();
        }

        setResizeLimits(juce::roundToInt(minimumSize.x * zoom) + frame.getLeftAndRight(), juce::roundToInt(minimumSize.y * zoom) + frame.getTopAndBottom(),
                        maximumWindowSide, maximumWindowSide);

        if (display != nullptr) {
            setBounds(getBounds().constrainedWithin(display->userArea));
        }
    };

    setContentNonOwned(&scaledView, true);
    setResizable(true, true);

    if (! juce::JUCEApplicationBase::isStandaloneApp()) {
        setAlwaysOnTop(true);
    }

    scaledView.onZoomChanged();
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

void PopupWindowLauncher::show(juce::Component& origin)
{
    if (window == nullptr) {
        window = std::make_unique<PopupWindow>(windowTitle, contentFactory(), origin, openingScale, windowBackgroundColour);
    }

    presentWindow();
}

void PopupWindowLauncher::presentWindow()
{
    window->centreWithSize(window->getWidth(), window->getHeight());
    window->setVisible(true);
    window->toFront(true);
}

void PopupWindowLauncher::show(juce::Component& origin, const ContentFactory& factory)
{
    window = std::make_unique<PopupWindow>(windowTitle, factory(), origin, openingScale, windowBackgroundColour);

    presentWindow();
}

void PopupWindowLauncher::toFront()
{
    if (window != nullptr) {
        window->toFront(true);
    }
}

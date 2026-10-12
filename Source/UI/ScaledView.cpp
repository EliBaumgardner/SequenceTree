#include "ScaledView.h"
#include "Theme/InterfaceIdentifiers.h"

ScaledView::ScaledView(juce::Component& content, juce::PropertiesFile& interfaceSettings)
    : content(content), interfaceSettings(interfaceSettings)
{
    const float savedZoom = static_cast<float>(interfaceSettings.getDoubleValue(InterfaceIdentifiers::InterfaceZoom.toString(), 1.0));

    zoom         = juce::jlimit(minimumZoom, maximumZoom, savedZoom);
    previousZoom = zoom;

    interfaceSettings.addChangeListener(this);

    addAndMakeVisible(content);
}

ScaledView::~ScaledView()
{
    interfaceSettings.removeChangeListener(this);
}

void ScaledView::resized()
{
    const int contentWidth  = juce::roundToInt(static_cast<float>(getWidth()) / zoom);
    const int contentHeight = juce::roundToInt(static_cast<float>(getHeight()) / zoom);

    content.setBounds(0, 0, contentWidth, contentHeight);
    content.setTransform(juce::AffineTransform::scale(zoom));
}

bool ScaledView::zoomFits(float candidateZoom) const
{
    const juce::Component* const         window       = getTopLevelComponent();
    const juce::Displays::Display* const display      = juce::Desktop::getInstance().getDisplays().getDisplayForRect(window->getScreenBounds());
    const int                            chromeWidth  = window->getWidth() - getWidth();
    const int                            chromeHeight = window->getHeight() - getHeight();

    if (display == nullptr) {
        return true;
    }

    return juce::roundToInt(minimumContentSize.x * candidateZoom) + chromeWidth <= display->userArea.getWidth()
           && juce::roundToInt(minimumContentSize.y * candidateZoom) + chromeHeight <= display->userArea.getHeight();
}

void ScaledView::changeListenerCallback(juce::ChangeBroadcaster*)
{
    const float savedZoom = static_cast<float>(interfaceSettings.getDoubleValue(InterfaceIdentifiers::InterfaceZoom.toString(), 1.0));

    previousZoom = zoom;
    zoom         = juce::jlimit(minimumZoom, maximumZoom, savedZoom);

    if (juce::approximatelyEqual(zoom, previousZoom)) {
        return;
    }

    resized();

    if (onZoomChanged) {
        onZoomChanged();
    }
}

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "ScaledView.h"

#include <concepts>
#include <functional>
#include <memory>

class PopupWindow : public juce::DocumentWindow
{
public:

    PopupWindow(const juce::String& title, std::unique_ptr<juce::Component> content, juce::Component& origin, juce::Point<float> openingScale,
                juce::Colour backgroundColour = juce::Colour::fromRGB(30, 30, 30));

    void closeButtonPressed() override;

    std::unique_ptr<juce::Component> content;

private:

    ScaledView scaledView;

    static constexpr int maximumWindowSide = 8192;
};

class PopupWindowLauncher
{
public:

    using ContentFactory = std::function<std::unique_ptr<juce::Component>()>;

    PopupWindowLauncher(juce::String title, ContentFactory factory,
                        juce::Colour backgroundColour = juce::Colour::fromRGB(30, 30, 30));

    explicit PopupWindowLauncher(juce::String title,
                                 juce::Colour backgroundColour = juce::Colour::fromRGB(30, 30, 30));

    void show(juce::Component& origin);
    void show(juce::Component& origin, const ContentFactory& factory);
    void toFront();

    template <std::derived_from<juce::Component> ContentType>
    ContentType* getContentAs() const
    {
        if (window == nullptr) {
            return nullptr;
        }

        return dynamic_cast<ContentType*>(window->content.get());
    }

    std::unique_ptr<PopupWindow> window;

    juce::Point<float> openingScale { 1.0f, 1.0f };

private:

    void presentWindow();

    juce::String   windowTitle;
    ContentFactory contentFactory;
    juce::Colour   windowBackgroundColour;
};

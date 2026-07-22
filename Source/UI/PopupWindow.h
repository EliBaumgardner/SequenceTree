//
// Created by Eli Baumgardner on 7/21/26.
//

#ifndef SEQUENCETREE_POPUPWINDOW_H
#define SEQUENCETREE_POPUPWINDOW_H

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>

class PopupWindow : public juce::DocumentWindow {

public:

    PopupWindow(const juce::String& title, std::unique_ptr<juce::Component> content,
                juce::Colour backgroundColour = juce::Colour::fromRGB(30, 30, 30));

    void closeButtonPressed() override;
};

class PopupWindowLauncher {

public:

    using ContentFactory = std::function<std::unique_ptr<juce::Component>()>;

    PopupWindowLauncher(juce::String title, ContentFactory factory,
                        juce::Colour backgroundColour = juce::Colour::fromRGB(30, 30, 30));

    explicit PopupWindowLauncher(juce::String title,
                                 juce::Colour backgroundColour = juce::Colour::fromRGB(30, 30, 30));

    void show();
    void show(const ContentFactory& factory);
    void close();
    void toFront();

    void createIfNeeded();

    bool isShowing() const;

    juce::Component* getContent() const;

    template <typename ContentType>
    ContentType* getContentAs() const { return dynamic_cast<ContentType*>(getContent()); }

private:

    void presentWindow();

    juce::String   windowTitle;
    ContentFactory contentFactory;
    juce::Colour   windowBackgroundColour;

    std::unique_ptr<PopupWindow> window;
};

#endif //SEQUENCETREE_POPUPWINDOW_H

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <vector>

struct ApplicationContext;

class ContextMenu
{
public:

    enum class ItemKind {
        Action,
        Toggle,
        Component
    };

    explicit ContextMenu(const ApplicationContext& context);

    void addItem(juce::String label, ItemKind kind, std::function<void()> action,
                 bool isEnabled = true, bool isOn = false);

    void addComponent(juce::Component& component, int idealWidth, int idealHeight);
    void show(juce::Component& target);

    std::function<void()> onDismissed;

private:

    struct Item
    {
        juce::String          label;
        ItemKind              kind;
        std::function<void()> action;
        bool                  isEnabled;
        bool                  isOn;
        juce::Component*      component   = nullptr;
        int                   idealWidth  = 0;
        int                   idealHeight = 0;
    };

    const ApplicationContext& applicationContext;

    std::vector<Item> items;
};

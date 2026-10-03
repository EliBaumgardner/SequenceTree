#include "ContextMenu.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../../Util/ApplicationContext.h"

ContextMenu::ContextMenu(const ApplicationContext& context)
    : applicationContext(context)
{
}

void ContextMenu::addItem(juce::String label, ItemKind kind, std::function<void()> action,
                          bool isEnabled, bool isOn)
{
    items.push_back({ std::move(label), kind, std::move(action), isEnabled, isOn });
}

void ContextMenu::show(juce::Component& target)
{
    juce::PopupMenu                               menu;
    juce::Component::SafePointer<juce::Component> safeTarget(&target);
    int                                           itemId = 1;

    menu.setLookAndFeel(applicationContext.lookAndFeel);

    for (const Item& item : items) {
        switch (item.kind) {
            case ItemKind::Action: {
                menu.addItem(itemId, item.label, item.isEnabled, false);
                break;
            }
            case ItemKind::Toggle: {
                menu.addItem(itemId, item.label, item.isEnabled, item.isOn);
                break;
            }
        }

        ++itemId;
    }

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(target).withMousePosition(),
                       [safeTarget, chosenItems = items, dismissed = onDismissed](int result) {
        if (safeTarget == nullptr) {
            return;
        }

        if (dismissed) {
            dismissed();
        }

        if (result <= 0 || result > static_cast<int>(chosenItems.size())) {
            return;
        }

        const Item& chosen = chosenItems[static_cast<size_t>(result - 1)];

        if (chosen.action) {
            chosen.action();
        }
    });
}

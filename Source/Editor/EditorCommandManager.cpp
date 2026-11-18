#include "EditorCommandManager.h"

EditorCommandManager::EditorCommandManager(juce::ApplicationCommandTarget& windowTarget)
    : windowTarget(windowTarget)
{
}

juce::ApplicationCommandTarget* EditorCommandManager::getFirstCommandTarget(juce::CommandID)
{
    juce::Component* const                focusedComponent = juce::Component::getCurrentlyFocusedComponent();
    juce::ApplicationCommandTarget* const focusedTarget    = findTargetForComponent(focusedComponent);

    if (focusedTarget != nullptr) {
        return focusedTarget;
    }

    return &windowTarget;
}

#include "EditorCommandManager.h"

EditorCommandManager::EditorCommandManager(juce::ApplicationCommandTarget& windowTarget)
    : windowTarget(windowTarget)
{
}

juce::ApplicationCommandTarget* EditorCommandManager::getFirstCommandTarget(juce::CommandID commandID)
{
    juce::Component* const                focusedComponent = juce::Component::getCurrentlyFocusedComponent();
    juce::ApplicationCommandTarget* const focusedTarget    = findTargetForComponent(focusedComponent);

    const bool focusedTargetHandlesCommand = focusedTarget != nullptr && focusedTarget->getTargetForCommand(commandID) != nullptr;

    if (focusedTargetHandlesCommand) {
        return focusedTarget;
    }

    return &windowTarget;
}

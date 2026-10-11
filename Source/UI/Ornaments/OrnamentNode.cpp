#include "OrnamentNode.h"
#include "../../Graph/OrnamentLibraries.h"

OrnamentNode::OrnamentNode(juce::UndoManager& undoManager)
    : Node(undoManager),
      velocityEditor(undoManager)
{
    auto velocityFormat = std::make_unique<NumberFormat>(1, OrnamentLibraries::maximumVelocityPercent);

    velocityFormat->suffix = "%";

    velocityEditor.backdrop = ValueEditor::Backdrop::Badge;

    velocityEditor.setFormat(std::move(velocityFormat));
    velocityEditor.setTooltip("Velocity");
    velocityEditor.setInterceptsMouseClicks(false, true);

    addAndMakeVisible(velocityEditor);
}

void OrnamentNode::resized()
{
    Node::resized();

    velocityEditor.setBounds(countEditor.getBounds());
}

void OrnamentNode::bindToTree()
{
    countEditor.setVisible(false);
    switchCountEditor.setVisible(false);
    subLoopLimitEditor.setVisible(false);

    velocityEditor.bindEditor(nodeValueTree, ValueTreeIdentifiers::OrnamentVelocity);
}

void OrnamentNode::bindValueEditorForMode()
{
    auto offsetFormat = std::make_unique<NumberFormat>(-OrnamentLibraries::maximumOffset, OrnamentLibraries::maximumOffset);

    offsetFormat->showsPositiveSign = true;

    nodeValueEditor.editable = true;

    nodeValueEditor.setFormat(std::move(offsetFormat));
    nodeValueEditor.bindEditor(nodeValueTree, ValueTreeIdentifiers::OrnamentOffset);
}

void OrnamentNode::respondToClick(juce::Point<int> localPoint)
{
    if (velocityEditor.getBounds().contains(localPoint)) {
        velocityEditor.beginEditing();

        return;
    }

    Node::respondToClick(localPoint);
}

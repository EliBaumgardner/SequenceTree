#include "Encapsulator.h"

#include "../../Graph/GraphState.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../Canvas/NodeCanvas.h"
#include "../Theme/CustomLookAndFeel.h"

Encapsulator::Encapsulator(juce::UndoManager& undoManager, GraphState& graphState, NodeCanvas& nodeCanvas)
    : Node(undoManager), graphState(graphState), nodeCanvas(nodeCanvas)
{
    nodeType    = NodeType::Encapsulator;
    hasInnerRim = true;

    countEditor.setVisible(false);
    switchCountEditor.setVisible(false);
    subLoopLimitEditor.setVisible(false);
}

void Encapsulator::bindToTree()
{
    memberNodeIds.clear();

    firstMemberValueTree = {};

    memberNodeIds = graphState.encapsulation.memberIds(nodeValueTree.getProperty(ValueTreeIdentifiers::Id));

    if (! memberNodeIds.empty()) {
        firstMemberValueTree = graphState.getNode(memberNodeIds.front());
    }

    const bool             hasFirstMember  = firstMemberValueTree.isValid();
    const juce::Identifier firstMemberType = firstMemberValueTree.getType();

    const bool firstMemberIsAlternative = (firstMemberType == ValueTreeIdentifiers::AlternativeNodeData
                                        || firstMemberType == ValueTreeIdentifiers::AlternativeModulatorData);

    juce::Identifier subLoopProperty = ValueTreeIdentifiers::SubLoopCountLimit;

    countEditor.setVisible(hasFirstMember);
    switchCountEditor.setVisible(hasFirstMember);
    subLoopLimitEditor.setVisible(hasFirstMember && ! firstMemberIsAlternative);

    if (! hasFirstMember) {
        return;
    }

    if (firstMemberType == ValueTreeIdentifiers::RootNodeData) {
        subLoopProperty = ValueTreeIdentifiers::LoopLimit;
    }

    countEditor.bindEditor(firstMemberValueTree, ValueTreeIdentifiers::CountLimit);
    switchCountEditor.bindEditor(firstMemberValueTree, ValueTreeIdentifiers::SwitchCountLimit);
    subLoopLimitEditor.bindEditor(firstMemberValueTree, subLoopProperty);

    countEditor.repaint();
    switchCountEditor.repaint();
    subLoopLimitEditor.repaint();
}

void Encapsulator::bindValueEditorForMode()
{
    nodeValueEditor.setFormat(std::make_unique<GreekLetterFormat>());

    nodeValueEditor.editable = false;

    nodeValueEditor.bindEditor(nodeValueTree, ValueTreeIdentifiers::EncapsulatorLabel);
}

void Encapsulator::syncHighlightsFromMembers()
{
    std::map<int, juce::Colour> memberHighlights;
    std::vector<int>            endedRunIds;

    for (const int memberNodeId : memberNodeIds) {
        Node* const member = nodeCanvas.nodeManager.find(memberNodeId);

        if (member == nullptr) {
            continue;
        }

        for (const auto& highlight : member->activeHighlights) {
            memberHighlights[highlight.first] = highlight.second;
        }
    }

    for (const auto& highlight : activeHighlights) {
        if (memberHighlights.count(highlight.first) == 0) {
            endedRunIds.push_back(highlight.first);
        }
    }

    for (const int endedRunId : endedRunIds) {
        setHighlightVisual(endedRunId, false, juce::Colours::white);
    }

    for (const auto& highlight : memberHighlights) {
        const bool isAlreadyShown = activeHighlights.count(highlight.first) > 0
                                 && pendingHighlightOffIds.count(highlight.first) == 0;

        if (isAlreadyShown) {
            continue;
        }

        setHighlightVisual(highlight.first, true, highlight.second);
    }
}

void Encapsulator::lookAndFeelChanged()
{
    Node::lookAndFeelChanged();

    nodeCanvas.encapsulationView.recolourGroup(*this);
}

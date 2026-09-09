//
// Created by Eli Baumgardner on 9/6/26.
//

#include "Encapsulator.h"

#include "../../Graph/GraphState.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../Canvas/NodeCanvas.h"
#include "../Theme/CustomLookAndFeel.h"

Encapsulator::Encapsulator(ApplicationContext& context) : Node(context)
{
    nodeType    = NodeType::Encapsulator;
    hasInnerRim = true;

    countEditor.setVisible(false);
    switchCountEditor.setVisible(false);
    subLoopLimitEditor.setVisible(false);
}

void Encapsulator::paint(juce::Graphics& g)
{
    CustomLookAndFeel::get(*this).drawNode(g, getNodeVisual());
}

void Encapsulator::bindValueEditorForMode()
{
    nodeValueEditor.setFormat(std::make_unique<GreekLetterFormat>());
    nodeValueEditor.setEditable(false);
    nodeValueEditor.bindEditor(nodeValueTree, ValueTreeIdentifiers::EncapsulatorLabel);
}

void Encapsulator::bindToTree()
{
    memberNodeIds.clear();
    firstMemberValueTree = {};

    if (applicationContext.graphState != nullptr) {
        memberNodeIds = applicationContext.graphState->encapsulatedNodeIds(
            nodeValueTree.getProperty(ValueTreeIdentifiers::Id));
    }

    if (! memberNodeIds.empty() && applicationContext.graphState != nullptr) {
        firstMemberValueTree = applicationContext.graphState->getNode(memberNodeIds.front());
    }

    subLoopLimitEditor.setVisible(true);
    subLoopLimitEditor.bindEditor(nodeValueTree, ValueTreeIdentifiers::SubLoopCountLimit);
    subLoopLimitEditor.repaint();

    const bool hasFirstMember = firstMemberValueTree.isValid();

    countEditor      .setVisible(hasFirstMember);
    switchCountEditor.setVisible(hasFirstMember);

    if (! hasFirstMember) {
        return;
    }

    countEditor      .bindEditor(firstMemberValueTree, ValueTreeIdentifiers::CountLimit);
    switchCountEditor.bindEditor(firstMemberValueTree, ValueTreeIdentifiers::SwitchCountLimit);

    countEditor      .repaint();
    switchCountEditor.repaint();
}

void Encapsulator::syncHighlightsFromMembers()
{
    std::map<int, juce::Colour> memberHighlights;

    for (const int memberNodeId : memberNodeIds) {
        Node* const member = applicationContext.canvas->nodeManager.find(memberNodeId);

        if (member == nullptr) {
            continue;
        }

        for (const auto& highlight : member->activeHighlights) {
            memberHighlights[highlight.first] = highlight.second;
        }
    }

    std::vector<int> endedRunIds;

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

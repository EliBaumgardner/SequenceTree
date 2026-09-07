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
    nodeType = NodeType::Encapsulator;

    countEditor.setVisible(false);
    switchCountEditor.setVisible(false);
    subLoopLimitEditor.setVisible(false);
}

void Encapsulator::paint(juce::Graphics& g)
{
    CustomLookAndFeel::get(*this).drawEncapsulatorNode(g, getNodeVisual());
}

void Encapsulator::setDisplayMode(NodeDisplayMode newMode)
{
    mode = newMode;

    nodeValueEditor.setFormat(std::make_unique<GreekLetterFormat>());
    nodeValueEditor.setEditable(false);
    nodeValueEditor.bindEditor(nodeValueTree, ValueTreeIdentifiers::EncapsulatorLabel);

    nodeValueEditor.repaint();
    repaint();
}

void Encapsulator::bindToEncapsulatedNodes()
{
    memberNodeIds.clear();

    const juce::ValueTree encapsulatedIds =
        nodeValueTree.getChildWithName(ValueTreeIdentifiers::EncapsulatedIds);

    for (int i = 0; i < encapsulatedIds.getNumChildren(); ++i) {
        memberNodeIds.push_back(encapsulatedIds.getChild(i).getProperty(ValueTreeIdentifiers::Id));
    }

    firstMemberValueTree = {};

    if (! memberNodeIds.empty() && applicationContext.graphState != nullptr) {
        firstMemberValueTree = applicationContext.graphState->getNode(memberNodeIds.front());
    }

    if (isExpanded) {
        for (const int memberNodeId : memberNodeIds) {
            Node* const member = applicationContext.canvas->nodeManager.find(memberNodeId);

            if (member == nullptr) {
                continue;
            }

            member->isEncapsulationRinged   = true;
            member->isEncapsulationEntry    = memberNodeId == memberNodeIds.front();
            member->encapsulationRingColour = nodeColour;
            member->repaint();
        }
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

    std::vector<int> endedTraversalIds;

    for (const auto& highlight : activeHighlights) {
        if (memberHighlights.count(highlight.first) == 0) {
            endedTraversalIds.push_back(highlight.first);
        }
    }

    for (const int endedTraversalId : endedTraversalIds) {
        setHighlightVisual(endedTraversalId, false, juce::Colours::white);
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

//
// Created by Eli Baumgardner.
//

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Util/ApplicationContext.h"
#include "../Theme/CustomLookAndFeel.h"
#include "Node.h"
#include "NodeArrow.h"

class DanglingArrow : public juce::Component
{
public:

    DanglingArrow(Node* startNode, juce::Point<int> tipOffset, ApplicationContext& context)
        : startNode(startNode), tipOffset(tipOffset)
    {
        setLookAndFeel(context.lookAndFeel);
        setInterceptsMouseClicks(false, false);
    }

    void paint(juce::Graphics& g) override
    {
        CustomLookAndFeel::get(*this).drawDanglingArrow(g, *this);
    }

    juce::Point<int> getTip() const
    {
        if (startNode == nullptr) {
            return tipOffset;
        }
        return startNode->getNodeCentre() + tipOffset;
    }

    int getDuration() const
    {
        if (startNode == nullptr) {
            return 0;
        }

        juce::Point<int> start = startNode->getNodeCentre();
        juce::Point<int> tip   = getTip();

        float dx = float(tip.x - start.x);
        float dy = float(tip.y - start.y);

        if (startNode->isAlternativeNode) {
            return (int)(std::abs(dy) * NodeArrow::durationAmount);
        }
        return (int)(std::abs(dx) * NodeArrow::durationAmount);
    }

    juce::String getDurationLabel() const
    {
        int duration = getDuration();

        if (startNode != nullptr && startNode->nodeType == NodeType::Modulator) {
            return juce::String(duration / 10) + "%";
        }
        return juce::String(duration);
    }

    void setTipOffset(juce::Point<int> offset)
    {
        tipOffset = offset;
        setArrowBounds();
    }

    void setArrowBounds()
    {
        if (startNode == nullptr) {
            return;
        }

        juce::Point<int> start = startNode->getNodeCentre();
        juce::Point<int> tip   = getTip();

        setBounds(juce::Rectangle<int>(start, tip).expanded(arrowBoundsPadding));
        repaint();
    }

    Node* startNode = nullptr;
    juce::Point<int> tipOffset;
    juce::ValueTree arrowTree;

    static constexpr int arrowBoundsPadding = 40;
};

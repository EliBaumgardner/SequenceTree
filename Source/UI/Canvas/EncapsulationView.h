#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <span>

class NodeCanvas;
class Encapsulator;
class GraphState;

class EncapsulationView
{
public:
    EncapsulationView(NodeCanvas& nodeCanvas, GraphState& graphState);

    void collapse   (int encapsulatorId) const;
    void expand     (int encapsulatorId) const;
    void showMembers(std::span<const int> memberNodeIds) const;
    void refreshMembership(int encapsulatorId) const;
    void collapseAll() const;
    juce::Point<int> collapsedSpanShift(int nodeId) const;
    void syncHighlights() const;
    void recolourGroup(const Encapsulator& encapsulator) const;

private:

    bool applyCollapsedState(int encapsulatorId) const;
    void repositionAll() const;

    NodeCanvas& nodeCanvas;
    GraphState& graphState;
};

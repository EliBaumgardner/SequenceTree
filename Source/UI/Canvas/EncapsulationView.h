//
// Created by Eli Baumgardner on 9/7/26.
//

#ifndef SEQUENCETREE_ENCAPSULATIONVIEW_H
#define SEQUENCETREE_ENCAPSULATIONVIEW_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

class NodeCanvas;
class Encapsulator;
struct ApplicationContext;

class EncapsulationView {

public:

    EncapsulationView(NodeCanvas& canvas, ApplicationContext& context);

    void collapse   (int encapsulatorId) const;
    void expand     (int encapsulatorId) const;
    void collapseAll() const;

    void refreshMembership(int encapsulatorId) const;
    void showMembers(const std::vector<int>& memberNodeIds) const;

    juce::Point<int> collapsedSpanShift(int nodeId) const;

    void syncHighlights() const;
    void recolourGroup(const Encapsulator& encapsulator, juce::Colour colour) const;

private:

    bool applyCollapsedState(int encapsulatorId) const;
    void repositionAll() const;

    NodeCanvas&         canvas;
    ApplicationContext& applicationContext;
};

#endif //SEQUENCETREE_ENCAPSULATIONVIEW_H

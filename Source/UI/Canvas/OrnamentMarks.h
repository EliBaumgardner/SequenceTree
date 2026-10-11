#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <map>
#include <memory>

#include "../../Util/NodeInfo.h"

class NodeCanvas;
class Node;
class OrnamentLibraries;

class OrnamentMarkView : public juce::Component
{
public:

    void paint(juce::Graphics& graphics) override;

    OrnamentMark    mark = OrnamentMark::None;
    juce::String    symbol;
    juce::ValueTree nodeValueTree;
};

class OrnamentMarks : private juce::ValueTree::Listener, private juce::ComponentListener
{
public:

    OrnamentMarks(NodeCanvas& nodeCanvas, OrnamentLibraries& ornamentLibraries);
    ~OrnamentMarks() override;

    void refresh(Node& node);

    static constexpr float markHeightRatio = 0.6f;

private:

    void valueTreeChildAdded       (juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved     (juce::ValueTree& parent, juce::ValueTree& child, int index) override;
    void valueTreePropertyChanged  (juce::ValueTree& tree, const juce::Identifier& property) override;
    void componentMovedOrResized   (juce::Component& component, bool wasMoved, bool wasResized) override;
    void componentVisibilityChanged(juce::Component& component) override;
    void componentBeingDeleted     (juce::Component& component) override;
    void place                     (Node& node);
    void refreshAll                ();

    NodeCanvas&        nodeCanvas;
    OrnamentLibraries& ornamentLibraries;

    std::map<juce::Component*, std::unique_ptr<OrnamentMarkView>> views;
};

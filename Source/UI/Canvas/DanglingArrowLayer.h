//
// Created by Eli Baumgardner on 7/21/26.
//

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

struct ApplicationContext;

class NodeCanvas;
class Node;
class DanglingArrow;

class DanglingArrowLayer {

public:

    DanglingArrowLayer(NodeCanvas& canvas, ApplicationContext& context);
    ~DanglingArrowLayer();

    void setArrowMode(bool enabled);
    bool isArrowMode() const { return arrowMode; }

    void updatePreview(Node* node, juce::Point<int> tipOffset, bool dashed = false);
    void commitPreview();
    void cancelPreview();
    bool hasPreview() const { return preview != nullptr; }

    void add(Node* node, juce::Point<int> tipOffset);
    void remove(DanglingArrow* arrow);

    DanglingArrow* hitTestHead(juce::Point<int> canvasPos, float radius) const;

    void setTip   (DanglingArrow* arrow, juce::Point<int> tipOffset);
    void commitTip(DanglingArrow* arrow);

    void rebuildForNode  (int nodeId);
    void removeForNode   (Node* node);
    void removeForNodeId (int nodeId);

    void clear();

    juce::OwnedArray<DanglingArrow> arrows;

private:

    NodeCanvas&         canvas;
    ApplicationContext& applicationContext;

    std::unique_ptr<DanglingArrow> preview;

    bool arrowMode = false;
};

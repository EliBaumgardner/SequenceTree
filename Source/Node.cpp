/*
  ==============================================================================

    Node.cpp
    Created: 6 May 2025 8:37:02pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#include "Node.h"
#include "NodeCanvas.h"


int Node::globalNodeID = 0;

Node::Node(NodeCanvas* nodeCanvas) : nodeCanvas(nodeCanvas),nodeID(++globalNodeID){
    
    nodeLogic.setNode(this);
    nodeData.setNode(this);
    editor.makeBoundsVisible(false);
    editor.setColour(juce::TextEditor::textColourId, juce::Colours::white);
 
    nodeController = std::make_unique<NodeController>(nodeCanvas,this);
    
    this->addMouseListener(nodeController.get(), true);
    
    editor.setText("1");
    addAndMakeVisible(editor);
    editor.refit();
    
    editor.onTextChange = [=]()
        {
            if(editor.getText().getIntValue() != 0){
                nodeLogic.setCountLimit(editor.getText().getIntValue());
                nodeData.nodeData.setProperty("countLimit",editor.getText().getIntValue(),nullptr);
                nodeCanvas->updateProcessorGraph(nodeCanvas->root);
            }
        };
}

Node::~Node(){
    
    //this->removeMouseListener(nodeController);
}

void Node::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto circleBorder = bounds.reduced(2.5f);
    auto circleSelect = bounds.reduced(0.5f);
    auto circleHover = bounds.reduced(4.5f);
    auto circleFill = bounds.reduced(5.5f);

    g.setColour(juce::Colours::black);
    g.drawEllipse(circleBorder, 1.0f);

    
    g.setColour(isHighlighted ? nodeColour.darker() : nodeColour);
    g.fillEllipse(circleFill);
    
    if (isHovered)
        g.drawEllipse(circleHover, 2.0f);
    
    if (isSelected)
    {
        juce::Path dottedPath;
        dottedPath.addEllipse(circleSelect);

        juce::PathStrokeType stroke(0.325f);

        float dashLengths[] = { 2.935f, 2.935f };
        stroke.createDashedStroke(dottedPath, dottedPath, dashLengths, 2);

        g.setColour(juce::Colours::black);
        g.strokePath(dottedPath, stroke);
    }
}


void Node::resized(){
    
    editor.setBounds(getLocalBounds().reduced(10.0f));
    editor.setJustification(juce::Justification::centred);
    editor.refit();
    
    nodeData.nodeData.setProperty("x",getX(),nullptr);
    nodeData.nodeData.setProperty("y",getY(),nullptr);
    nodeData.nodeData.setProperty("radius", getWidth()/2,nullptr);
}

void Node::setHoverVisual(bool isHovered){
    
    this->isHovered = isHovered;
    repaint();
}

void Node::setSelectVisual(bool isSelected){
    
    this->isSelected = isSelected;
    repaint();
}

void Node::setSelectVisual(){
    
    isSelected = !isSelected;
    repaint();
}

void Node::setHighlightVisual(bool isHighlighted){
    
    this->isHighlighted = isHighlighted;
    repaint();
}

NodeData* Node::getNodeData(){
    
    return &nodeData;
}


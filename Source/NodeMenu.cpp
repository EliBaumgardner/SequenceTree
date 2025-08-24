/*
  ==============================================================================

    NodeMenu.cpp
    Created: 6 May 2025 8:37:51pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#include "NodeMenu.h"
#include "Node.h"
#include "NodeData.h"

NodeMenu::NodeMenu() {
    nameEditor.displayHint(true,"Node Name");
    sliderBar = std::make_unique<SliderBar>(SliderBar::SliderPosition::Right, 10);
    menuRoot  = std::make_unique<MenuItem>();
    selector  = std::make_unique<ColourSelector>();

    addAndMakeVisible(nameEditor);
    addAndMakeVisible(sliderBar.get());
    addAndMakeVisible(menuRoot.get());
    addAndMakeVisible(selector.get());
    
}

NodeMenu::~NodeMenu() {
    
}

void NodeMenu::paint(juce::Graphics& g){
    g.setColour(nodeMenuColour);
    g.fillAll();
}

void NodeMenu::resized() {
    auto width = getWidth();
    auto height = getHeight();

    float nameWidth = width / 1.375f;
    float nameHeight = height / 15.0f;

    float fieldWidth = width / 6.0f;
    float fieldHeight = height / 20.0f;

    float spacing = width * 0.025f;

    nameEditor.setBounds(0, 0, nameWidth, nameHeight);
    nameEditor.refit();
    selector->setBounds(nameWidth,0,nameWidth*0.25,nameHeight);

    if (nodeDataField != nullptr)
        nodeDataField->setBounds(0, nameHeight + spacing, width-10, fieldHeight);

    sliderBar->resized();

    menuRoot->setBounds(0, (nameHeight + fieldHeight) + spacing, width - 5.0f, height - (nameHeight + fieldHeight + spacing));
    menuRoot->resized();
    
    
}

void NodeMenu::setDisplayedNode(Node* node) {
    displayedNode = node;
    auto* displayedNodeData = displayedNode->getNodeData();
    
    
    selector->setNode(displayedNode);
    
    if(!midiNoteItem){
        
        menuRoot->selectedNode = displayedNode;
        
        midiNoteItem = std::make_unique<MenuItem>("Midi Notes",MenuItem::ItemType::MidiNoteItem, displayedNode);
        midiNoteItem->setOpenState(openNote);
        
        // 4. Add them
        menuRoot->addComponent(midiNoteItem.get());
    }
    else {
        midiNoteItem->changeNode(displayedNode);
    }

    if(nodeDataField == nullptr){
        nodeDataField = std::make_unique<DynamicField>(displayedNodeData->nodeData,false);
        addAndMakeVisible(nodeDataField.get());
    }
    else {
        nodeDataField->setValueTree(displayedNodeData->nodeData);
    }
  
    nameEditor.bindEditor(displayedNode->getNodeData()->nodeData, NodeData::nameID);

    repaint();
    resized();
}


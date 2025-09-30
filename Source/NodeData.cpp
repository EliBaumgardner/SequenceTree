#include "NodeCanvas.h"
#include "Node.h"
#include "NodeData.h"


const juce::Identifier NodeData::nameID { "name" };
const juce::Identifier NodeData::channelID { "channel" };

const juce::Identifier NodeData::xID { "x" };
const juce::Identifier NodeData::yID { "y" };
const juce::Identifier NodeData::radiusID { "radius" };
const juce::Identifier NodeData::countID { "count" };
const juce::Identifier NodeData::countLimitID { "countLimit" };
const juce::Identifier NodeData::colourID { "colour" };

const juce::Identifier NodeData::pitchID { "pitch" };
const juce::Identifier NodeData::velocityID { "velocity" };
const juce::Identifier NodeData::durationID { "duration" };

const juce::Identifier NodeData::ccValueID { "ccValue" };
const juce::Identifier NodeData::ccNumberID { "ccNumber" };



NodeData::NodeData() : nodeData("NodeData"), midiNoteData("MidiNoteData"), midiCCData("MidiCCData"){
    
    nodeData.setProperty(nameID,"node",nullptr);
    nodeData.setProperty(xID, 0, nullptr);
    nodeData.setProperty(yID,0,nullptr);
    nodeData.setProperty(radiusID,0, nullptr);
    nodeData.setProperty(countID,0,nullptr);
    nodeData.setProperty(countLimitID,1,nullptr);
    nodeData.setProperty(colourID,0,nullptr);
    
    midiNoteData.setProperty(channelID,0,nullptr);
    midiNoteData.setProperty(pitchID,0,nullptr);
    midiNoteData.setProperty(velocityID,0,nullptr);
    midiNoteData.setProperty(durationID,0,nullptr);
    
    midiCCData.setProperty(nameID,0,nullptr);
    midiCCData.setProperty(channelID,0,nullptr);
    midiCCData.setProperty(ccNumberID,0,nullptr);
    midiCCData.setProperty(ccValueID,0,nullptr);
    

}

NodeData::~NodeData() {
    for (auto& val : propertyValues)
        val = juce::Value(); // unbind
    propertyValues.clear();
}

void NodeData::addChild(Node* child){
    
    children.add(child);
}

void NodeData::removeChild(Node* child){
    
    children.removeFirstMatchingValue(child);
}

void NodeData::bindEditor(juce::TextEditor& editor, const juce::Identifier propertyID, juce::String treeType){
    
    std::cout<<"binding"<<std::endl;
    
    juce::Value propertyValue;
    propertyValues.add(propertyValue);
    juce::ValueTree* selectedTree = nullptr;
    
    if(treeType == "NodeData"){
        selectedTree = &nodeData;
    }
    else if(treeType == "MidiNoteData"){
        selectedTree = &midiNoteData;
    }
    else if(treeType == "MidiCCData"){
        selectedTree = &midiCCData;
    }
    
    propertyValue.referTo(selectedTree->getPropertyAsValue(propertyID,nullptr));
    editor.getTextValue().referTo(propertyValue);
}

void NodeData::createTree(juce::String type){
    
    if(type == "MidiNoteData"){
        juce::ValueTree tree("MidiNoteData");
        tree.setProperty(channelID,0,nullptr);
        tree.setProperty(pitchID,63,nullptr);
        tree.setProperty(velocityID,63,nullptr);
        tree.setProperty(durationID,1000,nullptr);
        midiNotes.add(tree);
        
        tree.addListener(&listener);
        
        listener.onChanged = [this](){
            std::cout<<"note edited"<<std::endl;
            node->nodeCanvas->makeRTGraph(node->nodeCanvas->root);
        };
    }
    else {
        juce::ValueTree tree("MidiCCData");
        tree.setProperty(nameID,0,nullptr);
        tree.setProperty(channelID,0,nullptr);
        tree.setProperty(ccNumberID,0,nullptr);
        tree.setProperty(ccValueID,0,nullptr);
        midiCCs.add(tree);
    }
    
    node->nodeCanvas->makeRTGraph(node->nodeCanvas->root);
}

void NodeData::setNode(Node* node){
    this->node = node;
};

void NodeData::pushData(){
    
}

void NodeData::setContext(PluginContext* context){
    
}

/*
  ==============================================================================

    NodeData.h
    Created: 8 May 2025 9:59:10am
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once

#include "../Util/PluginModules.h"
#include "../Util/PluginContext.h"

struct PluginContext;

class NodeCanvas;

class Node;

class NodeData{
    
    
    public:

        NodeData();
    
        ~NodeData();
            
        juce::ValueTree nodeData;
        juce::ValueTree midiNoteData;
        juce::ValueTree midiCCData;
        
        static const juce::Identifier nameID;
        static const juce::Identifier channelID;
        
        static const juce::Identifier xID;
        static const juce::Identifier yID;
        static const juce::Identifier radiusID;
        static const juce::Identifier countID;
        static const juce::Identifier countLimitID;
        static const juce::Identifier colourID;
        
        static const juce::Identifier pitchID;
        static const juce::Identifier velocityID;
        static const juce::Identifier durationID;

        static const juce::Identifier ccValueID;
        static const juce::Identifier ccNumberID;
        
        juce::Array<juce::ValueTree> midiNotes;
        juce::Array<juce::ValueTree> midiCCs;
        juce::Array<juce::Value> propertyValues;

        juce::Array<Node*> children;
        juce::Array<Node*> connectors;
    
        Node* node;
    
        void setNode(Node* node);
        void addChild(Node* child);
        void removeChild(Node* child);

        void addConnector(Node* connector);
        void removeConnector(Node* connector);
    
        void bindEditor(juce::TextEditor& editor, const juce::Identifier propertyID,juce::String treeType);
        void createTree(juce::String type);

        private:
    
        PluginContext* context = nullptr;

        class ValueListener : public juce::ValueTree::Listener {
            
            public:
            
            std::function<void()> onChanged;
            
            void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override {
                
                if(property == juce::Identifier("pitch") || property == juce::Identifier("velocity") || property == juce::Identifier("duration")){
                    if(onChanged) onChanged();
                }
            }
        };
    
        ValueListener listener;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NodeData)
};

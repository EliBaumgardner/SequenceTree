#include "DynamicField.h"
#include "MenuItem.h"
#include "ComponentContext.h"
#include "NodeCanvas.h"
#include "Node.h"

DynamicField::DynamicField(juce::ValueTree tree, bool withMinusButton) : withMinusButton(withMinusButton), tree(tree) {
    if (tree.hasType("NodeData")) {
        
        
        createEditor("X",tree,NodeData::xID,true);
        createEditor("Y", tree,NodeData::yID,true);
        createEditor("R", tree,NodeData::radiusID,true);

    }
    else if (tree.hasType("MidiNoteData")) {
        
        createEditor("pitch",tree,NodeData::pitchID,false);
        createEditor("vel",tree,NodeData::velocityID,false);
        createEditor("dur", tree, NodeData::durationID,false);
        
    }
    else if (tree.hasType("MidiCCData")) {
        
        createEditor("name",tree,NodeData::nameID,true);
        createEditor("chan#",tree,NodeData::channelID,true);
        createEditor("cc#",tree,NodeData::ccNumberID,true);
        createEditor("ccValue", tree, NodeData::ccValueID,true);
    }
    
    if(withMinusButton){
        addAndMakeVisible(&minusButton);
        
        minusButton.onClick = [this](){
            
            auto* parent = getParentComponent();
            if(auto* component = dynamic_cast<MenuItem*>(parent)){
                component->removeComponent(this);
                ComponentContext::canvas->makeRTGraph(ComponentContext::canvas->root);
            }
        };
    }
}

DynamicField::DynamicField(DynamicLabel label){
    if(label == DynamicLabel::MidiNote){
        
        createLabel("pitch");
        createLabel("vel");
        createLabel("dur");
    }
}

DynamicField::~DynamicField() {

}

void DynamicField::paint(juce::Graphics& g) {
    g.setColour(juce::Colours::black);
}

void DynamicField::resized() {
    int i = 0;
    for (EditorInfo* editorInfo : editorInfos) {
        DynamicEditor* editor = editorInfo->editor.get();
        layout(editor, i);
        editor->refit();
        ++i;
    }
    if(withMinusButton){
        layout(&minusButton, i);
    }
}

void DynamicField::layout(Component* component, int index) {
    float width = (float)getWidth();
    float height = (float)getHeight();

    width /= tree.hasType("NodeData") ? 4.5f : 4.5f;
    float spacing = width / 32.0f;
    float xPos = (width + spacing) * (float)index;

    component->setBounds(xPos, 0, width, height);
}

void DynamicField::setValueTree(juce::ValueTree tree){
    this->tree = tree;
    
    for(EditorInfo* editorInfo : editorInfos){
        editorInfo->editor.get()->bindEditor(tree,editorInfo->propertyID);
    }
    
}

void DynamicField::createEditor(std::string name,juce::ValueTree tree, juce::Identifier propertyID, bool hint){
    
    EditorInfo* info = new EditorInfo;
    info->propertyID = propertyID;
    info->editor = std::make_unique<DynamicEditor>();
    
    if(hint)
        info->editor->displayHint(true,name);
    
    info->editor->bindEditor(tree, propertyID);
    addAndMakeVisible(info->editor.get());

    editorInfos.add(info);
}

void DynamicField::createLabel(std::string name){
    
    EditorInfo* info = new EditorInfo;
    info->editor = std::make_unique<DynamicEditor>();
    info->editor->setLabelText(name);
    info->editor->setEditable(false);
    addAndMakeVisible(info->editor.get());
    editorInfos.add(info);
}

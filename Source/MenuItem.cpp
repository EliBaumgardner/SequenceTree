#include "MenuItem.h"

static constexpr int kRowHeight   = 30;  // Fixed height (in pixels) for every menu‐item row
static constexpr int kRowSpacing  =  2;  // Vertical spacing (in pixels) between rows

MenuItem::MenuItem(juce::String label,ItemType type,Node* node) : selectedNode(node), type(type)
{
    
    menuLabel = std::make_unique<DynamicEditor>();
    menuLabel->setEditable(false);
    menuLabel->setText(label, juce::dontSendNotification);
    
    addAndMakeVisible(menuLabel.get());
    addAndMakeVisible(arrowButton);
    addComponent(addButton.get());
    
    changeNode(selectedNode);
    
    arrowButton.onToggle = [this](){
        
        open = arrowButton.isOpen;
        resized();
        if(auto* parent = getParentComponent())
            parent->resized();
    };
    
    if(this->type == ItemType::MidiNoteItem){
        
        auto labelTemp = std::make_unique<DynamicField>(DynamicField::DynamicLabel::MidiNote);
        fieldLabel = labelTemp.get();
        addComponent(fieldLabel);
        fieldStorage.push_back(std::move(labelTemp));
        
        resized();
        if(auto* parent = getParentComponent())
            parent->resized();
    }
    
    addButton.get()->onClick = [this](){
      
        std::unique_ptr<DynamicField> field;
        if(this->type == ItemType::MidiCCItem){
            //selectedNode->getNodeData()->midiCCData;
            selectedNode->getNodeData()->createTree("MidiCCData");
            juce::ValueTree tree = selectedNode->getNodeData()->midiCCs.getLast();
            field = std::make_unique<DynamicField>(tree,true);
        }
        else {
            selectedNode->getNodeData()->createTree("MidiNoteData");
            juce::ValueTree tree = selectedNode->getNodeData()->midiNotes.getLast();
            field = std::make_unique<DynamicField>(tree,true);
        }
        
        addComponent(field.get());
        
        fieldStorage.push_back(std::move(field));
        
        resized();
        if(auto* parent = getParentComponent())
            parent->resized();
    };
    
}

MenuItem::MenuItem()
{
    isRoot = true;
}

MenuItem::~MenuItem() {

}

void MenuItem::paint(juce::Graphics& g)
{

}

void MenuItem::resized()
{
    // STEP A: Lay out this MenuItem’s own “label + arrow” row at a fixed height
    const int width  = getWidth();
    const int rowH   = kRowHeight;
    const int gap    = kRowSpacing;

    if (! isRoot)                 {
        // Place the label on the left 75% of this width, at (0, 0), with height = rowH
        menuLabel->setBounds(0, 0, static_cast<int>(width * 0.75f), rowH);
        menuLabel->refit();

        // Place the arrow button on the right 25% of this width, at (width*0.75, 0), with height = rowH
        arrowButton.setBounds(static_cast<int>(width * 0.75f), 0,
                               static_cast<int>(width * 0.25f), rowH);

        // Store whether this submenu should be open or closed based on the arrow’s state
        open = arrowButton.isOpen;

    }

    int yOffset = rowH + gap;
    if (open)
    {
        if(!fields[selectedNode].isEmpty()){
            for (int i = fields[selectedNode].size(); --i >= 0;)
            {
                juce::Component* component = fields[selectedNode].getReference(i);
                component->setVisible(true);
                
                if (auto* submenu = dynamic_cast<MenuItem*>(component))
                {
                   
                    const int subtreeHeight = submenu->getMenuItemHeight();
                    submenu->setBounds(0, yOffset, width, subtreeHeight);
                    submenu->resized();
                    yOffset += subtreeHeight + gap;
                }
                else
                {
                    if(component == addButton.get()){
                        component->setBounds(0, yOffset, width*0.85, rowH);
                    }
                    else{
                        component->setBounds(0, yOffset, width, rowH);
                        yOffset += rowH + gap;
                    }
                }
            }
            
        }
    }
    else
    {
        for (auto& component : fields[selectedNode])
            component->setVisible(false);
        
    }
}

void MenuItem::addComponent(juce::Component* component)
{
    addChildComponent(component);
    
    //fields[selectedNode].remove(fieldLabel.get());
    fields[selectedNode].add(component);
    
    if(type == ItemType::MidiNoteItem){
        if(fieldLabel != nullptr){
            fields[selectedNode].removeFirstMatchingValue(fieldLabel);
            fields[selectedNode].add(fieldLabel);
        }
    }
    
    resized();
}

void MenuItem::removeComponent(juce::Component* component)
{
    
    if(auto* field = dynamic_cast<DynamicField*>(component)){
        if(type == ItemType::MidiNoteItem){
            selectedNode->nodeData.midiNotes.removeFirstMatchingValue(field->tree);
        }
        else{
            selectedNode->nodeData.midiCCs.removeFirstMatchingValue(field->tree);
        }
    }
        
 
    fields[selectedNode].removeFirstMatchingValue(component);
    removeChildComponent(component);
    
    resized();
    if(auto* parent = getParentComponent()){
        parent->resized();
    }
}

// Returns the total pixel height needed to display this MenuItem (one row for itself + rows for any open descendants)
int MenuItem::getMenuItemHeight()
{
    
    // Count 1 row for this MenuItem’s own “label+arrow” row
    int totalRows = 1;

    if (open && ! fields[selectedNode].isEmpty())
    {
        // If open, each child contributes its own rows
        for (auto& component : fields[selectedNode])
        {
            if (auto* submenu = dynamic_cast<MenuItem*>(component))
            {
                // A submenu: ask it how many total rows it needs (including its descendants)
                totalRows += submenu->getMenuItemHeight() / kRowHeight;
            }
            else
            {
                // A “leaf” component: it occupies exactly one row
                totalRows += 1;
            }
        }
    }

    // Convert row count into pixels: each row is kRowHeight + kRowSpacing (except the final row doesn’t need extra gap)
    const int pixelHeight = totalRows * kRowHeight
                          + (totalRows - 1) * kRowSpacing;
    return pixelHeight;
}

bool MenuItem::getOpenState(){
    return open;
}

void MenuItem::setOpenState(bool state){
    open = state;
    arrowButton.isOpen = open;
    resized();
    if(auto* parent = getParentComponent())
        parent->resized();
}

void MenuItem::changeNode(Node* node){
    
    for(int i = fields[selectedNode].size(); --i >= 0;){
        juce::Component* component = fields[selectedNode].getReference(i);
        if(component != fieldLabel){
            removeChildComponent(component);
        }
    }
    
    fields[selectedNode].removeFirstMatchingValue(addButton.get());
    
    selectedNode = node;
    
    
    fields[selectedNode].insert(0,addButton.get());
    
    for(juce::Component* component : fields[selectedNode]){
        addChildComponent(component);
    }
    
    resized();
    if(auto* parent = getParentComponent())
        parent->resized();
}

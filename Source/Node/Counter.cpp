//
// Created by Eli Baumgardner on 10/6/25.
//

#include "Counter.h"

Counter::Counter() {

    editor = std::make_unique<NodeBox>(this);

    addAndMakeVisible(upButton);
    addAndMakeVisible(downButton);
    addAndMakeVisible(editor.get());

    nodeLogic.setNode(this);
    nodeData.setNode(this);
    //nodeController = std::make_unique<ObjectController>(this);
    //this->addMouseListener(nodeController.get(), true);

    editor.get()->setColour(juce::TextEditor::textColourId, juce::Colours::white);
    editor.get()->bindEditor(nodeData.nodeData,"countLimit");

    upButton.onChanged = [this](){
        double value = editor.get()->bindValue.toString().getDoubleValue();
        value += 1;
        editor.get()->bindValue.setValue(value);
        editor.get()->formatDisplay(editor.get()->mode);
    };

    downButton.onChanged = [this](){
        double value = editor.get()->bindValue.toString().getDoubleValue();
        value -= 1;
        editor.get()->bindValue.setValue(value);
        editor.get()->formatDisplay(editor.get()->mode);
    };
}

void Counter::resized() {
    auto editorArea = getLocalBounds().reduced(10.0f);

    upButton.setBounds(editorArea.removeFromTop(4.0f));
    downButton.setBounds(editorArea.removeFromBottom(4.0f));

    editor.get()->setBounds(editorArea);
    editor.get()->setJustification(juce::Justification::centred);
    //editor.refit();

    nodeData.nodeData.setProperty("x",getX(),nullptr);
    nodeData.nodeData.setProperty("y",getY(),nullptr);
    nodeData.nodeData.setProperty("radius", getWidth()/2,nullptr);
}

void Counter::paint(juce::Graphics& g) {

    auto bounds = getLocalBounds().toFloat();
    auto squareBorder = bounds.reduced(2.5f);
    auto squareSelect = bounds.reduced(0.5f);
    auto squareHover = bounds.reduced(4.5f);
    auto squareFill = bounds.reduced(5.5f);

    g.setColour(juce::Colours::black);
    g.drawRect(squareBorder, 1.0f);


    g.setColour(isHighlighted ? nodeColour.darker() : nodeColour);
    g.fillRect(squareFill);

    if (isHovered)
        g.drawRect(squareHover, 2.0f);

    if (isSelected)
    {
        juce::Path dottedPath;
        dottedPath.addRectangle(squareSelect);

        juce::PathStrokeType stroke(0.325f);

        float dashLengths[] = { 2.935f, 2.935f };
        stroke.createDashedStroke(dottedPath, dottedPath, dashLengths, 2);

        g.setColour(juce::Colours::black);
        g.strokePath(dottedPath, stroke);
    }
}

void Counter::setDisplayMode(NodeBox::DisplayMode mode){


    juce::ValueTree midiTree("MidiNoteData");

    midiTree.setProperty("pitch",60,nullptr);
    midiTree.setProperty("velocity", 60, nullptr);
    midiTree.setProperty("duration", 500, nullptr);

    if(nodeData.midiNotes.isEmpty()){
        nodeData.midiNotes.add(midiTree);
    }
    else if(nodeData.midiNotes.size() == 1) {
        midiTree = nodeData.midiNotes.getLast();
    }

    if(mode == NodeBox::DisplayMode::Pitch){
        // editor.get()->bindEditor(midiTree, "pitch");
        // editor.get()->formatDisplay(NodeBox::DisplayMode::Pitch);
    }

    if(mode == NodeBox::DisplayMode::Velocity){
        // editor.get()->bindEditor(midiTree, "velocity");
        // editor.get()->formatDisplay(NodeBox::DisplayMode::Velocity);
    }

    if(mode == NodeBox::DisplayMode::Duration){
        editor.get()->bindEditor(midiTree, "duration");
        editor.get()->formatDisplay(NodeBox::DisplayMode::Duration);
    }

    if(mode == NodeBox::DisplayMode::CountLimit){
        editor.get()->bindEditor(nodeData.nodeData,"countLimit");
        editor.get()->formatDisplay(NodeBox::DisplayMode::CountLimit);
    }
}

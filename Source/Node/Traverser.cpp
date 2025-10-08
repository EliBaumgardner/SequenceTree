//
// Created by Eli Baumgardner on 10/7/25.
//

#include "Traverser.h"

Traverser::Traverser(NodeCanvas* canvas) : Node(canvas) {

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

void Traverser::resized() {
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

void Traverser::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Outer triangle (border)
    juce::Point<float> p1(bounds.getCentreX(), bounds.getY());
    juce::Point<float> p2(bounds.getRight(), bounds.getBottom());
    juce::Point<float> p3(bounds.getX(), bounds.getBottom());

    juce::Path triangleBorder;
    triangleBorder.addTriangle(p1, p2, p3);

    // Inner triangle (fill) - scaled uniformly inside border
    auto scaleFactor = 0.85f; // 85% of outer triangle
    auto center = bounds.getCentre();

    auto scalePoint = [&](juce::Point<float> p) {
        return center + (p - center) * scaleFactor;
    };

    juce::Path triangleFill;
    triangleFill.addTriangle(scalePoint(p1), scalePoint(p2), scalePoint(p3));

    // Hover triangle
    juce::Path triangleHover;
    triangleHover.addTriangle(scalePoint(p1) + juce::Point<float>(0,0), // same scaled points
                              scalePoint(p2),
                              scalePoint(p3));

    // Selection triangle
    juce::Path triangleSelect;
    triangleSelect.addTriangle(scalePoint(p1), scalePoint(p2), scalePoint(p3));

    // Draw border
    g.setColour(juce::Colours::black);
    g.strokePath(triangleBorder, juce::PathStrokeType(1.0f));

    // Fill
    g.setColour(isHighlighted ? nodeColour.darker() : nodeColour);
    g.fillPath(triangleFill);

    // Hover
    if (isHovered)
        g.strokePath(triangleHover, juce::PathStrokeType(2.0f));

    // Selection
    if (isSelected)
    {
        juce::PathStrokeType stroke(0.325f);
        float dashLengths[] = { 2.935f, 2.935f };
        stroke.createDashedStroke(triangleSelect, triangleSelect, dashLengths, 2);

        g.setColour(juce::Colours::black);
        g.strokePath(triangleSelect, stroke);
    }
}

void Traverser::setDisplayMode(NodeBox::DisplayMode mode){


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

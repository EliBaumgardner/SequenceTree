//
// Created by Eli Baumgardner on 10/7/25.
//

#include "Connector.h"

Connector::Connector() {
    nodeType = NodeType::Connector;

    // std::cout<<nodeId<<std::endl;
    // nodeData.setProperty("nodeType","Connector", "NodeData");
    // std::cout<<"relay intiliazed"<<std::endl;
    // editor = std::make_unique<NodeTextEditor>(this);
    //
    // addAndMakeVisible(upButton);
    // addAndMakeVisible(downButton);
    // addAndMakeVisible(editor.get());
    //
    // nodeData.setNode(this);
    // //nodeController = std::make_unique<ObjectController>(this);
    // //this->addMouseListener(nodeController.get(), true);
    //
    // editor.get()->setColour(juce::TextEditor::textColourId, juce::Colours::white);
    // editor.get()->bindEditor(nodeData.nodeData,"countLimit");
    //
    // upButton.onChanged = [this](){
    //     double value = editor.get()->bindValue.toString().getDoubleValue();
    //     value += 1;
    //     editor.get()->bindValue.setValue(value);
    //     editor.get()->formatDisplay(editor.get()->mode);
    // };
    //
    // downButton.onChanged = [this](){
    //     double value = editor.get()->bindValue.toString().getDoubleValue();
    //     value -= 1;
    //     editor.get()->bindValue.setValue(value);
    //     editor.get()->formatDisplay(editor.get()->mode);
    // };
}

void Connector::resized() {
    auto editorArea = getLocalBounds().reduced(10.0f);

    upButton.setBounds(editorArea.removeFromTop(4.0f));
    downButton.setBounds(editorArea.removeFromBottom(4.0f));

    nodeTextEditor.get()->setBounds(editorArea);
    nodeTextEditor.get()->setJustification(juce::Justification::centred);
    //editor.refit();
}

void Connector::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    juce::Graphics::ScopedSaveState savedState(g);
    g.addTransform(juce::AffineTransform::rotation(
        incomingAngle + juce::MathConstants<float>::halfPi,
        bounds.getCentreX(),
        bounds.getCentreY()
    ));

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


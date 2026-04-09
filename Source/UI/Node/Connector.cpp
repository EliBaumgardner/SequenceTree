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
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();

    // Use circumscribed circle so all rotation angles stay within bounds.
    // Shrink slightly so the stroke doesn't clip at the component edge.
    float r = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f - 2.0f;

    // Build equilateral triangle by placing vertices on the circumscribed circle,
    // rotated so the tip points in the direction of incomingAngle.
    float baseAngle = incomingAngle;
    constexpr float twoPiOver3 = juce::MathConstants<float>::twoPi / 3.0f;

    juce::Point<float> p1 { cx + r * std::cos(baseAngle),
                             cy + r * std::sin(baseAngle) };
    juce::Point<float> p2 { cx + r * std::cos(baseAngle + twoPiOver3),
                             cy + r * std::sin(baseAngle + twoPiOver3) };
    juce::Point<float> p3 { cx + r * std::cos(baseAngle + 2.0f * twoPiOver3),
                             cy + r * std::sin(baseAngle + 2.0f * twoPiOver3) };

    juce::Path triangle;
    triangle.addTriangle(p1, p2, p3);

    // Fill
    g.setColour(isHighlighted ? nodeColour.darker() : nodeColour);
    g.fillPath(triangle);

    // Outline — drawn after fill so it sits exactly on the triangle edge
    g.setColour(juce::Colours::black);
    g.strokePath(triangle, juce::PathStrokeType(1.5f,
        juce::PathStrokeType::mitered,
        juce::PathStrokeType::rounded));

    // Hover
    if (isHovered)
    {
        g.setColour(juce::Colours::white.withAlpha(0.25f));
        g.strokePath(triangle, juce::PathStrokeType(3.0f,
            juce::PathStrokeType::mitered,
            juce::PathStrokeType::rounded));
    }

    // Selection
    if (isSelected)
    {
        juce::Path dashedPath;
        juce::PathStrokeType stroke(0.325f);
        float dashLengths[] = { 2.935f, 2.935f };
        stroke.createDashedStroke(dashedPath, triangle, dashLengths, 2);
        g.setColour(juce::Colours::black);
        g.fillPath(dashedPath);
    }
}


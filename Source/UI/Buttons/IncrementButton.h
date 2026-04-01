//
// Created by Eli Baumgardner on 4/1/26.
//

#ifndef SEQUENCETREE_INCREMENTBUTTON_H
#define SEQUENCETREE_INCREMENTBUTTON_H

class IncrementButton : public juce::Component {

public:

    bool increment;
    std::function<void()> onChanged;

    IncrementButton(bool increment) : increment(increment) {

    }

    void paint(juce::Graphics& g) override {

        auto bounds = getLocalBounds().toFloat();
        auto w = bounds.getWidth();
        auto h = bounds.getHeight();

        if(increment == true){
            juce::Path path;

            path.startNewSubPath(0, h);
            path.lineTo(w / 2, 0);
            path.lineTo(w, h);
            path.closeSubPath();

            g.setColour(juce::Colours::white);
            g.fillPath(path);

            g.setColour(juce::Colours::black);
            g.strokePath(path, juce::PathStrokeType(1.0f));
        }
        else {
            juce::Path path;

            path.startNewSubPath(0,0);
            path.lineTo(w/2,h);
            path.lineTo(w,0);
            path.closeSubPath();

            g.setColour(juce::Colours::white);
            g.fillPath(path);

            g.setColour(juce::Colours::black);
            g.strokePath(path, juce::PathStrokeType(1.0f));
        }
    }

    void mouseDown(const juce::MouseEvent& e) override {

        onChanged();
    }
};
#endif //SEQUENCETREE_INCREMENTBUTTON_H
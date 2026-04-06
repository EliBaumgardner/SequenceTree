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

        auto bounds = getLocalBounds().toFloat().reduced(4.0f, 1.0f);
        auto w = bounds.getWidth();
        auto h = bounds.getHeight();
        auto x = bounds.getX();
        auto y = bounds.getY();

        juce::Path path;
        if (increment)
        {
            path.startNewSubPath(x,       y + h);
            path.lineTo          (x + w * 0.5f, y);
            path.lineTo          (x + w,  y + h);
        }
        else
        {
            path.startNewSubPath(x,       y);
            path.lineTo          (x + w * 0.5f, y + h);
            path.lineTo          (x + w,  y);
        }
        path.closeSubPath();

        g.setColour(juce::Colours::black);
        g.fillPath(path);
    }

    void mouseDown(const juce::MouseEvent& e) override {

        onChanged();
    }
};
#endif //SEQUENCETREE_INCREMENTBUTTON_H
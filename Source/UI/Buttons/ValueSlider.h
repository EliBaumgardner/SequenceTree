//
// Created by Eli Baumgardner on 6/11/26.
//

#ifndef SEQUENCETREE_VALUESLIDER_H
#define SEQUENCETREE_VALUESLIDER_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Util/ApplicationContext.h"


class ValueSliderHandle : public juce::Component {
public:

    ValueSliderHandle() {
        setInterceptsMouseClicks(false,false);
    }

    void paint(juce::Graphics &g) override {
        auto bounds = getLocalBounds();

        g.setColour(juce::Colours::black);
        g.fillRect(bounds);
    }

};

class ValueSlider : public juce::Component {

    public:

    bool intersectsHandle = false;

    std::unique_ptr<ValueSliderHandle> handle;
    std::function<void()> valueChanged;

    juce::Rectangle<int> slider;

    juce::Value boundValue;

    ValueSlider() {

        handle = std::make_unique<ValueSliderHandle>();

        addAndMakeVisible(handle.get());

    }

    void paint(juce::Graphics &g) override {
        auto bounds = getLocalBounds();

        g.setColour(juce::Colours::black);
        g.drawRect(bounds, 1);

        g.setColour(juce::Colours::grey);
        g.fillRect(slider);
    }

    void resized() override {
        auto bounds = getLocalBounds();
        int handleWidth = bounds.getWidth() * 0.1f;

        float value = juce::jlimit(0.0f,1.0f, (float)boundValue.getValue());
        int handleX = (int)(handleWidth/2 + value * (bounds.getWidth() - handleWidth));

        handle->setBounds(handleX - handleWidth/2, bounds.getY(), handleWidth, bounds.getHeight());
        slider.setBounds(0,0,handleX,bounds.getHeight());
    }

    void bindValue(juce::Value& boundValue) {

        this->boundValue.referTo(boundValue);
    }

    void mouseDrag(const juce::MouseEvent& e) override {

        if (!intersectsHandle) {
            intersectsHandle = handle->getBounds().contains(e.getPosition());
        }

        if (intersectsHandle) {
            DBG("intersects handle");
            float width = (float)getWidth();
            float handleWidth = width * 0.1f;
            float value = juce::jlimit(0.0f, 1.0f,
                (e.getPosition().getX() - handleWidth/2.0f) / (width - handleWidth));
            boundValue.setValue(value);
            resized();
            repaint();
            valueChanged();
        }
    }

    void mouseUp(const juce::MouseEvent& e) override {
        intersectsHandle = false;
    }
};

#endif SEQUENCETREE_VALUESLIDER_H
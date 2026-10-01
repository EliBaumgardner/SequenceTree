//
// Created by Eli Baumgardner on 6/11/26.
//

#ifndef SEQUENCETREE_VALUESLIDER_H
#define SEQUENCETREE_VALUESLIDER_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Util/ApplicationContext.h"
#include "../Theme/CustomLookAndFeel.h"


class ValueSliderHandle : public juce::Component {
public:

    ValueSliderHandle() {
        setInterceptsMouseClicks(false,false);
    }

    void paint(juce::Graphics &g) override {
        CustomLookAndFeel::get(*this).drawValueSliderHandle(g, getLocalBounds().toFloat());
    }

};

class ValueSlider : public juce::Component, public juce::SettableTooltipClient {

    public:

    float handleWidthRatio = 0.1f;

    std::unique_ptr<ValueSliderHandle> handle;
    std::function<void()> valueChanged;

    juce::Rectangle<int> slider;

    juce::Value boundValue;

    ValueSlider() {

        handle = std::make_unique<ValueSliderHandle>();

        addAndMakeVisible(handle.get());

    }

    void paint(juce::Graphics &g) override {
        CustomLookAndFeel::get(*this).drawValueSlider(g, *this);
    }

    void resized() override {
        const auto bounds = getLocalBounds();
        const int handleWidth = bounds.getWidth() * handleWidthRatio;

        const float lineWidth = Theme::valueSliderHandleLineWidth;

        const float value = juce::jlimit(0.0f,1.0f, static_cast<float>(boundValue.getValue()));
        const int handleX = static_cast<int>(lineWidth/2 + value * (bounds.getWidth() - lineWidth));

        handle->setBounds(handleX - handleWidth/2, bounds.getY(), handleWidth, bounds.getHeight());
        slider.setBounds(0,0,handleX,bounds.getHeight());
    }

    void bindValue(const juce::Value& boundValue) {

        this->boundValue.referTo(boundValue);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        const float width = static_cast<float>(getWidth());
        const float lineWidth = Theme::valueSliderHandleLineWidth;
        const float value = juce::jlimit(0.0f, 1.0f,
            (e.getPosition().getX() - lineWidth/2.0f) / (width - lineWidth));
        boundValue.setValue(value);
        resized();
        repaint();

        if (valueChanged) {
            valueChanged();
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        mouseDown(e);
    }
};

#endif //SEQUENCETREE_VALUESLIDER_H

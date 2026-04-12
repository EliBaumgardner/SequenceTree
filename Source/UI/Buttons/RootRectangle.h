//
// Created by Eli Baumgardner on 4/11/26.
//

#ifndef SEQUENCETREE_ROOTARROW_H
#define SEQUENCETREE_ROOTARROW_H

#include <juce_gui_basics/juce_gui_basics.h>

class RootRectangle : public juce::Component {

public:
    RootRectangle();

    void resized() override;
    void paint(juce::Graphics& g) override;


};


#endif //SEQUENCETREE_ROOTARROW_H
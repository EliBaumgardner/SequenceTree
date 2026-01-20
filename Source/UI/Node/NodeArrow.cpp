/*
  ==============================================================================

    NodeArrow.cpp
    Created: 12 Jun 2025 12:45:57am
    Author:  Eli Baimgardner

  ==============================================================================
*/
#include "NodeArrow.h"
#include "Node.h"

NodeArrow::NodeArrow(Node* startNode, Node* endNode) : startNode(startNode), endNode(endNode), updater(this),

  animator (animator = juce::ValueAnimatorBuilder{}

  .withValueChangedCallback([this](float newValue){
    wobblePhase = newValue * 2.0f * juce::float_Pi;  // convert to 0 -> 2π
      this->repaint();
    }).withDurationMs(2000.0).build()
  )

{

  setLookAndFeel(ComponentContext::lookAndFeel);
  // updater.addAnimator(animator);
  //
  // animator.start();
}

void NodeArrow::paint(juce::Graphics &g)
{
  if (auto* customLookAndFeel = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel())) { customLookAndFeel->drawNodeArrow(g,*this); }
}


void NodeArrow::animate()
{

}
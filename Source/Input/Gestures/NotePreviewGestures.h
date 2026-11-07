#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../Util/NodeInfo.h"

class RTGraphBuilder;
class TraversalSession;

class NotePreviewGestures
{
public:

    NotePreviewGestures(TraversalSession& traversalSession, RTGraphBuilder& rtGraphBuilder);

    void respondToClick(juce::Component* clicked);
    void stopPreview ();
    void toggleRepeat ();

    bool repeat = false;

    int traversalId = 1;

    juce::Value count { NodeInfo::minimumCountLimit };

private:

    TraversalSession& traversalSession;
    RTGraphBuilder&   rtGraphBuilder;
};

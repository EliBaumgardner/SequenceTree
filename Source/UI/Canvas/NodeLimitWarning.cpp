#include "NodeLimitWarning.h"
#include "../../Graph/GraphState.h"
#include "../../Audio/Traversal/NodeStateTable.h"
#include <ArsNovaUI/Menus/Dialog.h>

NodeLimitWarning::NodeLimitWarning(GraphState& graphState, juce::Component& target)
    : graphState(graphState),
      target    (target)
{
    graphState.nodeMap.addListener(this);

    triggerAsyncUpdate();
}

NodeLimitWarning::~NodeLimitWarning()
{
    graphState.nodeMap.removeListener(this);
}

void NodeLimitWarning::handleAsyncUpdate()
{
    const juce::String trackedNodeText = juce::String(NodeStateTable::maxNodeIds);

    int playableNodeCount = 0;

    ArsNovaUI::Dialog dialog;

    for (const juce::ValueTree& node : graphState.nodeMap) {
        if (GraphState::kindOf(node) != NodeInfo::Kind::Encapsulator) {
            ++playableNodeCount;
        }
    }

    const juce::String nodeCountText = juce::String(playableNodeCount);

    const bool isOverLimit = playableNodeCount > NodeStateTable::maxNodeIds;

    if (! isOverLimit) {
        hasWarned = false;

        return;
    }

    if (hasWarned) {
        return;
    }

    hasWarned    = true;
    dialog.title = "Too many nodes";

    dialog.message = "This patch has " + nodeCountText + " nodes, and playback keeps loop counts and limits for "
                   + trackedNodeText + ". Past that, some counts can stop advancing, and the branches that wait on them will not play.";

    dialog.addButton("OK", nullptr);
    dialog.show(target);
}

void NodeLimitWarning::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    if (parent == graphState.nodeMap) {
        triggerAsyncUpdate();
    }
}

void NodeLimitWarning::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int childIndex)
{
    if (parent == graphState.nodeMap) {
        triggerAsyncUpdate();
    }
}

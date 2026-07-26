#pragma once

#include "../Util/PluginModules.h"
#include "../Util/NodeInfo.h"

class ValueTreeState;

enum class NodeCreationMode { Node, Modulator, TraversalFlag };

class NodeCreationDispatcher
{
public:

    static juce::ValueTree create(NodeCreationMode         mode,
                                  ValueTreeState&          state,
                                  int                      parentNodeId,
                                  const juce::Identifier&  parentType,
                                  bool                     makeAlternative,
                                  const NodePosition&      position,
                                  juce::UndoManager*       undoManager);
};

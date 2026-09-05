#pragma once

#include "../Util/PluginModules.h"
#include "../Util/NodeInfo.h"

class GraphState;

enum class NodeCreationMode { Node, Modulator, TraversalFlag };

class NodeCreationDispatcher
{
public:

    static juce::ValueTree create(NodeCreationMode         mode,
                                  GraphState&          state,
                                  int                      parentNodeId,
                                  const juce::Identifier&  parentType,
                                  bool                     makeAlternative,
                                  const NodePosition&      position,
                                  juce::UndoManager*       undoManager);
};

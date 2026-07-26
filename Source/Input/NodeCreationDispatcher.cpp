#include "NodeCreationDispatcher.h"
#include "../UI/Node/NodeFactory.h"
#include "../Graph/ValueTreeIdentifiers.h"
#include "../Graph/ValueTreeState.h"

juce::ValueTree NodeCreationDispatcher::create(NodeCreationMode         mode,
                                               ValueTreeState&          state,
                                               int                      parentNodeId,
                                               const juce::Identifier&  parentType,
                                               bool                     makeAlternative,
                                               const NodePosition&      position,
                                               juce::UndoManager*       undoManager)
{
    switch (mode)
    {
        case NodeCreationMode::Node:
        {
            if (makeAlternative) {
                return NodeFactory::createAlternativeNode(state, parentNodeId, position, undoManager);
            }

            return NodeFactory::createNode(state, parentNodeId, position, undoManager);
        }

        case NodeCreationMode::Modulator:
        {
            if (parentType == ValueTreeIdentifiers::ModulatorRootData
                || parentType == ValueTreeIdentifiers::ModulatorData) {
                return NodeFactory::createModulator(state, parentNodeId, position, undoManager);
            }

            return NodeFactory::createModulatorRoot(state, parentNodeId, position, undoManager);
        }

        case NodeCreationMode::TraversalFlag:
        {
            return NodeFactory::createTraversalFlagNode(state, parentNodeId, position, undoManager);
        }
    }

    return {};
}

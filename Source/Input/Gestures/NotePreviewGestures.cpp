#include "NotePreviewGestures.h"
#include "../../Audio/TraversalSession.h"
#include "../../Graph/RTGraphBuilder.h"
#include "../../UI/Node/Node.h"

NotePreviewGestures::NotePreviewGestures(TraversalSession& traversalSession, RTGraphBuilder& rtGraphBuilder)
    : traversalSession(traversalSession),
      rtGraphBuilder  (rtGraphBuilder)
{
}

void NotePreviewGestures::respondToClick(juce::Component* clicked)
{
    const Node* node = dynamic_cast<const Node*>(clicked);

    if (node == nullptr) {
        return;
    }

    traversalSession.previewRequests.push({ RTPreviewRequest::Kind::Start, node->nodeId, static_cast<int>(count.getValue()),
                                            rtGraphBuilder.buildRTtraversal({ traversalId, 0 }), repeat });
}

void NotePreviewGestures::stopPreview()
{
    traversalSession.previewRequests.push({ RTPreviewRequest::Kind::Stop });
}

void NotePreviewGestures::toggleRepeat()
{
    repeat = ! repeat;

    traversalSession.previewRequests.push({ .kind = RTPreviewRequest::Kind::SetRepeat, .repeat = repeat });
}

#include "ArrowHoverController.h"
#include "NodeCanvas.h"
#include "../Node/Arrow.h"
#include "../Node/Node.h"

void ArrowHoverController::update(juce::Point<float> cursor) const
{
    for (Arrow* arrow : canvas.arrowManager.all()) {
        if (arrow->startNode == nullptr || arrow->endNode == nullptr) {
            continue;
        }
        if (arrow->startNode->nodeType != NodeType::TraversalFlag) {
            continue;
        }

        const float dist = CanvasHitTester::distanceToSegment(cursor,
                                       arrow->startNode->getNodeCentre().toFloat(),
                                       arrow->endNode->getNodeCentre().toFloat());

        const bool nearby = dist < flagProximityRadius;
        if (nearby != arrow->proximityHovered) {
            arrow->proximityHovered = nearby;
            arrow->refreshHoverVisibility();
        }
    }

    Arrow* const hoveredArrow = canvas.hitTester.arrowNear(cursor, boldHoverRadius);

    for (Arrow* arrow : canvas.arrowManager.all()) {
        const bool shouldBold = (arrow == hoveredArrow);
        if (shouldBold != arrow->hovered) {
            arrow->hovered = shouldBold;
            arrow->repaint();
        }
    }
}

#include "CanvasHitTester.h"
#include "NodeCanvas.h"
#include "../Node/Node.h"
#include "../Node/Arrow.h"
#include "../../Graph/ValueTreeIdentifiers.h"

#include <iterator>
#include <utility>

namespace
{

template <typename Range, typename Project, typename Keep, typename Dist>
auto nearest(const Range& range, Project project, Keep keep, Dist dist, float radius)
    -> decltype(project(*std::begin(range)))
{
    using Candidate = decltype(project(*std::begin(range)));

    Candidate best    = nullptr;
    float      minDist = radius;

    for (const auto& element : range) {
        Candidate const candidate = project(element);
        if (candidate == nullptr || ! keep(candidate)) {
            continue;
        }

        const float d = dist(candidate);
        if (d < minDist) {
            minDist = d;
            best    = candidate;
        }
    }

    return best;
}

Arrow* identity(Arrow* arrow) { return arrow; }

Node* nodeOf(const std::pair<const int, Node*>& entry) { return entry.second; }

}

float CanvasHitTester::distanceToSegment(juce::Point<float> p, juce::Point<float> a, juce::Point<float> b)
{
    const juce::Point<float> ab = b - a;
    const float lengthSquared = ab.x * ab.x + ab.y * ab.y;

    if (lengthSquared < 1.0e-6f) {
        return p.getDistanceFrom(a);
    }

    float t = ((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / lengthSquared;
    t = juce::jlimit(0.0f, 1.0f, t);

    const juce::Point<float> projection = a + ab * t;
    return p.getDistanceFrom(projection);
}

Arrow* CanvasHitTester::arrowNear(juce::Point<float> point, float radius) const
{
    return nearest(canvas.arrowManager.all(), identity,
        [] (Arrow* arrow) { return arrow->startNode != nullptr && arrow->isVisible(); },
        [point] (Arrow* arrow) {
            return distanceToSegment(point,
                                     arrow->startNode->getNodeCentre().toFloat(),
                                     arrow->getTip().toFloat());
        },
        radius);
}

Arrow* CanvasHitTester::arrowHeadNear(juce::Point<float> point, float radius) const
{
    return nearest(canvas.arrowManager.all(), identity,
        [] (Arrow* arrow) {
            return ! arrow->isDangling() && arrow->startNode != nullptr && arrow->isVisible();
        },
        [point] (Arrow* arrow) { return point.getDistanceFrom(arrow->getHeadAnchor()); },
        radius);
}

Arrow* CanvasHitTester::danglingHeadNear(juce::Point<float> point, float radius) const
{
    return nearest(canvas.arrowManager.all(), identity,
        [] (Arrow* arrow) { return arrow->isDangling() && arrow->startNode != nullptr; },
        [point] (Arrow* arrow) { return point.getDistanceFrom(arrow->getTip().toFloat()); },
        radius);
}

Node* CanvasHitTester::nodeNear(juce::Point<float> point, float radius, int excludeId) const
{
    return nearest(canvas.nodeManager.all(), nodeOf,
        [excludeId] (Node* node) { return node->getComponentID().getIntValue() != excludeId; },
        [point] (Node* node) { return point.getDistanceFrom(node->getNodeCentre().toFloat()); },
        radius);
}

Node* CanvasHitTester::rootNear(juce::Point<float> point, float radius, int excludeId) const
{
    return nearest(canvas.nodeManager.all(), nodeOf,
        [excludeId] (Node* node) {
            return node->getComponentID().getIntValue() != excludeId
                && node->nodeValueTree.getType() == ValueTreeIdentifiers::RootNodeData;
        },
        [point] (Node* node) { return point.getDistanceFrom(node->getNodeCentre().toFloat()); },
        radius);
}

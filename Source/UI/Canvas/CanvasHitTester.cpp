#include "CanvasHitTester.h"
#include "NodeCanvas.h"
#include "../Node/Node.h"
#include "../Node/Arrow.h"
#include "../../Graph/ValueTreeIdentifiers.h"

#include <iterator>
#include <limits>
#include <memory>
#include <ranges>
#include <utility>

namespace
{

template <std::ranges::input_range Range,
          std::indirectly_unary_invocable<std::ranges::iterator_t<const Range>> Project,
          std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<const Range>, Project>> Keep,
          std::indirectly_unary_invocable<std::projected<std::ranges::iterator_t<const Range>, Project>> Dist>
auto nearest(const Range& range, Project project, Keep keep, Dist dist, float radius)
    -> std::indirect_result_t<Project&, std::ranges::iterator_t<const Range>>
{
    using Candidate = std::indirect_result_t<Project&, std::ranges::iterator_t<const Range>>;

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

Node* nodeOf(const std::pair<const int, std::unique_ptr<Node>>& entry) { return entry.second.get(); }

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
        [] (Arrow* arrow) {
            return arrow->isDangling() && arrow->startNode != nullptr && arrow->isVisible();
        },
        [point] (Arrow* arrow) { return point.getDistanceFrom(arrow->getTip().toFloat()); },
        radius);
}

Arrow* CanvasHitTester::arrowLabelNear(juce::Point<float> point, float radius) const
{
    return nearest(canvas.arrowManager.all(), identity,
        [] (Arrow* arrow) {
            return arrow->startNode != nullptr && arrow->isVisible() && arrow->showsDurationLabel();
        },
        [point] (Arrow* arrow) {
            const ArrowLabel label = arrow->getLabel(arrow->getGeometry(1.0f), Arrow::arrowHeadLength);

            return point.getDistanceFrom(label.centre);
        },
        radius);
}

Node* CanvasHitTester::nodeNear(juce::Point<float> point, float radius, int excludeId) const
{
    return nearest(canvas.nodeManager.all(), nodeOf,
        [excludeId] (Node* node) {
            return node->nodeId != excludeId && node->isVisible();
        },
        [point] (Node* node) { return point.getDistanceFrom(node->getNodeCentre().toFloat()); },
        radius);
}

Node* CanvasHitTester::nodeContaining(juce::Point<float> point, int excludeId) const
{
    return nearest(canvas.nodeManager.all(), nodeOf,
        [point, excludeId] (Node* node) {
            const bool isConnectableType = node->nodeValueTree.getType() == ValueTreeIdentifiers::NodeData
                                        || node->nodeValueTree.getType() == ValueTreeIdentifiers::RootNodeData;

            return node->nodeId != excludeId
                && node->isVisible()
                && isConnectableType
                && point.getDistanceFrom(node->getNodeCentre().toFloat()) <= node->getVisualRadius();
        },
        [point] (Node* node) { return point.getDistanceFrom(node->getNodeCentre().toFloat()); },
        std::numeric_limits<float>::max());
}

Node* CanvasHitTester::rootNear(juce::Point<float> point, float radius, int excludeId) const
{
    return nearest(canvas.nodeManager.all(), nodeOf,
        [excludeId] (Node* node) {
            return node->nodeId != excludeId
                && node->isVisible()
                && node->nodeType != NodeType::Encapsulator
                && node->nodeValueTree.getType() == ValueTreeIdentifiers::RootNodeData;
        },
        [point] (Node* node) { return point.getDistanceFrom(node->getNodeCentre().toFloat()); },
        radius);
}

#include "OrnamentFigure.h"
#include "Processor/Document/ArrowBindingOps.h"
#include "Processor/Document/TypedProperties.h"
#include "Processor/Document/ValueTreeIdentifiers.h"
#include "Core/Model/ArrowInfo.h"
#include <ArsNovaUI/Canvas/CanvasIdentifiers.h>

#include <algorithm>
#include <set>

void OrnamentFigure::setNodePosition(juce::ValueTree ornament, int nodeId, juce::Point<int> position, juce::UndoManager* undoManager)
{
    const juce::ValueTree figure = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);

    juce::ValueTree node  = figure.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, nodeId);
    juce::ValueTree moved = node;

    FigureNoteProperties properties(node);

    const int oldX          = node.getProperty(ArsNovaUI::CanvasIdentifiers::XPosition);
    const int oldY          = node.getProperty(ArsNovaUI::CanvasIdentifiers::YPosition);
    const int oldOffset     = properties.offset;
    const int oldRow        = juce::roundToInt(oldY / ArrowInfo::pixelsPerGridSpace);
    const int newRow        = juce::roundToInt(position.y / ArrowInfo::pixelsPerGridSpace);
    const int rowShift      = juce::roundToInt((oldRow - newRow) * ArrowInfo::semitonesPerGridSpace);
    const int shiftedOffset = oldOffset + rowShift;
    const int clampedOffset = std::clamp(shiftedOffset, -maximumOffset, maximumOffset);

    const juce::Point<int> oldPosition { oldX, oldY };
    const juce::Point<int> delta = position - oldPosition;

    auto clearDurationOverride = [undoManager](juce::ValueTree arrowTree) {
        ArrowProperties arrow(arrowTree);

        if (arrow.durationOverride != ArrowInfo::noDurationOverride) {
            arrow.durationOverride.setValue(ArrowInfo::noDurationOverride, undoManager);
        }
    };

    if (! node.isValid()) {
        return;
    }

    properties.offset.setValue(clampedOffset, undoManager);

    for (const juce::ValueTree& candidate : figure) {
        clearDurationOverride(candidate.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds).getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, nodeId));
    }

    while (moved.isValid()) {
        const juce::ValueTree outgoing = moved.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds).getChild(0);

        const juce::Point<int> movedFrom { moved.getProperty(ArsNovaUI::CanvasIdentifiers::XPosition), moved.getProperty(ArsNovaUI::CanvasIdentifiers::YPosition) };
        const juce::Point<int> movedTo = movedFrom + delta;

        moved.setProperty(ArsNovaUI::CanvasIdentifiers::XPosition, movedTo.x, undoManager);
        moved.setProperty(ArsNovaUI::CanvasIdentifiers::YPosition, movedTo.y, undoManager);

        clearDurationOverride(outgoing);

        moved = figure.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, outgoing.getProperty(ArsNovaUI::CanvasIdentifiers::Id, -1));
    }
}

void OrnamentFigure::removeNode(juce::ValueTree ornament, int nodeId, juce::UndoManager* undoManager)
{
    juce::ValueTree figure = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);

    const juce::ValueTree node     = figure.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, nodeId);
    const juce::ValueTree outgoing = node.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds).getChild(0);

    juce::ValueTree parent;
    juce::ValueTree incoming;

    juce::CachedValue<int> hostId(figure, ValueTreeIdentifiers::OrnamentHostId, nullptr, noHostId);

    if (! node.isValid()) {
        return;
    }

    if (hostId == nodeId) {
        hostId.setValue(noHostId, undoManager);
    }

    for (const juce::ValueTree& candidate : figure) {
        const juce::ValueTree connection = candidate.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds).getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, nodeId);

        if (connection.isValid()) {
            parent   = candidate;
            incoming = connection;
        }
    }

    if (incoming.isValid() && outgoing.isValid()) {
        incoming.setProperty(ArsNovaUI::CanvasIdentifiers::Id, outgoing.getProperty(ArsNovaUI::CanvasIdentifiers::Id), undoManager);
    }

    if (incoming.isValid() && ! outgoing.isValid()) {
        const juce::ValueTree danglingArrows = node.getChildWithName(ArsNovaUI::CanvasIdentifiers::DanglingArrows);

        parent.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds).removeChild(incoming, undoManager);

        if (danglingArrows.isValid() && danglingArrows.getNumChildren() > 0) {
            parent.appendChild(danglingArrows.createCopy(), undoManager);
        }
    }

    figure.removeChild(node, undoManager);
}

void OrnamentFigure::toggleHost(juce::ValueTree ornament, int nodeId, juce::UndoManager* undoManager)
{
    juce::ValueTree figure = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);

    juce::CachedValue<int> storedHostId(figure, ValueTreeIdentifiers::OrnamentHostId, nullptr, noHostId);

    int hostId = nodeId;

    if (! figure.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, nodeId).isValid()) {
        return;
    }

    if (storedHostId == nodeId) {
        hostId = noHostId;
    }

    storedHostId.setValue(hostId, undoManager);
}

void OrnamentFigure::addDanglingArrow(juce::ValueTree ornament, int nodeId, juce::Point<int> tipOffset, juce::UndoManager* undoManager)
{
    const juce::ValueTree figure = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);

    juce::ValueTree node           = figure.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, nodeId);
    juce::ValueTree danglingArrows = node.getChildWithName(ArsNovaUI::CanvasIdentifiers::DanglingArrows);
    juce::ValueTree danglingArrow  {ArsNovaUI::CanvasIdentifiers::DanglingArrow};

    ArrowInfo::Settings arrowInfo;

    if (! node.isValid()) {
        return;
    }

    if (! danglingArrows.isValid()) {
        danglingArrows = juce::ValueTree(ArsNovaUI::CanvasIdentifiers::DanglingArrows);

        node.appendChild(danglingArrows, undoManager);
    }

    if (danglingArrows.getNumChildren() > 0) {
        danglingArrows.getChild(0).setProperty(ArsNovaUI::CanvasIdentifiers::ArrowTipX, tipOffset.x, undoManager);
        danglingArrows.getChild(0).setProperty(ArsNovaUI::CanvasIdentifiers::ArrowTipY, tipOffset.y, undoManager);

        return;
    }

    arrowInfo.xBinding = ArrowInfo::Binding::DurationBind;
    arrowInfo.yBinding = ArrowInfo::Binding::NoBind;

    danglingArrow.setProperty(ArsNovaUI::CanvasIdentifiers::ArrowTipX, tipOffset.x, nullptr);
    danglingArrow.setProperty(ArsNovaUI::CanvasIdentifiers::ArrowTipY, tipOffset.y, nullptr);

    ArrowBindingOps::setArrowInfo(danglingArrow, arrowInfo, nullptr);

    danglingArrows.appendChild(danglingArrow, undoManager);
}

void OrnamentFigure::removeDanglingArrow(juce::ValueTree ornament, juce::UndoManager* undoManager)
{
    const juce::ValueTree figure = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);

    for (juce::ValueTree node : figure) {
        const juce::ValueTree danglingArrows = node.getChildWithName(ArsNovaUI::CanvasIdentifiers::DanglingArrows);

        if (danglingArrows.isValid()) {
            node.removeChild(danglingArrows, undoManager);
        }
    }
}

void OrnamentFigure::setLastNodeLength(juce::ValueTree ornament, juce::Point<int> tipOffset, juce::UndoManager* undoManager)
{
    const juce::ValueTree figure = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);

    for (const juce::ValueTree& node : figure) {
        juce::ValueTree danglingArrow = node.getChildWithName(ArsNovaUI::CanvasIdentifiers::DanglingArrows).getChild(0);

        if (danglingArrow.isValid()) {
            danglingArrow.setProperty(ArsNovaUI::CanvasIdentifiers::ArrowTipX, tipOffset.x, undoManager);
            danglingArrow.setProperty(ArsNovaUI::CanvasIdentifiers::ArrowTipY, tipOffset.y, undoManager);

            return;
        }
    }

    for (const juce::ValueTree& node : figure) {
        if (node.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds).getNumChildren() == 0) {
            addDanglingArrow(ornament, node.getProperty(ArsNovaUI::CanvasIdentifiers::Id), tipOffset, undoManager);

            return;
        }
    }
}

void OrnamentFigure::publish(juce::ValueTree ornament, juce::UndoManager* undoManager)
{
    juce::ValueTree published {ValueTreeIdentifiers::PublishedFigure};

    if (! isPublishable(ornament)) {
        return;
    }

    published.copyPropertiesAndChildrenFrom(ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure), nullptr);

    ornament.removeChild(ornament.getChildWithName(ValueTreeIdentifiers::PublishedFigure), undoManager);
    ornament.appendChild(published, undoManager);
}

juce::ValueTree OrnamentFigure::addNode(juce::ValueTree ornament, int parentNodeId, juce::Point<int> position, juce::UndoManager* undoManager)
{
    OrnamentProperties properties(ornament);

    juce::ValueTree figure = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);
    juce::ValueTree parent = figure.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, parentNodeId);
    juce::ValueTree node   {ValueTreeIdentifiers::OrnamentNoteData};

    const int nodeId           = properties.nextFigureNodeId;
    const int nextFigureNodeId = nodeId + 1;

    ArrowInfo::Settings arrowInfo;

    FigureNoteProperties noteProperties(node);

    if (! figure.isValid() || (parentNodeId == -1 && figure.getNumChildren() > 0)) {
        return {};
    }

    if (parentNodeId != -1 && (! parent.isValid() || parent.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds).getNumChildren() > 0)) {
        return {};
    }

    arrowInfo.xBinding = ArrowInfo::Binding::DurationBind;
    arrowInfo.yBinding = ArrowInfo::Binding::NoBind;

    node.setProperty(ArsNovaUI::CanvasIdentifiers::Id,               nodeId,                 nullptr);
    node.setProperty(ArsNovaUI::CanvasIdentifiers::XPosition,        position.x,             nullptr);
    node.setProperty(ArsNovaUI::CanvasIdentifiers::YPosition,        position.y,             nullptr);

    noteProperties.offset      .setValue(0,                      nullptr);
    noteProperties.velocity    .setValue(defaultVelocityPercent, nullptr);
    properties.nextFigureNodeId.setValue(nextFigureNodeId,       undoManager);

    node.appendChild(juce::ValueTree(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds), nullptr);

    if (parent.isValid()) {
        juce::ValueTree connection {ValueTreeIdentifiers::NodeId};

        connection.setProperty(ArsNovaUI::CanvasIdentifiers::Id, nodeId, nullptr);

        ArrowBindingOps::setArrowInfo(connection, arrowInfo, nullptr);

        parent.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds).appendChild(connection, undoManager);
        parent.removeChild(parent.getChildWithName(ArsNovaUI::CanvasIdentifiers::DanglingArrows), undoManager);
    }

    figure.appendChild(node, undoManager);

    return node;
}

bool OrnamentFigure::isPublishable(const juce::ValueTree& ornament)
{
    juce::ValueTree figure = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);

    const juce::CachedValue<int> hostId(figure, ValueTreeIdentifiers::OrnamentHostId, nullptr, noHostId);

    const int storedHostId = hostId;

    int totalWeight = 0;

    for (const OrnamentStep& step : steps(figure)) {
        totalWeight += step.durationWeight;
    }

    return figure.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, storedHostId).isValid()
           && figure.getNumChildren() > 1 && totalWeight > 0;
}

std::vector<OrnamentStep> OrnamentFigure::steps(const juce::ValueTree& figure)
{
    juce::ValueTree figureTree = figure;
    juce::ValueTree node;

    const juce::CachedValue<int> storedHostId(figureTree, ValueTreeIdentifiers::OrnamentHostId, nullptr, noHostId);

    const int hostId = storedHostId;

    std::vector<OrnamentStep> steps;

    std::set<int> pointedAt;

    for (const juce::ValueTree& candidate : figure) {
        for (const juce::ValueTree& connection : candidate.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds)) {
            const int targetId = connection.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

            pointedAt.insert(targetId);
        }
    }

    for (const juce::ValueTree& candidate : figure) {
        const int candidateId = candidate.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

        if (pointedAt.count(candidateId) == 0) {
            node = candidate;

            break;
        }
    }

    walkChain(node, figure, hostId, steps);

    return steps;
}

OrnamentStatus OrnamentFigure::statusOf(const juce::ValueTree& ornament)
{
    const juce::ValueTree published = ornament.getChildWithName(ValueTreeIdentifiers::PublishedFigure);

    juce::ValueTree comparable {ValueTreeIdentifiers::OrnamentFigure};

    if (! published.isValid()) {
        return OrnamentStatus::Draft;
    }

    comparable.copyPropertiesAndChildrenFrom(published, nullptr);

    if (comparable.isEquivalentTo(ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure))) {
        return OrnamentStatus::Published;
    }

    return OrnamentStatus::ChangesPending;
}

void OrnamentFigure::walkChain(juce::ValueTree node, const juce::ValueTree& figure, int hostId, std::vector<OrnamentStep>& steps)
{
    int previousDurationWeight = 0;

    for (int visited = 0; node.isValid() && visited < figure.getNumChildren(); ++visited) {
        const juce::ValueTree connection    = node.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds).getChild(0);
        const juce::ValueTree danglingArrow = node.getChildWithName(ArsNovaUI::CanvasIdentifiers::DanglingArrows).getChild(0);

        const int nodeX = node.getProperty(ArsNovaUI::CanvasIdentifiers::XPosition);
        const int nodeY = node.getProperty(ArsNovaUI::CanvasIdentifiers::YPosition);

        juce::ValueTree next;

        const FigureNoteProperties note(node);

        OrnamentStep step { OrnamentNodeKind::Note, note.offset, note.velocity, 0 };

        if (connection.isValid()) {
            next = figure.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, connection.getProperty(ArsNovaUI::CanvasIdentifiers::Id));
        }

        if (static_cast<int>(node.getProperty(ArsNovaUI::CanvasIdentifiers::Id)) == hostId) {
            step.kind = OrnamentNodeKind::Host;
        }

        if (next.isValid()) {
            const int nextX  = next.getProperty(ArsNovaUI::CanvasIdentifiers::XPosition);
            const int nextY  = next.getProperty(ArsNovaUI::CanvasIdentifiers::YPosition);
            const int deltaX = nextX - nodeX;
            const int deltaY = nextY - nodeY;

            const ArrowInfo::Settings connectionInfo = ArrowBindingOps::getArrowInfo(connection);

            step.durationWeight    = ArrowInfo::Settings::durationFromDelta(connectionInfo, deltaX, deltaY);
            previousDurationWeight = step.durationWeight;
        }
        else if (danglingArrow.isValid()) {
            step.durationWeight = ArrowInfo::Settings::durationFromDelta(ArrowBindingOps::getArrowInfo(danglingArrow),
                                                                         danglingArrow.getProperty(ArsNovaUI::CanvasIdentifiers::ArrowTipX),
                                                                         danglingArrow.getProperty(ArsNovaUI::CanvasIdentifiers::ArrowTipY));
        }
        else {
            step.durationWeight = previousDurationWeight;
        }

        if (step.durationWeight > 0) {
            steps.push_back(step);
        }

        node = next;
    }
}

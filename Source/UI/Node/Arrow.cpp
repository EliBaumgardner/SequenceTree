#include "../../Graph/GraphState.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "Arrow.h"

#include "Node.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

Arrow::Arrow(ArsNovaUI::CanvasNode* startNode, ArsNovaUI::CanvasNode* endNode, juce::UndoManager& undoManager)
    : ArsNovaUI::CanvasArrow(startNode, endNode, undoManager)
{
}

Arrow::Arrow(ArsNovaUI::CanvasNode* startNode, juce::Point<int> tipOffset, juce::UndoManager& undoManager)
    : ArsNovaUI::CanvasArrow(startNode, tipOffset, undoManager)
{
    valueEditor = std::make_unique<ArsNovaUI::ValueEditor>(undoManager);

    valueEditor->setInterceptsMouseClicks(true, false);
    valueEditor->setTooltip("Count Limit");
    valueEditor->setFormat(std::make_unique<ArsNovaUI::NumberFormat>(NodeInfo::minimumCountLimit, NodeInfo::maximumCountLimit));
    addAndMakeVisible(*valueEditor);

    durationEditor->toFront(false);
}

void Arrow::paint(juce::Graphics& graphics)
{
    ArsNovaUI::CustomLookAndFeel::get(*this).drawArrow(graphics, getArrowVisual());
}

void Arrow::resized()
{
    ArsNovaUI::CanvasArrow::resized();

    if (valueEditor == nullptr) {
        return;
    }

    valueEditor->setBounds(juce::Rectangle<int>(0, 0, valueEditorWidth, valueEditorHeight).withCentre(getTip() - getPosition()));
}

void Arrow::beginDurationEdit()
{
    if (startNode == nullptr || ! arrowTree.isValid() || ! showsDurationLabel()) {
        return;
    }

    const bool percentDuration = (static_cast<const Node*>(startNode)->nodeType == NodeInfo::Type::Modulator);

    auto durationFormat = std::make_unique<ArsNovaUI::NumberFormat>(0.0, ArrowInfo::maximumDurationMs);

    if (percentDuration) {
        durationFormat = std::make_unique<ArsNovaUI::NumberFormat>(
            0.0, static_cast<double>(ArrowInfo::maximumDurationPercent * ArrowInfo::millisecondsPerDurationPercent));

        durationFormat->displayDivisor = ArrowInfo::millisecondsPerDurationPercent;
        durationFormat->suffix         = "%";
    }

    durationEditor->setFormat(std::move(durationFormat));

    const int durationOverride = arrowTree.getProperty(ValueTreeIdentifiers::ArrowDuration, ArrowInfo::noDurationOverride);

    if (durationOverride == ArrowInfo::noDurationOverride) {
        arrowTree.setProperty(ValueTreeIdentifiers::ArrowDuration, getDuration(), nullptr);
    }

    durationEditor->bindEditor(arrowTree, ValueTreeIdentifiers::ArrowDuration);

    editingDuration = true;

    durationEditor->setVisible(true);
    durationEditor->beginEditing();

    repaint();
}

void Arrow::startProgress(int trailId, int durationMs, int elapsedMs, juce::Colour colour, bool oneShot, BridgeCommands::TrailSource source)
{
    Trail& trail = trails[trailId];

    double originMs = juce::Time::getMillisecondCounterHiRes();

    if (trailsPaused && source == BridgeCommands::TrailSource::Live) {
        originMs = pausedAtMs;
    }

    trail.durationMs = juce::jmax(1, durationMs);
    trail.startMs    = originMs - elapsedMs;
    trail.progress   = static_cast<float>(juce::jlimit(0.0, 1.0, elapsedMs / static_cast<double>(trail.durationMs)));
    trail.colour     = colour;
    trail.active     = durationMs > 0;
    trail.oneShot    = oneShot;
    trail.source     = source;

    if (animationFrames.isEmpty()) {
        animationFrames = juce::VBlankAttachment(this, [this](double frameSec) { advanceAnimation(frameSec); });
    }

    repaint();
}

void Arrow::resetProgress(int trailId)
{
    if (trails.erase(trailId) > 0) {
        repaint();
    }
}

void Arrow::resetProgress()
{
    if (trails.empty()) {
        return;
    }

    trails.clear();
    repaint();
}

void Arrow::resumeProgress()
{
    if (trailsPaused) {
        const double pausedForMs = juce::Time::getMillisecondCounterHiRes() - pausedAtMs;

        for (auto& [trailId, trail] : trails) {
            if (trail.source == BridgeCommands::TrailSource::Live) {
                trail.startMs += pausedForMs;
            }
        }

        trailsPaused = false;
    }

    if (! trails.empty() && animationFrames.isEmpty()) {
        animationFrames = juce::VBlankAttachment(this, [this](double frameSec) { advanceAnimation(frameSec); });
    }
}

bool Arrow::advanceExtraAnimation()
{
    const double nowMs     = juce::Time::getMillisecondCounterHiRes();
    bool         anyActive = false;

    for (auto entry = trails.begin(); entry != trails.end(); ) {
        Trail& trail = entry->second;

        if (! trail.active || (trailsPaused && trail.source == BridgeCommands::TrailSource::Live)) {
            ++entry;
            continue;
        }

        const double normalised = (nowMs - trail.startMs) / static_cast<double>(trail.durationMs);

        if (normalised >= 1.0) {
            if (trail.oneShot) {
                entry = trails.erase(entry);
                continue;
            }

            trail.progress = 1.0f;
            trail.active   = false;
        }
        else {
            trail.progress = static_cast<float>(juce::jlimit(0.0, 1.0, normalised));
            anyActive      = true;
        }

        ++entry;
    }

    return anyActive;
}

bool Arrow::showsDurationLabel() const
{
    const ArrowInfo::Settings arrowInfo = ArrowBindingOps::getArrowInfo(arrowTree);

    if (ArrowInfo::Settings::bindsTo(arrowInfo, ArrowInfo::Binding::DurationBind)) {
        return true;
    }

    return ! ArrowInfo::Settings::bindsTo(arrowInfo, ArrowInfo::Binding::PitchBind);
}

bool Arrow::isTraversalArrow() const
{
    if (isDangling() || ! arrowTree.isValid()) {
        return false;
    }

    return ArrowBindingOps::getArrowInfo(arrowTree).type == ArrowInfo::Type::Traversal;
}

bool Arrow::isSyncArrow() const
{
    return ArrowBindingOps::getArrowInfo(arrowTree).isSynced;
}

int Arrow::getDuration() const
{
    if (startNode == nullptr) {
        return 0;
    }

    const juce::Point<int> delta = getTip() - startNode->getNodeCentre();

    return ArrowInfo::Settings::durationFromDelta(ArrowBindingOps::getArrowInfo(arrowTree), delta.x, delta.y);
}

juce::String Arrow::getDurationLabel() const
{
    if (startNode == nullptr) {
        return "0";
    }

    if (! showsDurationLabel()) {
        const Node* pitchedNode = static_cast<const Node*>(endNode);

        if (isDangling() || startNode->isAlternativeNode) {
            pitchedNode = static_cast<const Node*>(startNode);
        }

        if (pitchedNode != nullptr) {
            return juce::String(static_cast<int>(pitchedNode->midiNoteData.getProperty(ValueTreeIdentifiers::MidiPitch, NodeInfo::defaultMidiPitch)));
        }
    }

    const int duration = getDuration();

    if (static_cast<const Node*>(startNode)->nodeType == NodeInfo::Type::Modulator) {
        return juce::String(duration / ArrowInfo::millisecondsPerDurationPercent) + "%";
    }

    return juce::String(duration);
}

ArsNovaUI::ArrowVisual Arrow::getArrowVisual() const
{
    ArsNovaUI::ArrowVisual visual;

    visual.geometry = getGeometry(animation.snapProgress);

    if (! visual.geometry.valid) {
        return visual;
    }

    visual.origin       = { static_cast<float>(getX()), static_cast<float>(getY()) };
    visual.labelText    = getDurationLabel();
    visual.shaftStyle   = shaftStyle();
    visual.headLength   = arrowHeadLength;
    visual.rulerOpacity = animation.rulerFade;
    visual.rulerSpacing = ArrowInfo::pixelsPerGridSpace;
    visual.emphasised   = hovered || selected;
    visual.ghost        = isGhost;
    visual.showsLabel   = visual.labelText.isNotEmpty() && ! editingDuration && animation.snapProgress > labelVisibleThreshold;

    if (visual.emphasised) {
        visual.headLength = arrowHeadLengthHover;
    }

    if (visual.shaftStyle == ArsNovaUI::ArrowVisual::ShaftStyle::Ruler) {
        visual.rulerOpacity = 1.0f;
    }

    if (isTraversalArrow()) {
        visual.head = ArsNovaUI::ArrowVisual::HeadStyle::Outlined;
    }

    visual.shaft = buildShaftPath(visual.geometry, visual.headLength, visual.origin);

    if (visual.showsLabel) {
        visual.label = getLabel(visual.geometry, visual.headLength);
    }

    for (const auto& entry : trails) {
        const Trail&          trail = entry.second;
        ArsNovaUI::ArrowTrail drawnTrail { trail.colour, trail.progress };

        if (trail.source == BridgeCommands::TrailSource::Preview) {
            drawnTrail.stroke = ArsNovaUI::ArrowTrail::Stroke::Dashed;
        }

        visual.trails.push_back(drawnTrail);
    }

    return visual;
}

ArsNovaUI::ArrowVisual::ShaftStyle Arrow::shaftStyle() const
{
    if (dragging) {
        return ArsNovaUI::ArrowVisual::ShaftStyle::Ruler;
    }

    if (isGhost) {
        return ArsNovaUI::ArrowVisual::ShaftStyle::Dashed;
    }

    if (startNode == nullptr) {
        return ArsNovaUI::ArrowVisual::ShaftStyle::Solid;
    }

    if (static_cast<const Node*>(startNode)->nodeType == NodeInfo::Type::TraversalFlag) {
        return ArsNovaUI::ArrowVisual::ShaftStyle::Dashed;
    }

    if (endNode == nullptr || static_cast<const Node*>(endNode)->nodeType != NodeInfo::Type::Root || startNode->isAlternativeNode) {
        return ArsNovaUI::ArrowVisual::ShaftStyle::Solid;
    }

    const ArrowInfo::Type arrowType = ArrowBindingOps::getArrowInfo(arrowTree).type;

    if (arrowType == ArrowInfo::Type::Traversal || arrowType == ArrowInfo::Type::StepIntoTree) {
        return ArsNovaUI::ArrowVisual::ShaftStyle::Solid;
    }

    return ArsNovaUI::ArrowVisual::ShaftStyle::Dashed;
}
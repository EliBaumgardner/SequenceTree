#include "OrnamentFigureController.h"
#include "OrnamentCanvas.h"
#include "../../Input/FigureDocument.h"
#include "../../Graph/OrnamentLibraries.h"

OrnamentFigureController::OrnamentFigureController(OrnamentCanvas& ornamentCanvas, FigureDocument& figureDocument, OrnamentLibraries& ornamentLibraries,
                                                   juce::UndoManager& undoManager)
    : ArsNovaUI::CanvasController(ornamentCanvas, figureDocument, undoManager),
      ornamentCanvas(ornamentCanvas),
      figureDocument(figureDocument),
      ornamentLibraries(ornamentLibraries),
      undoManager(undoManager)
{
}

void OrnamentFigureController::mouseDown(const juce::MouseEvent& event)
{
    const juce::Point<int> point = event.getEventRelativeTo(&ornamentCanvas).position.transformedBy(ornamentCanvas.modelTransform).roundToInt();

    Node* const node = dynamic_cast<Node*>(event.eventComponent);

    if (! figureDocument.ornament.isValid()) {
        return;
    }

    if (clickMode == ClickMode::PickHost) {
        if (node != nullptr && ! event.mods.isRightButtonDown()) {
            undoManager.beginNewTransaction();

            ornamentLibraries.toggleHost(figureDocument.ornament, node->nodeId, &undoManager);
        }

        switchClickMode(ClickMode::Build);

        return;
    }

    if (event.eventComponent == &ornamentCanvas && event.mods.isShiftDown() && event.mods.isLeftButtonDown()) {
        undoManager.beginNewTransaction();

        figureDocument.addRootNode(ornamentCanvas.snapPointToGrid(point));

        return;
    }

    ArsNovaUI::CanvasController::mouseDown(event);
}

void OrnamentFigureController::switchClickMode(ClickMode mode)
{
    juce::Image    cursorImage(juce::Image::ARGB, hostCursorSize, hostCursorSize, true);
    juce::Graphics cursorGraphics(cursorImage);

    clickMode = mode;

    if (mode == ClickMode::PickHost) {
        arrowGestures.setArrowMode(false);

        cursorGraphics.setColour(juce::Colours::black);
        cursorGraphics.drawEllipse(cursorImage.getBounds().toFloat().reduced(1.0f), 1.5f);

        ornamentCanvas.setMouseCursor(juce::MouseCursor(cursorImage, hostCursorSize / 2, hostCursorSize / 2));
    }
    else {
        ornamentCanvas.setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    if (onClickModeChanged != nullptr) {
        onClickModeChanged();
    }

    if (onArrowModeChanged != nullptr) {
        onArrowModeChanged();
    }
}

void OrnamentFigureController::setArrowMode(bool enabled)
{
    if (arrowGestures.arrowMode == enabled) {
        return;
    }

    arrowGestures.setArrowMode(enabled);

    if (enabled && clickMode == ClickMode::PickHost) {
        switchClickMode(ClickMode::Build);

        return;
    }

    if (onArrowModeChanged != nullptr) {
        onArrowModeChanged();
    }
}
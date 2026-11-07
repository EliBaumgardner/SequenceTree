#include "OrnamentBuildPanel.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "../../Graph/OrnamentLibraries.h"
#include "../../Util/ArrowInfo.h"

OrnamentBuildPanel::OrnamentBuildPanel(OrnamentLibraries& ornamentLibraries, juce::UndoManager& undoManager)
    : document         (ornamentLibraries, undoManager),
      canvas           (document),
      port             (canvas),
      controller       (canvas, document, ornamentLibraries),
      ornamentLibraries(ornamentLibraries),
      symbolEditor     (ornamentLibraries, undoManager),
      nameEditor       (undoManager)
{
    titlebar.nodePane.onSelectionChanged = [this](const ArsNovaUI::IconButton* selected) {
        if (selected == titlebar.hostButton) {
            controller.switchClickMode(OrnamentFigureController::ClickMode::PickHost);
        }
        else if (selected == titlebar.nodeButton) {
            controller.setArrowMode(false);
            controller.switchClickMode(OrnamentFigureController::ClickMode::Build);
        }
        else {
            controller.setArrowMode(true);
        }
    };

    controller.onClickModeChanged = [this]() {
        if (controller.clickMode == OrnamentFigureController::ClickMode::PickHost) {
            titlebar.nodePane.setSelectedButton(titlebar.hostButton);
        }
        else if (controller.arrowGestures.arrowMode) {
            titlebar.nodePane.setSelectedButton(nullptr);
        }
        else {
            titlebar.nodePane.setSelectedButton(titlebar.nodeButton);
        }
    };

    titlebar.undoRedoPane.addButton(&ArsNovaUI::CustomLookAndFeel::drawUndoIcon, "Undo", [&undoManager]() { undoManager.undo(); });
    titlebar.undoRedoPane.addButton(&ArsNovaUI::CustomLookAndFeel::drawRedoIcon, "Redo", [&undoManager]() { undoManager.redo(); });

    titlebar.publishButton->onClick = [this, &undoManager]() {
        undoManager.beginNewTransaction();

        this->ornamentLibraries.publish(ornament, &undoManager);
    };

    titlebar.symbolButton->onClick = [this]() {
        if (view == BuildView::Figure) {
            view = BuildView::Symbol;
        }
        else {
            view = BuildView::Figure;
        }

        applyView();
    };

    nameEditor.autoFitText        = true;
    nameEditor.fontStyle          = ArsNovaUI::Theme::FontStyle::SemiBold;
    controller.onArrowModeChanged = controller.onClickModeChanged;
    port.initialViewCentre        = { OrnamentCanvas::initialViewColumns * ArrowInfo::pixelsPerGridSpace, 0.0f };

    nameEditor.setFormat(std::make_unique<ArsNovaUI::TextFormat>(ArsNovaUI::TextFormat::labelTextLength, ArsNovaUI::TextFormat::labelCharacters));
    nameEditor.setJustification(juce::Justification::centredLeft);
    nameEditor.setTooltip("Ornament name");

    port.onTransformChanged = [this](const juce::AffineTransform& viewTransform, float) {
        canvas.viewTransform  = viewTransform;
        canvas.modelTransform = viewTransform.inverted();
    };

    canvas.addMouseListener(&controller, true);

    addAndMakeVisible(titlebar);

    addChildComponent(port);
    addChildComponent(symbolEditor);
    addChildComponent(nameEditor);

    ornamentLibraries.libraries.addListener(this);

    showActiveOrnament();
}

OrnamentBuildPanel::~OrnamentBuildPanel()
{
    ornamentLibraries.libraries.removeListener(this);

    document.nodes.removeListener(&canvas.treeListener);

    canvas.removeMouseListener(&controller);
}

void OrnamentBuildPanel::paint(juce::Graphics& graphics)
{
    const ArsNovaUI::Theme& theme = ArsNovaUI::CustomLookAndFeel::get(*this);
    const auto              body  = getLocalBounds().withTrimmedTop(titlebarHeight);

    graphics.setColour(theme.canvasColour);
    graphics.fillRect(body);

    if (ornament.isValid()) {
        return;
    }

    graphics.setColour(theme.mutedTextColour);
    graphics.setFont(theme.font(ArsNovaUI::Theme::FontStyle::Regular, ArsNovaUI::Theme::labelFontHeight));
    graphics.drawText("Choose an ornament, or add one to a library", body, juce::Justification::centred, true);
}

void OrnamentBuildPanel::resized()
{
    auto      bounds    = getLocalBounds();
    const int nameWidth = juce::roundToInt(bounds.getWidth() * nameWidthRatio);

    titlebar.setBounds(bounds.removeFromTop(titlebarHeight));
    port.setBounds(bounds);
    symbolEditor.setBounds(bounds);
    nameEditor.setBounds(bounds.getX() + nameInset, bounds.getBottom() - nameInset - nameHeight, nameWidth, nameHeight);
}

int OrnamentBuildPanel::minimumWidth() const
{
    const int barInset      = juce::roundToInt(titlebarHeight * titlebarInsetRatio);
    const int contentHeight = titlebarHeight - barInset * 2;
    const int paneWidths    = titlebar.undoRedoPane.idealWidth(contentHeight) + titlebar.nodePane.idealWidth(contentHeight);
    const int buttonsWidth  = paneWidths + titlebar.symbolPane.idealWidth(contentHeight) + titlebar.publishPane.idealWidth(contentHeight);

    const float spacingShare = 1.0f - ArsNovaUI::Theme::contentSpacingRatio * titlebarGaps;

    return barInset * 2 + static_cast<int>(std::ceil(buttonsWidth / spacingShare));
}

void OrnamentBuildPanel::applyView()
{
    titlebar.nodePane.setVisible(ornament.isValid() && view == BuildView::Figure);
    port.setVisible(ornament.isValid() && view == BuildView::Figure);
    nameEditor.setVisible(ornament.isValid() && view == BuildView::Figure);
    symbolEditor.setVisible(ornament.isValid() && view == BuildView::Symbol);

    titlebar.symbolButton->setSelected(view == BuildView::Symbol);

    controller.switchClickMode(OrnamentFigureController::ClickMode::Build);
    controller.setArrowMode(false);
}

void OrnamentBuildPanel::showActiveOrnament()
{
    ornament = ornamentLibraries.findOrnament(ornamentLibraries.libraries.getProperty(ValueTreeIdentifiers::ActiveOrnamentUid).toString());

    document.nodes.removeListener(&canvas.treeListener);

    document.ornament     = ornament;
    document.nodes        = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);
    symbolEditor.ornament = ornament;

    document.nodes.addListener(&canvas.treeListener);

    canvas.rebuildRequested = true;

    titlebar.symbolPane.setVisible(ornament.isValid());
    titlebar.publishPane.setVisible(ornament.isValid());

    applyView();

    if (ornament.isValid()) {
        nameEditor.bindEditor(ornament, ValueTreeIdentifiers::OrnamentName);
    }

    refreshFigure();

    repaint();
}

void OrnamentBuildPanel::refreshFigure()
{
    const bool canPublish = ornamentLibraries.isPublishable(ornament) && ornamentLibraries.statusOf(ornament) != OrnamentStatus::Published;

    titlebar.publishButton->setEnabled(ornament.isValid() && canPublish);

    canvas.refreshHost();
}

void OrnamentBuildPanel::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    if (property == ValueTreeIdentifiers::ActiveOrnamentUid) {
        showActiveOrnament();

        return;
    }

    if (tree == ornament && property == ValueTreeIdentifiers::OrnamentSymbol) {
        symbolEditor.repaint();

        return;
    }

    if (tree.isAChildOf(ornament)) {
        refreshFigure();
    }
}

void OrnamentBuildPanel::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    if (child.getType() == ValueTreeIdentifiers::OrnamentLibrary) {
        showActiveOrnament();

        return;
    }

    if (parent == ornament || parent.isAChildOf(ornament)) {
        refreshFigure();
    }
}

void OrnamentBuildPanel::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index)
{
    if (child.getType() == ValueTreeIdentifiers::OrnamentLibrary) {
        showActiveOrnament();

        return;
    }

    if (parent == ornament || parent.isAChildOf(ornament)) {
        refreshFigure();
    }
}

OrnamentBuildPanel::BuildTitlebar::BuildTitlebar()
    : ArsNovaUI::Bar(Orientation::Horizontal, titlebarInsetRatio)
{
    nodePane.selection     = ArsNovaUI::ButtonPane::Selection::ExclusiveOrNone;
    hostButton             = &nodePane.addButton(&ArsNovaUI::CustomLookAndFeel::drawHostNodeIcon, "Host node");
    nodeButton             = &nodePane.addButton(&ArsNovaUI::CustomLookAndFeel::drawCircleIcon, "Node (deselect for arrows)");
    hostButton->state.look = ArsNovaUI::ButtonState::Look::Raised;
    nodeButton->state.look = ArsNovaUI::ButtonState::Look::Raised;
    symbolButton           = &symbolPane.addButton(&ArsNovaUI::CustomLookAndFeel::drawPencilIcon, "Draw symbol");
    publishButton          = &publishPane.addButton(&ArsNovaUI::CustomLookAndFeel::drawUploadIcon, "Publish");

    addAndMakeVisible(undoRedoPane);

    addChildComponent(nodePane);
    addChildComponent(symbolPane);
    addChildComponent(publishPane);
}

void OrnamentBuildPanel::BuildTitlebar::resized()
{
    auto bounds = getContentBounds();

    const int spacing = juce::roundToInt(bounds.getWidth() * ArsNovaUI::Theme::contentSpacingRatio);

    nodePane.setBounds(bounds.removeFromLeft(nodePane.idealWidth(bounds.getHeight())));
    undoRedoPane.setBounds(bounds.removeFromRight(undoRedoPane.idealWidth(bounds.getHeight())));

    bounds.removeFromRight(spacing);

    publishPane.setBounds(bounds.removeFromRight(publishPane.idealWidth(bounds.getHeight())));

    bounds.removeFromRight(spacing);

    symbolPane.setBounds(bounds.removeFromRight(symbolPane.idealWidth(bounds.getHeight())));
}

#include "OrnamentBuildPanel.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../../Graph/OrnamentLibraries.h"

OrnamentBuildPanel::OrnamentBuildPanel(OrnamentLibraries& ornamentLibraries, juce::UndoManager& undoManager)
    : canvas(undoManager),
      port(canvas),
      controller(canvas, port, ornamentLibraries, undoManager),
      ornamentLibraries(ornamentLibraries),
      symbolEditor(ornamentLibraries, undoManager),
      nameEditor(undoManager)
{
    titlebar.hostButton.onClick = [this]() {
        if (controller.clickMode == OrnamentFigureController::ClickMode::PickHost) {
            controller.switchClickMode(OrnamentFigureController::ClickMode::Build);
        }
        else {
            controller.switchClickMode(OrnamentFigureController::ClickMode::PickHost);
        }
    };

    titlebar.arrowButton.onClick = [this]() {
        controller.setArrowMode(! controller.arrowMode);
    };

    controller.onClickModeChanged = [this]() {
        titlebar.hostButton.setSelected(controller.clickMode == OrnamentFigureController::ClickMode::PickHost);
    };

    controller.onArrowModeChanged = [this]() {
        titlebar.arrowButton.setSelected(controller.arrowMode);
    };

    titlebar.undoRedoPane.addButton(&CustomLookAndFeel::drawUndoIcon, "Undo", [&undoManager]() { undoManager.undo(); });
    titlebar.undoRedoPane.addButton(&CustomLookAndFeel::drawRedoIcon, "Redo", [&undoManager]() { undoManager.redo(); });

    titlebar.publishButton.onClick = [this, &undoManager]() {
        undoManager.beginNewTransaction();

        this->ornamentLibraries.publish(ornament, &undoManager);
    };

    titlebar.symbolButton.onClick = [this]() {
        if (view == BuildView::Figure) {
            view = BuildView::Symbol;
        }
        else {
            view = BuildView::Figure;
        }

        applyView();
    };

    nameEditor.autoFitText = true;
    nameEditor.fontStyle   = Theme::FontStyle::SemiBold;

    nameEditor.setFormat(std::make_unique<TextFormat>(TextFormat::labelTextLength, TextFormat::labelCharacters));
    nameEditor.setJustification(juce::Justification::centredLeft);
    nameEditor.setTooltip("Ornament name");

    port.initialViewCentre = { OrnamentCanvas::initialViewColumns * ArrowInfo::pixelsPerGridSpace, 0.0f };

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

    canvas.removeMouseListener(&controller);
}

void OrnamentBuildPanel::paint(juce::Graphics& graphics)
{
    const Theme& theme = CustomLookAndFeel::get(*this);
    const auto   body  = getLocalBounds().withTrimmedTop(titlebarHeight);

    graphics.setColour(theme.canvasColour);
    graphics.fillRect(body);

    if (ornament.isValid()) {
        return;
    }

    graphics.setColour(theme.mutedTextColour);
    graphics.setFont(theme.font(Theme::FontStyle::Regular, Theme::labelFontHeight));
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
    const int   barInset      = juce::roundToInt(titlebarHeight * titlebarInsetRatio);
    const int   contentHeight = titlebarHeight - barInset * 2;
    const int   buttonsWidth  = juce::roundToInt(contentHeight * buttonWidthRatio) * titlebarButtons + titlebar.undoRedoPane.idealWidth(contentHeight);
    const float spacingShare  = 1.0f - Theme::contentSpacingRatio * titlebarGaps;

    return barInset * 2 + static_cast<int>(std::ceil(buttonsWidth / spacingShare));
}

void OrnamentBuildPanel::applyView()
{
    titlebar.hostButton.setVisible(ornament.isValid() && view == BuildView::Figure);
    titlebar.arrowButton.setVisible(ornament.isValid() && view == BuildView::Figure);
    port.setVisible(ornament.isValid() && view == BuildView::Figure);
    nameEditor.setVisible(ornament.isValid() && view == BuildView::Figure);
    symbolEditor.setVisible(ornament.isValid() && view == BuildView::Symbol);

    titlebar.symbolButton.setSelected(view == BuildView::Symbol);

    controller.switchClickMode(OrnamentFigureController::ClickMode::Build);
    controller.setArrowMode(false);
}

void OrnamentBuildPanel::showActiveOrnament()
{
    ornament = ornamentLibraries.findOrnament(ornamentLibraries.libraries.getProperty(ValueTreeIdentifiers::ActiveOrnamentUid).toString());

    controller.ornament   = ornament;
    symbolEditor.ornament = ornament;
    canvas.figure         = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);

    titlebar.hostButton.setVisible(ornament.isValid());
    titlebar.arrowButton.setVisible(ornament.isValid());
    titlebar.publishButton.setVisible(ornament.isValid());
    titlebar.symbolButton.setVisible(ornament.isValid());

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

    titlebar.publishButton.setEnabled(ornament.isValid() && canPublish);

    canvas.triggerAsyncUpdate();
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
    : Bar(Orientation::Horizontal, titlebarInsetRatio)
{
    hostButton.painter = [this](juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state) {
        CustomLookAndFeel::get(*this).drawCircledHButton(graphics, bounds, state);
    };

    arrowButton.painter = [this](juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state) {
        CustomLookAndFeel::get(*this).drawArrowButton(graphics, bounds, state);
    };

    publishButton.painter = [this](juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state) {
        CustomLookAndFeel::get(*this).drawUploadButton(graphics, bounds, state);
    };

    symbolButton.painter = [this](juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state) {
        CustomLookAndFeel::get(*this).drawPencilButton(graphics, bounds, state);
    };

    hostButton.setText("Host");
    arrowButton.setText("Arrow");
    publishButton.setText("Publish");
    symbolButton.setText("Draw");

    hostButton.setTooltip("Choose the host: click a node to make it the host, or click the host to unmark it");
    arrowButton.setTooltip("Arrow mode: Shift-drag from a node to add a dangling arrow");
    publishButton.setTooltip("Publish this figure, so nodes play it");
    symbolButton.setTooltip("Draw this ornament's symbol");

    addAndMakeVisible(undoRedoPane);

    addChildComponent(hostButton);
    addChildComponent(arrowButton);
    addChildComponent(publishButton);
    addChildComponent(symbolButton);
}

void OrnamentBuildPanel::BuildTitlebar::resized()
{
    auto      bounds      = getContentBounds();
    const int width       = bounds.getWidth();
    const int spacing     = juce::roundToInt(width * Theme::contentSpacingRatio);
    const int buttonWidth = juce::roundToInt(bounds.getHeight() * buttonWidthRatio);

    hostButton.setBounds(bounds.removeFromLeft(buttonWidth));
    bounds.removeFromLeft(spacing);

    arrowButton.setBounds(bounds.removeFromLeft(buttonWidth));

    undoRedoPane.setBounds(bounds.removeFromRight(undoRedoPane.idealWidth(bounds.getHeight())));
    bounds.removeFromRight(spacing);

    publishButton.setBounds(bounds.removeFromRight(buttonWidth));
    bounds.removeFromRight(spacing);

    symbolButton.setBounds(bounds.removeFromRight(buttonWidth));
}

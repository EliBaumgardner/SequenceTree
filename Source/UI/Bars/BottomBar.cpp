#include "BottomBar.h"
#include "../../Graph/TypedProperties.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "../Canvas/NodeCanvas.h"

#include "../../Util/NodeInfo.h"
#include "../../Graph/OrnamentLibraries.h"
#include "../../Input/GraphDocument.h"
#include "../../Input/Gestures/NotePreviewGestures.h"
#include "../../Graph/ValueTreeIdentifiers.h"

BottomBar::BottomBar(NodeCanvas& nodeCanvas, GraphDocument& graphDocument, NotePreviewGestures& notePreviewGestures, juce::ValueTree colourPresets, OrnamentLibraries& ornamentLibraries)
    : ArsNovaUI::Bar     (nodeCanvas, Orientation::Horizontal, ArsNovaUI::Theme::contentInsetRatio),
      nodeCanvas         (nodeCanvas),
      notePreviewGestures(notePreviewGestures),
      ornamentLibraries  (ornamentLibraries),
      canvasSettings     (graphDocument.canvasSettings),
      paintPanel         (nodeCanvas, colourPresets, graphDocument.undoManager),
      countsField        (graphDocument.undoManager)
{
    auto countsFormat = std::make_unique<ArsNovaUI::NumberFormat>(NodeInfo::minimumCountLimit, NodeInfo::maximumCountLimit);

    quaverTool = &quaverPane.addButton(&ArsNovaUI::CustomLookAndFeel::drawQuaverIcon, "Note",
        [this]() {
            if (this->nodeCanvas.activeTool == NodeCanvas::CanvasTool::NotePreview) {
                this->nodeCanvas.setActiveTool(NodeCanvas::CanvasTool::None);
                return;
            }

            this->nodeCanvas.setActiveTool(NodeCanvas::CanvasTool::NotePreview);
        });

    spanTool = &toolPane.addButton(&ArsNovaUI::CustomLookAndFeel::drawDashedBoxIcon, "Node Span",
        [this]() {
            if (this->nodeCanvas.activeTool == NodeCanvas::CanvasTool::Span) {
                this->nodeCanvas.setActiveTool(NodeCanvas::CanvasTool::None);
                return;
            }

            this->nodeCanvas.setActiveTool(NodeCanvas::CanvasTool::Span);
        });

    configureOrnamentButton();

    nodeCanvas.onActiveToolChanged = [this](NodeCanvas::CanvasTool tool) {
        paintPanel.paintTool.setSelected(tool == NodeCanvas::CanvasTool::Paint);
        spanTool->setSelected(tool == NodeCanvas::CanvasTool::Span);
        quaverTool->setSelected(tool == NodeCanvas::CanvasTool::NotePreview);
        ornamentButton->setSelected(tool == NodeCanvas::CanvasTool::Ornament);
    };

    configureAxis(xAxis, "X");
    configureAxis(yAxis, "Y");

    paintPanel.paintTool.state.look = ArsNovaUI::ButtonState::Look::Raised;
    quaverTool->state.look          = ArsNovaUI::ButtonState::Look::Raised;
    spanTool->state.look            = ArsNovaUI::ButtonState::Look::Raised;
    ornamentButton->state.look      = ArsNovaUI::ButtonState::Look::Raised;

    quaverTool->onRightClick = [this]() {
        showQuaverMenu();
    };

    countsFormat->suffix      = "x";
    countsField.backdrop      = ArsNovaUI::ValueEditor::Backdrop::Field;
    countsField.wheelResponse = ArsNovaUI::ValueEditor::WheelResponse::StepValue;
    countsField.setFormat(std::move(countsFormat));
    countsField.setTooltip("Preview count");

    countsField.boundValue.referTo(notePreviewGestures.count);

    addAndMakeVisible(paintPanel);
    addAndMakeVisible(toolPane);
    addAndMakeVisible(quaverPane);
    addAndMakeVisible(bindPane);
    addAndMakeVisible(ornamentPane);
    addAndMakeVisible(countsField);

    ornamentLibraries.libraries.addListener(this);
    this->canvasSettings.addListener(this);

    showActiveOrnamentName();
}

BottomBar::~BottomBar()
{
    ornamentLibraries.libraries.removeListener(this);
    canvasSettings.removeListener(this);
}

void BottomBar::resized()
{
    auto bounds = getContentBounds();

    const int height          = bounds.getHeight();
    const int spacing         = contentSpacing;
    const int toolPaneWidth   = toolPane.idealWidth(height);
    const int bindPaneWidth   = bindPane.idealWidth(height);
    const int ornamentWidth   = ornamentPane.idealWidth(height);
    const int paneWidth       = quaverPaneWidth;
    const int paneInset       = juce::roundToInt(height * ArsNovaUI::Theme::contentInsetRatio);
    const int paneInsets      = paneInset * 2;
    const int innerHeight     = juce::jmax(0, height - paneInsets);
    const int buttonSlotWidth = juce::roundToInt(paneWidth * quaverButtonWidthRatio);
    const int buttonWidth     = juce::jlimit(0, innerHeight, buttonSlotWidth - paneInsets);

    const float textHeight    = ArsNovaUI::Theme::textHeight;
    const float captionHeight = textHeight * captionHeightRatio;

    ArsNovaUI::CustomLookAndFeel& lookAndFeel = ArsNovaUI::CustomLookAndFeel::get(*this);

    const juce::Font captionFont = lookAndFeel.font(ArsNovaUI::Theme::FontStyle::SemiBold, captionHeight).withExtraKerningFactor(captionKerningFactor);

    juce::Rectangle<int> quaverPaneBounds;

    paintPanel.sizeField.label.setFont(captionFont);
    paintPanel.flowField.label.setFont(captionFont);

    paintPanel.sizeField.label.setColour(juce::Label::textColourId, lookAndFeel.dimTextColour);
    paintPanel.flowField.label.setColour(juce::Label::textColourId, lookAndFeel.dimTextColour);

    paintPanel.sizeField.editor.setFontHeight(textHeight);
    paintPanel.flowField.editor.setFontHeight(textHeight);
    countsField                .setFontHeight(textHeight);

    paintPanel.setBounds(bounds.removeFromLeft(paintPanelWidth));

    bindPane.setBounds(bounds.removeFromRight(bindPaneWidth));
    bounds.removeFromRight(spacing);

    toolPane.setBounds(bounds.removeFromRight(toolPaneWidth));
    bounds.removeFromRight(spacing);

    ornamentPane.setBounds(bounds.removeFromRight(ornamentWidth));
    bounds.removeFromRight(spacing);

    quaverPaneBounds = bounds.removeFromRight(paneWidth);
    quaverPane.gridLayout = ArsNovaUI::ButtonPane::Grid { buttonWidth, innerHeight, 0, paneInset };

    quaverPane.setBounds(quaverPaneBounds);
    quaverPane.resized();

    quaverPaneBounds = quaverPaneBounds.reduced(0, paneInset).withTrimmedRight(paneInset);

    quaverPaneBounds.removeFromLeft(buttonSlotWidth);

    countsField.setBounds(quaverPaneBounds);
}

void BottomBar::applyDisplayMode(NodeInfo::DisplayMode mode)
{
    switch (mode) {
        case NodeInfo::DisplayMode::Pitch:    paintPanel.setPaintMode(ValueField::PaintLayer::Pitch);    break;
        case NodeInfo::DisplayMode::Velocity: paintPanel.setPaintMode(ValueField::PaintLayer::Velocity); break;
        default: break;
    }
}

int BottomBar::narrowestWidth() const
{
    const auto bounds = getContentBounds();

    const int paneGapCount = 4;
    const int height       = bounds.getHeight();

    return getWidth() - bounds.getWidth() + paintPanelWidth + bindPane.idealWidth(height) + toolPane.idealWidth(height) + ornamentPane.idealWidth(height)
           + quaverPaneWidth + contentSpacing * paneGapCount;
}

void BottomBar::configureOrnamentButton()
{
    ornamentButton = &ornamentPane.addButton(nullptr, "Ornaments", [this]() {
        if (nodeCanvas.activeTool == NodeCanvas::CanvasTool::Ornament) {
            nodeCanvas.setActiveTool(NodeCanvas::CanvasTool::None);
            return;
        }

        nodeCanvas.setActiveTool(NodeCanvas::CanvasTool::Ornament);
    });

    ornamentButton->onRightClick = [this]() { onOrnamentWindowRequested(); };

    ornamentButton->painter = [this](juce::Graphics& graphics, juce::Rectangle<float> bounds, const ArsNovaUI::ButtonState& state) {
        ArsNovaUI::CustomLookAndFeel& lookAndFeel = ArsNovaUI::CustomLookAndFeel::get(*this);

        const juce::Rectangle<float> glyphArea = lookAndFeel.drawButtonTile(graphics, bounds, state);

        const float cellSide      = glyphArea.getHeight() / ornamentGridCellsPerHeight;
        const float columnsToEdge = std::floor(glyphArea.getWidth() * 0.5f / cellSide);
        const float rowsToEdge    = std::floor(glyphArea.getHeight() * 0.5f / cellSide);
        const float firstColumn   = glyphArea.getCentreX() - columnsToEdge * cellSide;
        const float firstRow      = glyphArea.getCentreY() - rowsToEdge * cellSide;

        const juce::String activeUid = ornamentLibraries.activeOrnamentUid;

        const OrnamentProperties active(ornamentLibraries.findOrnament(activeUid));

        const juce::String symbol = active.symbol;

        const std::vector<ArsNovaUI::SymbolStroke> strokes = OrnamentLibraries::readSymbol(symbol);

        juce::Rectangle<float> extent;

        juce::Path tileOutline;

        tileOutline.addRoundedRectangle(glyphArea, ArsNovaUI::Theme::paneCornerRadius);

        graphics.saveState();
        graphics.reduceClipRegion(tileOutline);
        graphics.setColour(lookAndFeel.gridColour);

        for (float x = firstColumn; x < glyphArea.getRight(); x += cellSide) {
            graphics.drawLine(x, glyphArea.getY(), x, glyphArea.getBottom(), ArsNovaUI::Theme::borderThickness);
        }

        for (float y = firstRow; y < glyphArea.getBottom(); y += cellSide) {
            graphics.drawLine(glyphArea.getX(), y, glyphArea.getRight(), y, ArsNovaUI::Theme::borderThickness);
        }

        graphics.restoreState();

        for (const ArsNovaUI::SymbolStroke& stroke : strokes) {
            const float halfWidth = stroke.width * 0.5f;

            extent = extent.getUnion(stroke.path.getBounds().expanded(halfWidth));
        }

        if (extent.isEmpty()) {
            lookAndFeel.drawWaveButton(graphics, glyphArea, state);

            return;
        }

        const float fillSide   = juce::jmin(glyphArea.getWidth(), glyphArea.getHeight());
        const float extentSide = juce::jmax(extent.getWidth(), extent.getHeight());
        const float unitSide   = fillSide / extentSide;

        const juce::Point<float> unitOrigin = glyphArea.getCentre() - extent.getCentre() * unitSide;

        const juce::Rectangle<float> unitSquare = juce::Rectangle<float>(unitSide, unitSide).withPosition(unitOrigin);

        graphics.setColour(lookAndFeel.pressableButtonColour(state));

        lookAndFeel.drawStrokeSymbol(graphics, unitSquare, strokes);
    };
}

void BottomBar::showActiveOrnamentName()
{
    const juce::String activeUid = ornamentLibraries.activeOrnamentUid;

    const juce::ValueTree active = ornamentLibraries.findOrnament(activeUid);

    juce::String tooltip = "Ornaments";

    const OrnamentProperties activeInfo(active);

    if (active.isValid()) {
        tooltip = "Ornaments - " + activeInfo.ornamentName.get();
    }

    ornamentButton->setTooltip(tooltip);
}

void BottomBar::configureAxis(BindAxis& axis, const juce::String& text)
{
    axis.button               = &bindPane.addButton(nullptr, text + " Binding", [this, &axis]() { toggleAxis(axis); });
    axis.button->onRightClick = [this, &axis]() { showAxisMenu(axis); };

    axis.button->setText(text);

    axis.multiplierSlider.range  = { minimumBindMultiplier, maximumBindMultiplier, bindMultiplierInterval };
    axis.multiplierSlider.label  = "multiplier";
    axis.multiplierSlider.suffix = "x";

    axis.multiplierSlider.boundValue.referTo(canvasSettings.getPropertyAsValue(axis.multiplierProperty, nullptr));

    valueTreePropertyChanged(canvasSettings, axis.bindingProperty);
}

void BottomBar::toggleAxis(BindAxis& axis)
{
    const int storedBinding = canvasSettings.getProperty(axis.bindingProperty);
    const int storedTarget  = canvasSettings.getProperty(axis.targetProperty);
    const int noBinding     = static_cast<int>(ArrowInfo::Binding::NoBind);

    int toggledBinding = noBinding;

    if (storedBinding == noBinding) {
        toggledBinding = storedTarget;
    }

    canvasSettings.setProperty(axis.bindingProperty, toggledBinding, nullptr);
}

void BottomBar::showAxisMenu(BindAxis& axis)
{
    const int storedBinding = canvasSettings.getProperty(axis.bindingProperty);

    ArsNovaUI::ContextMenu menu;

    for (const BindTarget& target : bindTargets) {
        const int targetBinding = static_cast<int>(target.binding);

        const bool isChecked = storedBinding == targetBinding;

        menu.addItem(target.label, ArsNovaUI::ContextMenu::ItemKind::Toggle, [this, &axis, targetBinding]() {
            canvasSettings.setProperty(axis.targetProperty,  targetBinding, nullptr);
            canvasSettings.setProperty(axis.bindingProperty, targetBinding, nullptr);
        }, true, isChecked);
    }

    menu.addComponent(axis.multiplierSlider, multiplierSliderWidth, ArsNovaUI::Theme::popupMenuItemHeight);

    menu.show(*axis.button);
}

void BottomBar::showQuaverMenu()
{
    ArsNovaUI::ContextMenu menu;

    menu.addItem("repeat", ArsNovaUI::ContextMenu::ItemKind::Toggle, [this]() { notePreviewGestures.toggleRepeat(); }, true, notePreviewGestures.repeat);

    menu.show(*quaverTool);
}

void BottomBar::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    if (tree == canvasSettings) {
        for (BindAxis* axis : { &xAxis, &yAxis }) {
            if (property == axis->bindingProperty) {
                const int storedBinding = canvasSettings.getProperty(property);

                const bool isBound = static_cast<ArrowInfo::Binding>(storedBinding) != ArrowInfo::Binding::NoBind;

                axis->button->setSelected(isBound);
            }
        }

        return;
    }

    if (property == ValueTreeIdentifiers::ActiveOrnamentUid || property == ValueTreeIdentifiers::OrnamentName
        || property == ValueTreeIdentifiers::OrnamentSymbol) {
        showActiveOrnamentName();

        ornamentButton->repaint();
    }
}

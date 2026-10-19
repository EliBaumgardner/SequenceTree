#include "BottomBar.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "../Canvas/NodeCanvas.h"
#include "../../Audio/TraversalSession.h"
#include "../../Util/NodeInfo.h"
#include "../../Graph/OrnamentLibraries.h"

BottomBar::BottomBar(NodeCanvas& nodeCanvas, TraversalSession& traversalSession, juce::ValueTree colourPresets, juce::UndoManager& undoManager,
                     OrnamentLibraries& ornamentLibraries, juce::UndoManager& ornamentUndoManager)
    : ArsNovaUI::Bar(nodeCanvas, Orientation::Horizontal, ArsNovaUI::Theme::contentInsetRatio),
      nodeCanvas(nodeCanvas),
      traversalSession(traversalSession),
      ornamentLibraries(ornamentLibraries),
      ornamentUndoManager(ornamentUndoManager),

      paintPanel(nodeCanvas, colourPresets, undoManager),
      countsField(undoManager)
{
    ornamentLauncher.openingScale = OrnamentWindow::openingScale;

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

    countsField.editor.setTooltip("Counts");

    quaverTool->onRightClick = [this]() {
        showQuaverMenu();
    };

    countsField.label.setText("counts", juce::dontSendNotification);

    countsField.editor.setFormat(std::make_unique<ArsNovaUI::NumberFormat>(minimumCountLimit, maximumCountLimit));

    countsField.editor.wheelResponse = ArsNovaUI::ValueEditor::WheelResponse::StepValue;

    countsField.editor.boundValue.referTo(nodeCanvas.quaverCount);

    addAndMakeVisible(paintPanel);
    addAndMakeVisible(toolPane);
    addAndMakeVisible(quaverPane);
    addAndMakeVisible(bindPane);
    addAndMakeVisible(ornamentPane);
    addAndMakeVisible(countsField);

    ornamentLibraries.libraries.addListener(this);

    showActiveOrnamentName();
}

BottomBar::~BottomBar()
{
    ornamentLibraries.libraries.removeListener(this);
}

void BottomBar::resized()
{
    auto                 bounds          = getContentBounds();
    const int            height          = bounds.getHeight();
    const int            spacing         = contentSpacing;
    const int            toolPaneWidth   = toolPane.idealWidth(height);
    const int            bindPaneWidth   = bindPane.idealWidth(height);
    const int            ornamentWidth   = ornamentPane.idealWidth(height);
    const int            paneWidth       = quaverPaneWidth;
    const int            paneInset       = juce::roundToInt(height * ArsNovaUI::Theme::contentInsetRatio);
    const int            innerHeight     = juce::jmax(0, height - paneInset * 2);
    const int            buttonSlotWidth = juce::roundToInt(paneWidth * quaverButtonWidthRatio);
    const int            buttonWidth     = juce::jlimit(0, innerHeight, buttonSlotWidth - paneInset * 2);
    const int            labelWidth      = juce::roundToInt(paneWidth * countsLabelWidthRatio);
    const float          textHeight      = ArsNovaUI::Theme::textHeight;
    const juce::Font     textFont        = ArsNovaUI::CustomLookAndFeel::get(*this).font(ArsNovaUI::Theme::FontStyle::Regular, textHeight);

    juce::Rectangle<int> quaverPaneBounds;

    paintPanel.sizeField.label.setFont(textFont);
    paintPanel.flowField.label.setFont(textFont);
    countsField.label.setFont(textFont);

    paintPanel.sizeField.editor.setFontHeight(textHeight);
    paintPanel.flowField.editor.setFontHeight(textHeight);
    countsField.editor         .setFontHeight(textHeight);

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

    countsField.labelWidth = labelWidth;
    countsField.gap        = paneInset;

    countsField.setBounds(quaverPaneBounds);
}

void BottomBar::applyDisplayMode(NodeDisplayMode mode)
{
    switch (mode) {
        case NodeDisplayMode::Pitch:    paintPanel.setPaintMode(ValueField::PaintLayer::Pitch);    break;
        case NodeDisplayMode::Velocity: paintPanel.setPaintMode(ValueField::PaintLayer::Velocity); break;
        default: break;
    }
}

int BottomBar::narrowestWidth() const
{
    const auto bounds = getContentBounds();
    const int  height = bounds.getHeight();

    return getWidth() - bounds.getWidth() + paintPanelWidth + bindPane.idealWidth(height) + toolPane.idealWidth(height) + ornamentPane.idealWidth(height)
           + quaverPaneWidth + contentSpacing * 4;
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

    ornamentButton->onRightClick = [this]() {
        ornamentLibraries.rescan(ornamentUndoManager);

        ornamentLauncher.show(*this);
    };


    ornamentButton->painter = [this](juce::Graphics& graphics, juce::Rectangle<float> bounds, const ArsNovaUI::ButtonState& state) {
        ArsNovaUI::CustomLookAndFeel& lookAndFeel = ArsNovaUI::CustomLookAndFeel::get(*this);
        const juce::Rectangle<float>  glyphArea   = lookAndFeel.drawButtonTile(graphics, bounds, state);
        const float                   glyphSide   = glyphArea.getHeight() * ArsNovaUI::Theme::glyphSizeRatio;
        const juce::String            activeUid   = ornamentLibraries.libraries.getProperty(ValueTreeIdentifiers::ActiveOrnamentUid).toString();
        const juce::String            symbol      = ornamentLibraries.findOrnament(activeUid).getProperty(ValueTreeIdentifiers::OrnamentSymbol).toString();

        if (symbol.isEmpty()) {
            lookAndFeel.drawWaveButton(graphics, glyphArea, state);

            return;
        }

        graphics.setColour(lookAndFeel.pressableButtonColour(state));

        lookAndFeel.drawStrokeSymbol(graphics, glyphArea.withSizeKeepingCentre(glyphSide, glyphSide), OrnamentLibraries::readSymbol(symbol));
    };
}

void BottomBar::showActiveOrnamentName()
{
    const juce::String    activeUid = ornamentLibraries.libraries.getProperty(ValueTreeIdentifiers::ActiveOrnamentUid).toString();
    const juce::ValueTree active    = ornamentLibraries.findOrnament(activeUid);
    juce::String          tooltip   = "Ornaments";

    if (active.isValid()) {
        tooltip = "Ornaments - " + active.getProperty(ValueTreeIdentifiers::OrnamentName).toString();
    }

    ornamentButton->setTooltip(tooltip);
}

void BottomBar::configureAxis(BindAxis& axis, const juce::String& text)
{
    const ArrowInfo& arrowInfo = nodeCanvas.arrowManager.currentArrowInfo;

    axis.button = &bindPane.addButton(nullptr, text + " Binding", [this, &axis]() { toggleAxis(axis); });

    axis.button->onRightClick = [this, &axis]() { showAxisMenu(axis); };


    axis.button->setText(text);

    if (arrowInfo.*axis.binding != ArrowBinding::NoBind) {
        axis.target = arrowInfo.*axis.binding;
    }

    axis.multiplierSlider.range  = { minimumBindMultiplier, maximumBindMultiplier, bindMultiplierInterval };
    axis.multiplierSlider.label  = "multiplier";
    axis.multiplierSlider.suffix = "x";

    axis.multiplierSlider.boundValue.setValue(arrowInfo.*axis.multiplier);

    axis.multiplierSlider.onValueChange = [this, &axis]() {
        nodeCanvas.arrowManager.currentArrowInfo.*axis.multiplier = static_cast<double>(axis.multiplierSlider.boundValue.getValue());
    };

    axis.button->setSelected(arrowInfo.*axis.binding != ArrowBinding::NoBind);
}

void BottomBar::toggleAxis(BindAxis& axis)
{
    ArrowInfo& arrowInfo = nodeCanvas.arrowManager.currentArrowInfo;

    if (arrowInfo.*axis.binding == ArrowBinding::NoBind) {
        arrowInfo.*axis.binding = axis.target;
    }
    else {
        arrowInfo.*axis.binding = ArrowBinding::NoBind;
    }

    axis.button->setSelected(arrowInfo.*axis.binding != ArrowBinding::NoBind);
}

void BottomBar::showAxisMenu(BindAxis& axis)
{
    const ArrowBinding     binding = nodeCanvas.arrowManager.currentArrowInfo.*axis.binding;
    ArsNovaUI::ContextMenu menu;

    for (const BindTarget& target : bindTargets) {
        menu.addItem(target.label, ArsNovaUI::ContextMenu::ItemKind::Toggle, [this, &axis, bound = target.binding]() {
            axis.target                                        = bound;
            nodeCanvas.arrowManager.currentArrowInfo.*axis.binding = bound;

            axis.button->setSelected(true);
        }, true, binding == target.binding);
    }

    menu.addComponent(axis.multiplierSlider, multiplierSliderWidth, ArsNovaUI::Theme::popupMenuItemHeight);

    menu.show(*axis.button);
}

void BottomBar::showQuaverMenu()
{
    ArsNovaUI::ContextMenu menu;

    menu.addItem("repeat", ArsNovaUI::ContextMenu::ItemKind::Toggle, [this]() {
        nodeCanvas.quaverRepeat = ! nodeCanvas.quaverRepeat;

        traversalSession.previewRequests.push({
            .kind   = RTPreviewRequest::Kind::SetRepeat,
            .repeat = nodeCanvas.quaverRepeat
        }); }, true, nodeCanvas.quaverRepeat);

    menu.show(*quaverTool);
}

void BottomBar::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    if (property == ValueTreeIdentifiers::ActiveOrnamentUid || property == ValueTreeIdentifiers::OrnamentName
        || property == ValueTreeIdentifiers::OrnamentSymbol) {
        showActiveOrnamentName();

        ornamentButton->repaint();
    }
}

#include "OrnamentWindow.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

OrnamentWindow::OrnamentWindow(ArsNovaUI::CustomLookAndFeel& lookAndFeel, OrnamentLibraries& ornamentLibraries, juce::UndoManager& undoManager)
    : undoManager(undoManager),
      buildPanel(ornamentLibraries, undoManager),
      selectionPanel(ornamentLibraries, undoManager)
{
    setLookAndFeel(&lookAndFeel);
    setWantsKeyboardFocus(true);

    selectionPanel.resizer.onWidthDragged = [this](int newWidth) {
        selectionPanelWidth = newWidth;

        resized();
    };

    addAndMakeVisible(buildPanel);
    addAndMakeVisible(selectionPanel);
}

OrnamentWindow::~OrnamentWindow()
{
    setLookAndFeel(nullptr);
}

void OrnamentWindow::paint(juce::Graphics& graphics)
{
    const ArsNovaUI::Theme& theme = ArsNovaUI::CustomLookAndFeel::get(*this);

    graphics.setColour(theme.surfaceColour);
    graphics.fillRect(getLocalBounds());

    graphics.setColour(theme.borderColour);
    graphics.drawRect(getLocalBounds(), 1);
}

void OrnamentWindow::resized()
{
    auto      bounds        = getLocalBounds();
    const int minPanelWidth = selectionPanel.narrowestSize().x;
    const int maxPanelWidth = juce::jmax(minPanelWidth, bounds.getWidth() - buildPanel.minimumWidth());
    const int panelWidth    = juce::jlimit(minPanelWidth, maxPanelWidth, selectionPanelWidth);

    selectionPanel.setBounds(bounds.removeFromRight(panelWidth));
    buildPanel.setBounds(bounds);
}

bool OrnamentWindow::keyPressed(const juce::KeyPress& key)
{
    const juce::ModifierKeys command      = juce::ModifierKeys::commandModifier;
    const juce::ModifierKeys commandShift = juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier;

    if (key == juce::KeyPress::escapeKey) {
        if (buildPanel.controller.clickMode == OrnamentFigureController::ClickMode::PickHost) {
            buildPanel.controller.switchClickMode(OrnamentFigureController::ClickMode::Build);

            return true;
        }

        if (buildPanel.controller.arrowMode) {
            buildPanel.controller.setArrowMode(false);

            return true;
        }
    }

    if (key == juce::KeyPress('z', command, 0)) {
        undoManager.undo();

        return true;
    }

    if (key == juce::KeyPress('z', commandShift, 0) || key == juce::KeyPress('y', command, 0)) {
        undoManager.redo();

        return true;
    }

    return false;
}

juce::Point<int> OrnamentWindow::narrowestSize() const
{
    const juce::Point<int> selectionSize = selectionPanel.narrowestSize();
    const int              buildHeight   = OrnamentBuildPanel::titlebarHeight + OrnamentBuildPanel::nameInset * 2 + OrnamentBuildPanel::nameHeight;

    return { buildPanel.minimumWidth() + selectionSize.x, juce::jmax(buildHeight, selectionSize.y) };
}

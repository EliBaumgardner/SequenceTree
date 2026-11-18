#include "OrnamentWindow.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

OrnamentWindow::OrnamentWindow(ArsNovaUI::CustomLookAndFeel& lookAndFeel, OrnamentLibraries& ornamentLibraries, juce::ValueTree canvasSettings, juce::UndoManager& undoManager,
                               juce::ApplicationCommandManager& commandManager)
    : undoManager   (undoManager),
      commandManager(commandManager),
      buildPanel    (ornamentLibraries, canvasSettings, undoManager),
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

    addKeyListener(commandManager.getKeyMappings());
}

OrnamentWindow::~OrnamentWindow()
{
    removeKeyListener(commandManager.getKeyMappings());
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

void OrnamentWindow::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    const juce::Array<juce::CommandID> ornamentCommands { juce::StandardApplicationCommandIDs::undo, juce::StandardApplicationCommandIDs::redo };

    commands.addArray(ornamentCommands);
}

void OrnamentWindow::getCommandInfo(juce::CommandID, juce::ApplicationCommandInfo& info)
{
    info.setActive(true);
}

bool OrnamentWindow::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey) {
        if (buildPanel.controller.clickMode == OrnamentFigureController::ClickMode::PickHost) {
            buildPanel.controller.switchClickMode(OrnamentFigureController::ClickMode::Build);

            return true;
        }

        if (buildPanel.controller.arrowGestures.arrowMode) {
            buildPanel.controller.setArrowMode(false);

            return true;
        }
    }

    return false;
}

bool OrnamentWindow::perform(const InvocationInfo& info)
{
    if (info.commandID == juce::StandardApplicationCommandIDs::undo) {
        undoManager.undo();

        return true;
    }

    if (info.commandID == juce::StandardApplicationCommandIDs::redo) {
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

juce::ApplicationCommandTarget* OrnamentWindow::getNextCommandTarget()
{
    return nullptr;
}

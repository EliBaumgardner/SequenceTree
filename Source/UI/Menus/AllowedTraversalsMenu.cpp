#include "AllowedTraversalsMenu.h"
#include "../../Graph/GraphState.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../Editors/Formats/ValueFormat.h"

#include <algorithm>

AllowedTraversalsMenu::AllowedTraversalsMenu(CustomLookAndFeel& lookAndFeel, GraphState& graphState, juce::UndoManager& undoManager, juce::ValueTree connection)
    : undoManager(undoManager), connection(connection)
{
    const juce::ValueTree     traversalMap = graphState.traversals.map;
    std::vector<TraversalKey> keys;

    setLookAndFeel(&lookAndFeel);

    for (int traversalIndex = 0; traversalIndex < traversalMap.getNumChildren(); ++traversalIndex) {
        const juce::ValueTree traversalData = traversalMap.getChild(traversalIndex);

        if (traversalData.getType() != ValueTreeIdentifiers::TraversalData) {
            continue;
        }

        keys.push_back({ static_cast<int>(traversalData.getProperty(ValueTreeIdentifiers::TraversalId)), 0 });
    }

    graphState.traversals.collectKeys(keys);

    std::ranges::sort(keys);

    for (const TraversalKey& key : keys) {
        TraversalRow row;

        row.key          = key;
        row.label        = std::make_unique<CaptionLabel>();
        row.toggle       = std::make_unique<ToggleButton>();
        row.toggle->isOn = isTraversalEnabled(key);

        row.toggle->onToggle = [this, key](bool enabled) {
            setTraversalEnabled(key, enabled);
        };

        row.label->setText("Traversal " + TraversalFlagFormat::describe(key), juce::dontSendNotification);
        row.label->setFont(lookAndFeel.font(Theme::FontStyle::Regular, Theme::labelFontHeight));

        addAndMakeVisible(row.label.get());
        addAndMakeVisible(row.toggle.get());

        rows.push_back(std::move(row));
    }
}

void AllowedTraversalsMenu::paint(juce::Graphics& graphics)
{
    const Theme& theme = CustomLookAndFeel::get(*this);

    graphics.fillAll(theme.surfaceColour);

    graphics.setColour(theme.borderColour);
    graphics.drawRect(getLocalBounds(), 1);
}

void AllowedTraversalsMenu::resized()
{
    auto bounds = getLocalBounds().reduced(contentInset);

    for (auto& row : rows) {
        auto rowArea = bounds.removeFromTop(rowHeight).reduced(0, 2);

        row.toggle->setBounds(rowArea.removeFromRight(toggleWidth));

        rowArea.removeFromRight(6);

        row.label->setBounds(rowArea);
    }
}

bool AllowedTraversalsMenu::isTraversalEnabled(const TraversalKey& key) const
{
    const juce::ValueTree disabled = connection.getChildWithName(ValueTreeIdentifiers::DisabledTraversalIds);

    if (!disabled.isValid()) {
        return true;
    }

    return !TraversalState::findReference(disabled, key).isValid();
}

void AllowedTraversalsMenu::setTraversalEnabled(const TraversalKey& key, bool enabled)
{
    juce::ValueTree disabled = connection.getChildWithName(ValueTreeIdentifiers::DisabledTraversalIds);

    if (!connection.isValid()) {
        return;
    }

    undoManager.beginNewTransaction();

    if (enabled) {
        if (!disabled.isValid()) {
            return;
        }

        const juce::ValueTree entry = TraversalState::findReference(disabled, key);

        if (entry.isValid()) {
            disabled.removeChild(entry, &undoManager);
        }
    }
    else {
        if (!disabled.isValid()) {
            disabled = juce::ValueTree(ValueTreeIdentifiers::DisabledTraversalIds);

            connection.addChild(disabled, -1, &undoManager);
        }

        if (!TraversalState::findReference(disabled, key).isValid()) {
            juce::ValueTree entry {ValueTreeIdentifiers::TraversalId};

            entry.setProperty(ValueTreeIdentifiers::TraversalId,       key.typeId,   &undoManager);
            entry.setProperty(ValueTreeIdentifiers::TraversalInstance, key.instance, &undoManager);

            disabled.addChild(entry, -1, &undoManager);
        }
    }
}

int AllowedTraversalsMenu::getIdealHeight() const
{
    return contentInset * 2 + rowHeight * juce::jmax(1, static_cast<int>(rows.size()));
}

void AllowedTraversalsMenu::ToggleButton::paint(juce::Graphics& graphics)
{
    const Theme&       theme      = CustomLookAndFeel::get(*this);
    const auto         bounds     = getLocalBounds().toFloat().reduced(2.0f);
    const juce::Colour onColour   = theme.accentColour;
    const juce::Colour offColour  = theme.raisedColour;
    juce::Colour       textColour = theme.mutedTextColour;
    juce::String       stateText  = "off";

    if (isOn) {
        graphics.setColour(onColour);
    }
    else {
        graphics.setColour(offColour);
    }

    graphics.fillRoundedRectangle(bounds, Theme::paneCornerRadius);

    graphics.setColour(theme.borderStrongColour);
    graphics.drawRoundedRectangle(bounds, Theme::paneCornerRadius, Theme::borderThickness);

    if (isOn) {
        textColour = theme.onAccentColour;
        stateText  = "on";
    }

    graphics.setColour(textColour);
    graphics.setFont(theme.font(Theme::FontStyle::Regular, Theme::labelFontHeight));
    graphics.drawText(stateText, getLocalBounds(), juce::Justification::centred);
}

void AllowedTraversalsMenu::ToggleButton::mouseDown(const juce::MouseEvent&)
{
    isOn = !isOn;

    repaint();

    if (onToggle) {
        onToggle(isOn);
    }
}

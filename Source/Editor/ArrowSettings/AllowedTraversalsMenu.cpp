#include "AllowedTraversalsMenu.h"
#include "Processor/Document/Graph/GraphState.h"
#include "Processor/Document/TypedProperties.h"
#include "Processor/Document/ValueTreeIdentifiers.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "Editor/Nodes/TraversalFlagFormat.h"

#include <algorithm>

AllowedTraversalsMenu::AllowedTraversalsMenu(ArsNovaUI::CustomLookAndFeel& lookAndFeel, GraphState& graphState, juce::UndoManager& undoManager, juce::ValueTree connection)
    : undoManager(undoManager),
      connection (connection)
{
    const juce::ValueTree     traversalMap = graphState.traversals.map;
    std::vector<TraversalKey> keys;

    setLookAndFeel(&lookAndFeel);

    for (int traversalIndex = 0; traversalIndex < traversalMap.getNumChildren(); ++traversalIndex) {
        const juce::ValueTree traversalData = traversalMap.getChild(traversalIndex);

        const TraversalProperties traversal(traversalData);

        if (traversalData.getType() != ValueTreeIdentifiers::TraversalData) {
            continue;
        }

        keys.push_back({ traversal.traversalId, 0 });
    }

    graphState.traversals.collectKeys(keys);

    std::ranges::sort(keys);

    for (const TraversalKey& key : keys) {
        TraversalRow row;

        row.key          = key;
        row.label        = std::make_unique<ArsNovaUI::CaptionLabel>();
        row.toggle       = std::make_unique<ArsNovaUI::ToggleSwitch>();
        row.toggle->isOn = isTraversalEnabled(key);

        row.toggle->onToggle = [this, key](bool enabled) {
            setTraversalEnabled(key, enabled);
        };

        row.label->setText("Traversal " + TraversalFlagFormat::describe(key), juce::dontSendNotification);
        row.label->setFont(lookAndFeel.font(ArsNovaUI::Theme::FontStyle::Regular, ArsNovaUI::Theme::labelFontHeight));

        addAndMakeVisible(row.label.get());
        addAndMakeVisible(row.toggle.get());

        rows.push_back(std::move(row));
    }
}

void AllowedTraversalsMenu::paint(juce::Graphics& graphics)
{
    const ArsNovaUI::Theme& theme = ArsNovaUI::CustomLookAndFeel::get(*this);

    graphics.fillAll(theme.surfaceColour);

    graphics.setColour(theme.borderColour);
    graphics.drawRect(getLocalBounds(), 1);
}

void AllowedTraversalsMenu::resized()
{
    auto bounds = getLocalBounds().reduced(contentInset);

    for (auto& row : rows) {
        auto rowArea = bounds.removeFromTop(rowHeight).reduced(0, rowInset);

        row.toggle->setBounds(rowArea.removeFromRight(toggleWidth));

        rowArea.removeFromRight(toggleGap);

        row.label->setBounds(rowArea);
    }
}

int AllowedTraversalsMenu::getIdealHeight() const
{
    const int rowCount  = static_cast<int>(rows.size());
    const int shownRows = juce::jmax(1, rowCount);

    return contentInset * 2 + rowHeight * shownRows;
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

            TraversalReferenceProperties reference(entry);

            reference.traversalId      .setValue(key.typeId,   &undoManager);
            reference.traversalInstance.setValue(key.instance, &undoManager);

            disabled.addChild(entry, -1, &undoManager);
        }
    }
}


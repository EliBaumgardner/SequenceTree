#include "NodeMenu.h"
#include "../../Graph/GraphState.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../Canvas/NodeCanvas.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../Node/Node.h"

NodeMenu::NodeMenu(NodeCanvas& nodeCanvas, GraphState& graphState, TraversalRuleState& traversalRuleState, AudioSnapshotPublisher& snapshots,
                   juce::ValueTree colourPresets, juce::UndoManager& undoManager)
    : graphState(graphState),
      traversalRuleState(traversalRuleState),
      snapshots(snapshots),
      undoManager(undoManager),
      colourSelector(colourPresets, undoManager)
{
    editTraversalRulesButton.painter = [this](juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state) {
        CustomLookAndFeel::get(*this).drawRulesButton(graphics, bounds, state, Theme::textHeight);
    };

    colourLabel.setText("COL", juce::dontSendNotification);

    for (const LabeledRow& row : labeledRows) {
        auto format = std::make_unique<NumberFormat>(row.minimum, row.maximum);

        format->prefix = row.prefix;
        format->suffix = row.suffix;

        row.field.editor.setFormat(std::move(format));
        row.field.editor.setTooltip(row.tooltip);

        row.field.label.setText(row.labelText, juce::dontSendNotification);

        addChildComponent(row.field);
    }

    editTraversalRulesButton.setText("edit traversal rules");

    editTraversalRulesButton.onClick = [this]() { traversalRulesLauncher.show(*this); };

    colourSelector.onColourPicked = [this](juce::Colour pickedColour) {
        if (selectedNodeId < 0) {
            return;
        }

        this->graphState.setNodeColour(selectedNodeId, pickedColour.toString(), &this->undoManager);
    };

    addChildComponent(colourLabel);
    addChildComponent(colourSelector);
    addChildComponent(editTraversalRulesButton);

    nodeCanvas.nodeManager.nodeSelectedListeners.push_back([this](Node* node, bool selected) {
        if (selected && node != nullptr) {
            selectedNodeId        = node->nodeId;
            colourSelector.colour = node->nodeColour;

            colourLabel.setVisible(true);
            colourSelector.setVisible(true);
            editTraversalRulesButton.setVisible(true);

            bindToNode(node);
        }
        else {
            selectedNodeId = -1;

            colourLabel.setVisible(false);
            colourSelector.setVisible(false);
            editTraversalRulesButton.setVisible(false);

            for (const LabeledRow& row : labeledRows) {
                row.field.setVisible(false);
            }
        }

        resized();
        repaint();
    });
}

void NodeMenu::paint(juce::Graphics& graphics)
{
    CustomLookAndFeel& lookAndFeel = CustomLookAndFeel::get(*this);
    const int          spacing     = juce::roundToInt(Theme::barHeight * Theme::menuSpacingRatio);

    if (selectedNodeId >= 0) {
        lookAndFeel.drawPane(graphics, getLocalBounds().toFloat());

        return;
    }

    graphics.setColour(lookAndFeel.mutedTextColour);
    graphics.setFont(lookAndFeel.font(Theme::FontStyle::Regular, Theme::textHeight));
    graphics.drawFittedText("Left click a node to edit it", getLocalBounds().reduced(spacing), juce::Justification::centred, 2);
}

void NodeMenu::resized()
{
    const int        barHeight       = Theme::barHeight;
    const int        spacing         = juce::roundToInt(barHeight * Theme::menuSpacingRatio);
    const int        rowHeight       = juce::roundToInt(barHeight * Theme::menuRowHeightRatio);
    const float      textHeight      = Theme::textHeight;
    const juce::Font textFont        = CustomLookAndFeel::get(*this).font(Theme::FontStyle::Regular, textHeight);
    auto             bounds          = getLocalBounds().reduced(spacing).withWidth(Theme::menuContentWidth);

    editTraversalRulesButton.setBounds(bounds.removeFromBottom(juce::roundToInt(barHeight * Theme::menuButtonHeightRatio)));

    auto colourRowBounds = bounds.removeFromTop(rowHeight);

    colourLabel.setFont(textFont);
    colourLabel.setBounds(colourRowBounds.removeFromLeft(colourRowBounds.getWidth() / 3));
    colourSelector.setBounds(colourRowBounds);

    bounds.removeFromTop(spacing);

    for (const auto& row : labeledRows) {
        row.field.label.setFont(textFont);
        row.field.editor.setFontHeight(textHeight);

        if (!row.field.isVisible()) {
            continue;
        }

        auto rowBounds = bounds.removeFromTop(rowHeight);

        row.field.labelWidth = rowBounds.getWidth() / 3;

        row.field.setBounds(rowBounds);

        bounds.removeFromTop(spacing);
    }
}

int NodeMenu::narrowestHeight() const
{
    const int barHeight    = Theme::barHeight;
    const int spacing      = juce::roundToInt(barHeight * Theme::menuSpacingRatio);
    const int rowHeight    = juce::roundToInt(barHeight * Theme::menuRowHeightRatio);
    const int buttonHeight = juce::roundToInt(barHeight * Theme::menuButtonHeightRatio);
    const int rowCount     = static_cast<int>(labeledRows.size());

    return spacing * 2 + buttonHeight + rowHeight + rowCount * (spacing + rowHeight);
}

void NodeMenu::bindToNode(const Node* node)
{
    const bool hasNodeTree = node->nodeValueTree.isValid();
    const bool hasMidiNote = node->midiNoteData.isValid();
    const bool hasSubLoop  = hasNodeTree && ! node->isAlternativeNode;

    for (const LabeledRow& row : labeledRows) {
        juce::ValueTree boundTree = node->nodeValueTree;
        bool            isShown   = hasNodeTree;

        if (row.source == RowSource::SubLoop) {
            isShown = hasSubLoop;
        }

        if (row.source == RowSource::MidiNote) {
            boundTree = node->midiNoteData;
            isShown   = hasMidiNote;
        }

        if (isShown) {
            row.field.editor.bindEditor(boundTree, row.property);
        }

        row.field.setVisible(isShown);
    }
}

#include "NodeMenu.h"
#include "../../Graph/GraphState.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../Canvas/NodeCanvas.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../Node/Node.h"

NodeMenu::NodeMenu(const ApplicationContext& context)
    : applicationContext(context)
{
    setLookAndFeel(applicationContext.lookAndFeel);

    editTraversalRulesButton.painter = [this](juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state) {
        CustomLookAndFeel::get(*this).drawTextButton(graphics, bounds, state, CustomLookAndFeel::get(*this).textHeight);
    };

    editTraversalRulesButton.setLookAndFeel(context.lookAndFeel);

    colourLabel.setText("COL", juce::dontSendNotification);

    colourSelector.setEnabled(false);

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

    editTraversalRulesButton.onClick = [this]() { traversalRulesLauncher.show(); };

    colourSelector.onColourPicked = [this](juce::Colour pickedColour) {
        if (selectedNodeId < 0) {
            return;
        }

        applicationContext.graphState->setNodeColour(selectedNodeId, pickedColour.toString(), applicationContext.undoManager);
    };

    addAndMakeVisible(colourLabel);
    addAndMakeVisible(colourSelector);
    addAndMakeVisible(editTraversalRulesButton);

    applicationContext.canvas->nodeManager.nodeSelectedListeners.push_back([this](Node* node, bool selected) {
        if (selected && node != nullptr) {
            selectedNodeId        = node->nodeId;
            colourSelector.colour = node->nodeColour;

            colourSelector.setEnabled(true);

            bindToNode(node);
        }
        else {
            selectedNodeId = -1;

            colourSelector.setEnabled(false);

            for (const LabeledRow& row : labeledRows) {
                row.field.setVisible(false);
            }
        }

        resized();
        repaint();
    });
}

NodeMenu::~NodeMenu()
{
    setLookAndFeel(nullptr);
}

void NodeMenu::paint(juce::Graphics& graphics)
{
    const Theme& theme = CustomLookAndFeel::get(*this);

    graphics.setColour(theme.baseDarkColour2);
    graphics.fillRect(getLocalBounds().toFloat());
}

void NodeMenu::resized()
{
    const int        barHeight       = static_cast<int>(getHeight() * Theme::barHeightRatio);
    const int        spacing         = juce::roundToInt(barHeight * Theme::menuSpacingRatio);
    const int        rowHeight       = juce::roundToInt(barHeight * Theme::menuRowHeightRatio);
    const float      textHeight      = CustomLookAndFeel::get(*this).textHeight;
    const juce::Font textFont        { juce::FontOptions(textHeight) };
    auto             bounds          = getLocalBounds().reduced(spacing);

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

//
// Created by Eli Baumgardner on 7/20/26.
//

#include "AllowedTraversalsMenu.h"
#include "../Graph/ValueTreeState.h"
#include "../Graph/ValueTreeIdentifiers.h"
#include "../Graph/RTGraphBuilder.h"
#include "Theme/CustomLookAndFeel.h"

void AllowedTraversalsMenu::ToggleButton::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);

    juce::Colour onColour  = juce::Colour::fromRGB(195, 174, 132);
    juce::Colour offColour = juce::Colour::fromRGB(40, 40, 38);

    g.setColour(isOn ? onColour : offColour);
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    g.setColour(isOn ? juce::Colours::black.withAlpha(0.7f) : juce::Colour::fromRGB(195, 174, 132));
    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    g.drawText(isOn ? "on" : "off", getLocalBounds(), juce::Justification::centred);
}

void AllowedTraversalsMenu::ToggleButton::mouseDown(const juce::MouseEvent& e) {
    isOn = !isOn;
    repaint();

    if (onToggle) {
        onToggle(isOn);
    }
}

AllowedTraversalsMenu::AllowedTraversalsMenu(ApplicationContext& context, juce::ValueTree connection)
    : applicationContext(context), connection(connection)
{
    setLookAndFeel(context.lookAndFeel);

    for (int i = 0; i < applicationContext.valueTreeState->traversalMap.getNumChildren(); ++i) {
        juce::ValueTree traversalData = applicationContext.valueTreeState->traversalMap.getChild(i);

        if (traversalData.getType() != ValueTreeIdentifiers::TraversalData) {
            continue;
        }

        int traversalId = traversalData.getProperty(ValueTreeIdentifiers::TraversalId);

        TraversalRow row;
        row.traversalId = traversalId;

        row.label = std::make_unique<juce::Label>();
        row.label->setText("Traversal " + juce::String(traversalId), juce::dontSendNotification);
        row.label->setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        row.label->setFont(juce::Font(juce::FontOptions(9.0f)));
        row.label->setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(row.label.get());

        row.toggle = std::make_unique<ToggleButton>();
        row.toggle->isOn = isTraversalEnabled(traversalId);
        row.toggle->onToggle = [this, traversalId](bool enabled) {
            setTraversalEnabled(traversalId, enabled);
        };
        addAndMakeVisible(row.toggle.get());

        rows.push_back(std::move(row));
    }
}

int AllowedTraversalsMenu::getIdealHeight() const {
    return contentInset * 2 + rowHeight * juce::jmax(1, (int) rows.size());
}

bool AllowedTraversalsMenu::isTraversalEnabled(int traversalId) const {
    juce::ValueTree disabled = connection.getChildWithName(ValueTreeIdentifiers::DisabledTraversalIds);

    if (!disabled.isValid()) {
        return true;
    }

    return !disabled.getChildWithProperty(ValueTreeIdentifiers::TraversalId, traversalId).isValid();
}

void AllowedTraversalsMenu::setTraversalEnabled(int traversalId, bool enabled) {
    if (!connection.isValid()) {
        return;
    }

    juce::UndoManager* undoManager = applicationContext.undoManager;
    undoManager->beginNewTransaction();

    juce::ValueTree disabled = connection.getChildWithName(ValueTreeIdentifiers::DisabledTraversalIds);

    if (enabled) {
        if (!disabled.isValid()) {
            return;
        }

        juce::ValueTree entry = disabled.getChildWithProperty(ValueTreeIdentifiers::TraversalId, traversalId);
        if (entry.isValid()) {
            disabled.removeChild(entry, undoManager);
        }
    }
    else {
        if (!disabled.isValid()) {
            disabled = juce::ValueTree(ValueTreeIdentifiers::DisabledTraversalIds);
            connection.addChild(disabled, -1, undoManager);
        }

        if (!disabled.getChildWithProperty(ValueTreeIdentifiers::TraversalId, traversalId).isValid()) {
            juce::ValueTree entry {ValueTreeIdentifiers::TraversalId};
            entry.setProperty(ValueTreeIdentifiers::TraversalId, traversalId, undoManager);
            disabled.addChild(entry, -1, undoManager);
        }
    }

    if (applicationContext.rtGraphBuilder != nullptr) {
        juce::ValueTree ownerNode = connection.getParent().getParent();
        if (ownerNode.isValid()) {
            applicationContext.rtGraphBuilder->makeRTGraph(ownerNode);
        }
    }
}

void AllowedTraversalsMenu::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour::fromRGB(30, 30, 30));

    g.setColour(juce::Colours::black);
    g.drawRect(getLocalBounds(), 1);
}

void AllowedTraversalsMenu::resized() {
    auto bounds = getLocalBounds().reduced(contentInset);

    for (auto& row : rows) {
        auto rowArea = bounds.removeFromTop(rowHeight).reduced(0, 2);
        row.toggle->setBounds(rowArea.removeFromRight(toggleWidth));
        rowArea.removeFromRight(6);
        row.label->setBounds(rowArea);
    }
}

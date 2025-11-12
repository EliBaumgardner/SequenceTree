//
// Created by Eli Baumgardner on 11/9/25.
//

#ifndef SEQUENCETREE_TITLEBARBUTTONS_H
#define SEQUENCETREE_TITLEBARBUTTONS_H

#include "../Util/PluginModules.h"
#include "../Util/PluginContext.h"
#include "../Node/NodeCanvas.h"

static constexpr float buttonBoundsReduction = 3.0f;
static constexpr float buttonBorderThickness = 2.0f;
static constexpr float buttonContentBounds = 4.0f;
static constexpr float buttonSelectedThickness = 4.0f;


class PlayButton : public juce::Button {

public:

    PlayButton() : juce::Button("button") { setLookAndFeel(ComponentContext::lookAndFeel); }

    void paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown) override
    {
        if (auto* customLookAndFeel = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel())) { customLookAndFeel->drawPlayButton(g,isMouseOver,isButtonDown,*this); }
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        juce::Button::mouseDown(event);
        isOn = !isOn;
        repaint();
    }

    bool isOn = true;
};



class SyncButton : public juce::Button {

public:
    SyncButton() : juce::Button("button") { setLookAndFeel(ComponentContext::lookAndFeel); }

    void paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown) override
    {
        if (auto* customLookAndFeel = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel())) { customLookAndFeel->drawSyncButton(g, isMouseOver, isButtonDown, *this); }
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        juce::Button::mouseDown(event);
        isSynced = !isSynced;
        repaint();
    }

    bool isSynced = true;
};



class NodeButton : public juce::Component {

    public:

    std::function<void()> onClick;
    bool isSelected = false;

    NodeButton() { setLookAndFeel(ComponentContext::lookAndFeel); }

    void paint (juce::Graphics &g) override
    {
        if (auto* customLookAndFeel = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel())) { customLookAndFeel->drawNodeButton(g, *this); }
    }

    void mouseDown(const juce::MouseEvent& e) override { onClick(); }
};



class TraverserButton : public juce::Component {
    public:

    std::function<void()> onClick;
    bool isSelected = false;

    TraverserButton() { setLookAndFeel(ComponentContext::lookAndFeel); }
    void paint(juce::Graphics& g) override
    {
        if (auto* customLookAndFeel = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel())) {
            customLookAndFeel->drawTraverserButton(g, *this);
        }
    }

    void mouseDown(const juce::MouseEvent& e) override { onClick(); }
};



class ButtonPane : public juce::Component {

    public:

    NodeButton nodeButton;
    TraverserButton traverserButton;

    ButtonPane()
    {
        setLookAndFeel(ComponentContext::lookAndFeel);
        addAndMakeVisible(nodeButton);
        addAndMakeVisible(traverserButton);

        nodeButton.onClick = [this]() {

            nodeButton.isSelected = true;
            nodeButton.repaint();

            traverserButton.isSelected = false;
            traverserButton.repaint();

            ComponentContext::canvas->controllerMode = NodeCanvas::ControllerMode::Node;
        };

        traverserButton.onClick = [this]() {
            traverserButton.isSelected = true;
            traverserButton.repaint();

            nodeButton.isSelected = false;
            nodeButton.repaint();

            ComponentContext::canvas->controllerMode = NodeCanvas::ControllerMode::Traverser;
        };
    }

    void paint(juce::Graphics& g) override
    {
        if (auto* customLookAndFeel = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel())) { customLookAndFeel->drawButtonPane(g, *this);}
    }

    void resized () override
    {
        auto bounds = getLocalBounds().reduced(2.0f);
        int buttonSize = bounds.getHeight();
        int numButtons = 2;
        float totalButtonWidth = buttonSize * numButtons;

        float spacing = (bounds.getWidth() - totalButtonWidth) / (numButtons + 1);

        int x = static_cast<int>(bounds.getX() + spacing);
        nodeButton.setBounds(x, bounds.getY(), buttonSize, buttonSize);

        x += buttonSize + spacing;
        traverserButton.setBounds(x, bounds.getY(), buttonSize, buttonSize);
    }
};



class DisplayButton : public juce::Component {

    public:

    bool isSelected = false;
    std::function<void()> onClick;

    DisplayButton() { setLookAndFeel(ComponentContext::lookAndFeel); };

    void paint(juce::Graphics& g) override
    {
        if (auto* customLookAndFeel = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel())) { customLookAndFeel->drawDisplayButton(g, *this); }
    }

    void mouseDown(const juce::MouseEvent& e) override { onClick(); }
};



class DisplayMenu : public juce::Component {

    public:

    DisplayButton button;
    DynamicEditor display;
    juce::PopupMenu menu;
    juce::String selectedOption = "";

    DisplayMenu()
    {
        setLookAndFeel(ComponentContext::lookAndFeel);
        addAndMakeVisible(display);
        addAndMakeVisible(button);

        menu.addItem(1, "show pitch");
        menu.addItem(2, "show velocity");
        menu.addItem(3, "show duration");
        menu.addItem(4, "show countLimit");

        button.onClick = [this]() {
            button.isSelected = true;
            repaint();
            menu.showMenuAsync (juce::PopupMenu::Options(), [this] (int result)
            {
                button.isSelected = false;
                repaint();
                switch (result)
                {
                    case 1: selectedOption = "show pitch"; ComponentContext::canvas->setSelectionMode(NodeBox::DisplayMode::Pitch); break;
                    case 2: selectedOption = "show velocity"; ComponentContext::canvas->setSelectionMode(NodeBox::DisplayMode::Velocity); break;
                    case 3: selectedOption = "show duration"; ComponentContext::canvas->setSelectionMode(NodeBox::DisplayMode::Duration); break;
                    case 4: selectedOption = "show countLimit"; ComponentContext::canvas->setSelectionMode(NodeBox::DisplayMode::CountLimit); break;
                    default: break;
                }
                resized();
            });
        };
    }

    void paint(juce::Graphics& g) override
    {
        if(auto* customLookAndFeel = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel())) { customLookAndFeel->drawDisplayMenu(g,*this); };
    }

    void resized() override
    {
        auto contentBounds = getLocalBounds().reduced(buttonContentBounds);
        auto displayWidth = contentBounds.getWidth() * 2/3;

        display.setBounds(contentBounds.removeFromLeft(displayWidth));
        display.setText(selectedOption);
        display.refit();

        button.setBounds(contentBounds);
    }
};



class TempoDisplay : public juce::Component {

    public:

    SyncButton syncButton;
    DynamicEditor editor;

    TempoDisplay()
    {
        setLookAndFeel(ComponentContext::lookAndFeel);
        addAndMakeVisible(syncButton);
        addAndMakeVisible(editor);
    }

    void paint(juce::Graphics& g) override
    {
        if (auto* customLookAndFeel = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel())) { customLookAndFeel->drawTempoDisplay(g, *this); }
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        int editorWidth = bounds.getWidth() * 3 / 4;
        editor.setBounds(bounds.removeFromLeft(editorWidth));
        syncButton.setBounds(bounds.reduced(1.0f));

        editor.refit();
    }
};

#endif //SEQUENCETREE_TITLEBARBUTTONS_H
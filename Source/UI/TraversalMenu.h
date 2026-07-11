//
// Created by Eli Baumgardner on 5/23/26.
//

#ifndef SEQUENCETREE_TRAVERSALMENU_H
#define SEQUENCETREE_TRAVERSALMENU_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

#include "TraversalDisplayMenu.h"
#include "Node/ValueEditor.h"
#include "ColourSelector.h"

class TraversalMenuListener;

class TraversalMenu : public juce::Component {

public:

    TraversalMenu(ApplicationContext& context);
    ~TraversalMenu() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void selectTraversal(int traversalId);

    std::function<void(int)> onWidthDragged;

    static constexpr int resizerWidth = 6;
    static constexpr int minMenuWidth = 60;

    TraversalDisplayMenu displayMenu;

    juce::Label multiplierLabel;
    ValueEditor multiplierEditor;

    juce::Label channelLabel;
    ValueEditor channelEditor;

    juce::Label colourLabel;
    ColourSelector colourSelector;

private:

    juce::ValueTree currentTraversalData;

    class Resizer : public juce::Component {
    public:
        explicit Resizer(TraversalMenu& owner);

        void mouseDown(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;
        void mouseUp(const juce::MouseEvent& e) override;
        void mouseEnter(const juce::MouseEvent& e) override;
        void mouseExit(const juce::MouseEvent& e) override;
        void paint(juce::Graphics& g) override;

    private:
        TraversalMenu& owner;
        int dragStartWidth = 0;
        int dragStartX = 0;
        bool isHovered = false;
        bool isDragging = false;
    };

    Resizer resizer { *this };

    std::unique_ptr<TraversalMenuListener> menuListener;
};

#endif //SEQUENCETREE_TRAVERSALMENU_H

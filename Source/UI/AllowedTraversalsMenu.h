//
// Created by Eli Baumgardner on 7/20/26.
//

#ifndef SEQUENCETREE_ALLOWEDTRAVERSALSMENU_H
#define SEQUENCETREE_ALLOWEDTRAVERSALSMENU_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>

#include "../Util/ApplicationContext.h"

class AllowedTraversalsMenu : public juce::Component {

public:

    AllowedTraversalsMenu(ApplicationContext& context, juce::ValueTree connection);

    void paint(juce::Graphics& g) override;
    void resized() override;

    int  getIdealHeight() const;

    static constexpr int defaultWidth = 160;
    static constexpr int rowHeight    = 26;
    static constexpr int toggleWidth  = 40;
    static constexpr int contentInset = 8;

private:

    class ToggleButton : public juce::Component {
    public:
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& e) override;

        bool isOn = true;
        std::function<void(bool)> onToggle;
    };

    bool isTraversalEnabled(int traversalId) const;
    void setTraversalEnabled(int traversalId, bool enabled);

    struct TraversalRow {
        int traversalId = 0;
        std::unique_ptr<juce::Label>  label;
        std::unique_ptr<ToggleButton> toggle;
    };

    ApplicationContext& applicationContext;
    juce::ValueTree connection;

    std::vector<TraversalRow> rows;
};

#endif //SEQUENCETREE_ALLOWEDTRAVERSALSMENU_H

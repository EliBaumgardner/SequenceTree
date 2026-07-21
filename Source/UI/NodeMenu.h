//
// Created by Eli Baumgardner on 7/20/26.
//

#ifndef SEQUENCETREE_NODEMENU_H
#define SEQUENCETREE_NODEMENU_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include "../Util/ApplicationContext.h"
#include "Node/ValueEditor.h"
#include "ColourSelector.h"
#include "Buttons/EditTraversalRulesButton.h"

class Node;

class NodeMenu : public juce::Component {

public:

    explicit NodeMenu(ApplicationContext& context);
    ~NodeMenu() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:

    void bindToNode(Node* node);
    void clearBindings();

    ApplicationContext& applicationContext;

    ColourSelector colourSelector { applicationContext };
    juce::Label    colourLabel;

    ValueEditor countLimitEditor        { applicationContext };
    ValueEditor repeatEditor            { applicationContext };
    ValueEditor switchCountLimitEditor  { applicationContext };
    ValueEditor subLoopCountLimitEditor { applicationContext };
    ValueEditor velocityEditor          { applicationContext };
    ValueEditor pitchEditor             { applicationContext };
    ValueEditor channelEditor           { applicationContext };

    juce::Label countLimitLabel;
    juce::Label repeatLabel;
    juce::Label switchCountLimitLabel;
    juce::Label subLoopCountLimitLabel;
    juce::Label velocityLabel;
    juce::Label pitchLabel;
    juce::Label channelLabel;

    struct LabeledRow
    {
        juce::Label& label;
        ValueEditor& editor;
    };

    std::array<LabeledRow, 7> labeledRows {{
        { countLimitLabel,        countLimitEditor        },
        { repeatLabel,            repeatEditor            },
        { switchCountLimitLabel,  switchCountLimitEditor  },
        { subLoopCountLimitLabel, subLoopCountLimitEditor },
        { velocityLabel,          velocityEditor          },
        { pitchLabel,             pitchEditor              },
        { channelLabel,           channelEditor            }
    }};

    EditTraversalRulesButton editTraversalRulesButton { applicationContext };

    static constexpr int rowHeight = 20;
    static constexpr int rowGap    = 6;
};

#endif //SEQUENCETREE_NODEMENU_H

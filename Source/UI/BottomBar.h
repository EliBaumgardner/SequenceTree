#pragma once

#include "../Util/PluginModules.h"
#include "../Util/ApplicationContext.h"
#include "ColourSelector.h"
#include "Node/ValueEditor.h"
#include "Buttons/PaintTool.h"
#include "Buttons/ArrowTool.h"

class BottomBar : public juce::Component
{
public:
    explicit BottomBar(ApplicationContext& context);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:

    void bindToNode(Node* node);
    void clearBindings();

    ApplicationContext& applicationContext;
    ColourSelector colourSelector { applicationContext };

    ValueEditor countLimitEditor        { applicationContext };
    ValueEditor repeatEditor            { applicationContext };
    ValueEditor switchCountLimitEditor  { applicationContext };
    ValueEditor subLoopCountLimitEditor { applicationContext };
    ValueEditor velocityEditor          { applicationContext };
    ValueEditor pitchEditor             { applicationContext };
    ValueEditor channelEditor           { applicationContext };

    struct LabeledEditor
    {
        ValueEditor& editor;
        const char*  label;
    };

    std::array<LabeledEditor, 7> labeledEditors {{
        { countLimitEditor,        "CNT" },
        { repeatEditor,            "RPT" },
        { switchCountLimitEditor,  "SW"  },
        { subLoopCountLimitEditor, "SUB" },
        { velocityEditor,          "VEL" },
        { pitchEditor,             "PIT" },
        { channelEditor,           "CH"  }
    }};

    static constexpr int labelWidth  = 20;
    static constexpr int editorWidth = 28;
    static constexpr int cellGap     = 10;

    Node* node = nullptr;

    std::unique_ptr<PaintTool> paintTool;
    std::unique_ptr<ArrowTool> arrowTool;
};

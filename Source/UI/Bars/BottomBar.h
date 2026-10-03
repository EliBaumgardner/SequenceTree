#pragma once

#include "Bar.h"
#include "../Buttons/IconButton.h"
#include "../Buttons/ButtonPane.h"
#include "../Editors/LabeledEditor.h"
#include "../Buttons/PaintToolSettings.h"
#include "../Menus/ArrowWindow.h"
#include "../PopupWindow.h"
#include "../Menus/ContextMenu.h"
#include "../../Util/NodeInfo.h"

class BottomBar : public Bar
{
public:

    explicit BottomBar(const ApplicationContext& context);

    void resized() override;

    void applyDisplayMode(NodeDisplayMode mode);

private:

    void showQuaverMenu();

    static constexpr float paintPanelWidthRatio    = 0.26f;
    static constexpr float quaverPaneWidthRatio    = 0.105f;
    static constexpr float quaverButtonWidthRatio  = 0.28f;
    static constexpr float countsLabelWidthRatio   = 0.42f;
    static constexpr float countsEditorWidthRatio  = 0.30f;

    PopupWindowLauncher arrowWindowLauncher {
        "Arrows",
        [this]() {
            auto content = std::make_unique<ArrowWindow>(applicationContext);
            content->setSize(ArrowWindow::defaultWidth, ArrowWindow::defaultHeight);

            return content;
        }
    };

    PaintToolSettings paintPanel   { applicationContext };
    ButtonPane        toolPane     { applicationContext };
    ButtonPane        quaverPane   { applicationContext };
    LabeledEditor     countsField  { applicationContext };

    IconButton* quaverTool = nullptr;
    IconButton* spanTool   = nullptr;
};

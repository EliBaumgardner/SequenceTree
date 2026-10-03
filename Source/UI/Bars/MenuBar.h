#pragma once

#include "Bar.h"
#include "../Buttons/IconButton.h"

class MenuBar : public Bar
{
public:

    IconButton treeIcon;
    IconButton nodeIcon;
    IconButton traversalIcon;

    explicit MenuBar(const ApplicationContext& context);

private:

    void resized() override;

    static constexpr float iconInsetRatio = 0.214f;
    static constexpr int   iconCount      = 3;
};

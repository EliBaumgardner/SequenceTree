//
// Created by Eli Baumgardner on 7/20/26.
//

#ifndef SEQUENCETREE_MENUBAR_H
#define SEQUENCETREE_MENUBAR_H

#include "../Bar.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../Buttons/IconButton.h"

class MenuBar : public Bar {
public:

    explicit MenuBar(ApplicationContext& context);

    std::unique_ptr<IconButton> treeIcon = nullptr;
    std::unique_ptr<IconButton> nodeIcon = nullptr;
    std::unique_ptr<IconButton> traversalIcon = nullptr;

private:

    void resized() override;

    static constexpr int iconInset   = 6;
    static constexpr int maxIconSize = 24;
};


#endif //SEQUENCETREE_MENUBAR_H

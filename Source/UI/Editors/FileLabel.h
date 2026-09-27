//
// Created by Eli Baumgardner on 8/16/26.
//

#ifndef SEQUENCETREE_FILELABEL_H
#define SEQUENCETREE_FILELABEL_H


#include <juce_gui_basics/juce_gui_basics.h>
#include "ValueEditor.h"
#include "../../Util/ApplicationContext.h"
#include "../Buttons/IconButton.h"

class FileLabel : public juce::Component {
public:

    FileLabel(const ApplicationContext& context);

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;

    void setFileName(const juce::String fileName);

    void setGrabbed(bool shouldBeGrabbed);
    void setSelected(bool shouldBeSelected);

    std::function<void()> onRemove;
    std::function<void()> onMouseClicked;

    static constexpr float contentInsetRatio   = 0.06f;
    static constexpr float removeButtonRatio   = 0.7f;
    static constexpr float fileTextWidthRatio  = 0.5f;
    static constexpr float fileTextHeightRatio = 0.6f;

    int fileId;

    bool grabbed  = false;
    bool selected = false;

private:

    const ApplicationContext& context;
    std::unique_ptr<ValueEditor> fileText     = nullptr;
    std::unique_ptr<IconButton>  removeButton = nullptr;
};


#endif //SEQUENCETREE_FILELABEL_H
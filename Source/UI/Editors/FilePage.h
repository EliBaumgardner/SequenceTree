//
// Created by Eli Baumgardner on 8/19/26.
//

#ifndef SEQUENCETREE_FILEPAGE_H
#define SEQUENCETREE_FILEPAGE_H

#include <juce_gui_extra/juce_gui_extra.h>

#include "../../Util/ApplicationContext.h"


class FilePage : private juce::CodeDocument,
                 public juce::CodeEditorComponent,
                 private juce::CodeDocument::Listener {
public:

    FilePage(const ApplicationContext& context);
    ~FilePage() override;

    void paintOverChildren(juce::Graphics& g) override;

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void mouseMagnify  (const juce::MouseEvent& e, float scaleFactor) override;

    std::function<void()> onTextChanged;

    std::vector<int> errorLines;

    bool suppressTextChanged = false;

private:

    void setZoom(float newZoom);

    void codeDocumentTextInserted(const juce::String& newText, int insertIndex) override;
    void codeDocumentTextDeleted (int startIndex, int endIndex) override;

    constexpr static float minZoom         = 0.5f;
    constexpr static float maxZoom         = 3.0f;
    constexpr static float zoomSensitivity = 0.15f;

    constexpr static float baseFontHeight = 12.0f;
    constexpr static int   indentSize     = 4;
    constexpr static float errorLineAlpha = 0.14f;
    constexpr static float selectionAlpha = 0.35f;

    float zoom = 1.0f;
};


#endif //SEQUENCETREE_FILEPAGE_H
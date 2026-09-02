//
// Created by Eli Baumgardner on 8/19/26.
//

#ifndef SEQUENCETREE_FILEPAGE_H
#define SEQUENCETREE_FILEPAGE_H

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../Util/ApplicationContext.h"
#include "FileLine.h"


class FilePage : public juce::Component,
                 public juce::AsyncUpdater {
public:

    FilePage(ApplicationContext& context);

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void mouseMagnify  (const juce::MouseEvent& e, float scaleFactor) override;

    void setZoom(float newZoom);

    void setFile(std::vector<std::unique_ptr<FileLine>> newFileLines);

    void createNewFile();

    void         setText(const juce::String& text);
    juce::String getText() const;

    void clearLineErrors();
    void setLineError(int lineNumber, const juce::String& message);

    std::function<void()> onTextChanged;

    int preferredHeightForWidth(int width);

    void focusLine(int index);
    void insertLineAfter(const FileLine* line);
    void mergeWithPreviousLine(const FileLine* line);

private:

    void handleAsyncUpdate() override;

    int  lineHeight() const;
    void applyLineMetrics();

    constexpr static float minZoom         = 0.5f;
    constexpr static float maxZoom         = 3.0f;
    constexpr static float zoomSensitivity = 0.15f;

    float fontHeight() const;
    int   rowCountFor(const juce::String& text, int editorWidth);

    void notifyTextChanged();

    int  indexOf(const FileLine* line) const;
    void focusRelative(const FileLine* line, int offset);
    void refreshLines();
    void performMerge(const FileLine* line);


    constexpr static int   initialLineCount = 1;
    constexpr static int   textAreaInset   = 2;
    constexpr static float baseFontHeight  = 12.0f;

    float zoom = 1.0f;

    bool suppressTextChanged = false;

    juce::TextEditor textMeasurer;

    std::vector<std::unique_ptr<FileLine>> fileLines;

    ApplicationContext& context;
};


#endif //SEQUENCETREE_FILEPAGE_H
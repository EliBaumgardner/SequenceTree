//
// Created by Eli Baumgardner on 8/19/26.
//

#ifndef SEQUENCETREE_FILELINE_H
#define SEQUENCETREE_FILELINE_H

#include <juce_gui_basics/juce_gui_basics.h>

#include "../Theme/Theme.h"

class ApplicationContext;
class LineEditor;

class FileLine : public juce::Component{
public:

    FileLine(ApplicationContext& context);
    ~FileLine() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setLineNumber(int newLineNumber);
    void setGutterWidth(int newGutterWidth);
    void setFontHeight(float newFontHeight);
    void setRowHeight(int newRowHeight);

    juce::String getText() const;
    int          editorWidthFor(int lineWidth) const;

    void setError  (const juce::String& message);
    void clearError();

    bool hasError() const { return errorMessage.isNotEmpty(); }

    static constexpr int gutterTextInset = 2;
    static constexpr int contentInset    = 2;

    std::unique_ptr<LineEditor> lineEditor = nullptr;

    ApplicationContext& context;

private:

    juce::String errorMessage;

    int   lineNumber  = 0;
    int   gutterWidth = 0;
    float fontHeight  = Theme::labelFontHeight;
    int   rowHeight   = 0;
};


#endif //SEQUENCETREE_FILELINE_H
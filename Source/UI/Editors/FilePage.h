#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "../Theme/Theme.h"

class FilePage : private juce::CodeDocument,
                 public juce::CodeEditorComponent,
                 private juce::CodeDocument::Listener
{
public:

    explicit FilePage(const Theme& theme);
    ~FilePage() override;

    void paintOverChildren(juce::Graphics& graphics) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    void mouseMagnify(const juce::MouseEvent& event, float scaleFactor) override;

    std::function<void()> onTextChanged;

    std::vector<int> errorLines;

    bool suppressTextChanged = false;

private:

    void setZoom(float newZoom);
    void codeDocumentTextInserted(const juce::String& newText, int insertIndex) override;
    void codeDocumentTextDeleted(int startIndex, int endIndex) override;

    static constexpr float minZoom         = 0.5f;
    static constexpr float maxZoom         = 3.0f;
    static constexpr float zoomSensitivity = 0.15f;

    static constexpr float baseFontHeight = 12.0f;
    static constexpr int   indentSize     = 4;
    static constexpr float errorLineAlpha = 0.14f;

    float zoom = 1.0f;
};

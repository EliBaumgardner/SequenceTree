/*
  ==============================================================================

    ColourSelector.cpp
    Created: 10 Aug 2025 5:05:50pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#include "ColourSelector.h"
#include "../Graph/ValueTreeIdentifiers.h"
#include "Canvas/NodeCanvas.h"

juce::Colour MainComponent::presetColours[MainComponent::numPresets] {};
bool         MainComponent::presetSet[MainComponent::numPresets]     {};

void Cursor::paint(juce::Graphics& g) {
    auto bounds      = getLocalBounds().toFloat().reduced(0.1f);
    auto boundsPoint = getLocalBounds().toFloat().reduced(4.5f);
    g.drawEllipse(bounds, 1.0f);
    g.drawEllipse(boundsPoint, 1.0f);
}

void PresetSwatch::paint(juce::Graphics& g) {
    g.fillAll(isSet ? colour : juce::Colour(0xff3a3a3a));
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawRect(getLocalBounds(), 1);
}

void PresetSwatch::mouseDown(const juce::MouseEvent& e) {
    if (e.mods.isRightButtonDown()) {
        if (onSave) onSave();
    } else if (isSet) {
        if (onApply) onApply(colour);
    }
}

MainComponent::MainComponent() {
    addAndMakeVisible(cursor);
    cursor.setInterceptsMouseClicks(false, false);
    generateImage();

    for (int i = 0; i < numPresets; ++i)
    {
        auto* s = swatches.add(new PresetSwatch());
        s->colour = presetColours[i];
        s->isSet  = presetSet[i];

        s->onApply = [this](juce::Colour c) {
            colour = c;
            updateCursorPosition(c);
            if (colourPicked) colourPicked(c);
        };

        s->onSave = [this, i, s]() {
            presetColours[i] = colour;
            presetSet[i]     = true;
            s->colour = colour;
            s->isSet  = true;
            s->repaint();
        };

        addAndMakeVisible(s);
    }
}

void MainComponent::generateImage() {
    const int width  = 100;
    const int height = 100;

    image = juce::Image(juce::Image::RGB, width, height, true);

    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
        {
            float hue        = juce::jmap((float)x, 0.0f, (float)width,  0.0f, 1.0f);
            float saturation = juce::jmap((float)y, 0.0f, (float)height, 1.0f, 0.0f);
            image.setPixelAt(x, y, juce::Colour::fromHSV(hue, saturation, 1.0f, 1.0f));
        }
}

void MainComponent::paint(juce::Graphics& g) {
    int pickerH = getHeight() - presetRowHeight;
    if (image.isValid())
        g.drawImageWithin(image, 0, 0, getWidth(), pickerH, juce::RectanglePlacement::stretchToFit);

    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(0, pickerH, getWidth(), presetRowHeight);
}

void MainComponent::resized() {
    int pickerH   = getHeight() - presetRowHeight;
    int swatchSize = presetRowHeight - 4;
    int padding    = 2;
    int startX     = padding;
    int swatchY    = pickerH + (presetRowHeight - swatchSize) / 2;

    for (int i = 0; i < numPresets; ++i)
        swatches[i]->setBounds(startX + i * (swatchSize + padding), swatchY, swatchSize, swatchSize);

    updateCursorPosition(colour);
}

void MainComponent::mouseDrag(const juce::MouseEvent& event) {
    int pickerH = getHeight() - presetRowHeight;
    if (event.y >= pickerH) return;

    cursor.setCentrePosition(event.getPosition());

    if (image.isValid())
    {
        float imageX = juce::jmap<float>(event.x, 0.0f, (float)getWidth(),  0.0f, (float)image.getWidth());
        float imageY = juce::jmap<float>(event.y, 0.0f, (float)pickerH,     0.0f, (float)image.getHeight());

        int ix = juce::jlimit(0, image.getWidth()  - 1, (int)imageX);
        int iy = juce::jlimit(0, image.getHeight() - 1, (int)imageY);

        colour = image.getPixelAt(ix, iy);
        if (colourPicked) colourPicked(colour);
    }
}

void MainComponent::updateCursorPosition(juce::Colour selectedColour) {
    colour = selectedColour;

    float h, s, v;
    colour.getHSB(h, s, v);

    int pickerH = getHeight() - presetRowHeight;
    int x = (int)juce::jmap(h, 0.0f, 1.0f, 0.0f, (float)getWidth());
    int y = (int)juce::jmap(s, 1.0f, 0.0f, 0.0f, (float)pickerH);

    cursor.setBounds(x, y, 10, 10);
}


MainWindow::MainWindow (const juce::String& name, juce::Colour backgroundColour, int requiredButtons, bool addToDesktop)
: DocumentWindow (name, backgroundColour, requiredButtons, addToDesktop){
    
    component = new MainComponent();
    setContentOwned(component, true);
    setResizable(true,true);
}

void MainWindow::closeButtonPressed(){
    
    setVisible(false);
}

MainComponent* MainWindow::getContent() {
    return component;
}


ColourSelector::ColourSelector(ApplicationContext& context)
    : applicationContext(context)
{
}

void ColourSelector::paint(juce::Graphics& g) {

    g.setColour(juce::Colours::black);
    g.drawRect(getLocalBounds(), 1.0f);

    if (node == nullptr) {
        g.setColour(juce::Colours::grey.withAlpha(0.3f));
        g.fillRect(getLocalBounds().reduced(1.0f));
        return;
    }

    g.setColour(colour);
    g.fillRect(getLocalBounds().reduced(1.0f));
}

void ColourSelector::mouseDown(const juce::MouseEvent& event) {

    if (node == nullptr) return;

    if(mainWindow != nullptr){
        mainWindow.reset();
    }
    
    mainWindow = std::make_unique<MainWindow>("",juce::Colours::white,juce::DocumentWindow::closeButton,true);
    mainWindow->centreWithSize(160, 145);
    mainWindow->setVisible(true);
    
    mainWindow->getContent()->updateCursorPosition(colour);
    mainWindow->getContent()->colourPicked =[this](juce::Colour c) {

        if(node != nullptr){
            node->nodeColour = c;
            node->repaint();
            applyColourToDescendants(node, c);
        }

        colour = c;
        repaint();
    };
}

void ColourSelector::setNode(Node* node){

    this->node = node;

    if (node == nullptr) {
        repaint();
        return;
    }

    colour = node->nodeColour;
    repaint();
    
    if(mainWindow != nullptr && mainWindow->getContent() != nullptr){
        mainWindow->toFront(true);
        mainWindow->getContent()->updateCursorPosition(colour);
    }
}

void ColourSelector::applyColourToDescendants(Node* n, juce::Colour c)
{
    NodeCanvas* canvas = applicationContext.canvas;
    if (canvas == nullptr) return;

    juce::ValueTree childrenIds = n->nodeValueTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);
    if (! childrenIds.isValid()) return;

    for (int i = 0; i < childrenIds.getNumChildren(); ++i)
    {
        int childId = childrenIds.getChild(i).getProperty(ValueTreeIdentifiers::Id);
        auto it = canvas->nodeMap.find(childId);
        if (it != canvas->nodeMap.end())
        {
            it->second->nodeColour = c;
            it->second->repaint();
            applyColourToDescendants(it->second, c);
        }
    }
}

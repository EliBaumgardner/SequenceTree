/*
  ==============================================================================

    ColourSelector.cpp
    Created: 10 Aug 2025 5:05:50pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#include "ColourSelector.h"

//Cursor//
void Cursor::paint(juce::Graphics& g) {
  
    auto bounds = getLocalBounds().toFloat().reduced(0.1f);
    auto boundsPoint = getLocalBounds().toFloat().reduced(4.5f);
    
    g.drawEllipse(bounds,1.0f);
    
    g.drawEllipse(boundsPoint, 1.0f);
}


//Main Component//
MainComponent::MainComponent() {
    
    addAndMakeVisible(cursor);
    cursor.setInterceptsMouseClicks(false,false);
    generateImage();
}

void MainComponent::generateImage() {
    
    const int width = 100;
    const int height = 100;
    
    image = juce::Image(juce::Image::RGB,width,height,true);
    
    for(int y = 0; y < height; ++y){
        for(int x = 0; x < width; ++x){
            
            float hue = juce::jmap((float)x, 0.0f, (float)width, 0.0f, 1.0f);
            float saturation = juce::jmap((float)y, 0.0f, (float)height, 1.0f, 0.0f);
            
            juce::Colour pixelColour = juce::Colour::fromHSV(hue,saturation,1.0f,1.0f);
            
            image.setPixelAt(x,y,pixelColour);
        }
    }
}

void MainComponent::paint(juce::Graphics& g) {
    
    if(image.isValid()){
        
        g.drawImageWithin(image, 0, 0, getWidth(),getHeight(), juce::RectanglePlacement::stretchToFit);
    }
}

void MainComponent::resized(){
    
    updateCursorPosition(colour);
}

void MainComponent::mouseDrag(const juce::MouseEvent& event)
{
    auto position = event.getPosition();
    cursor.setCentrePosition(position);

    if (image.isValid())
    {
        // Map from component space to image space
        float imageX = juce::jmap<float>(event.x, 0.0f, (float)getWidth(), 0.0f, (float)image.getWidth());
        float imageY = juce::jmap<float>(event.y, 0.0f, (float)getHeight(), 0.0f, (float)image.getHeight());

        int ix = (int)juce::jlimit(0, image.getWidth()  - 1, (int)imageX);
        int iy = (int)juce::jlimit(0, image.getHeight() - 1, (int)imageY);

        colour = image.getPixelAt(ix, iy);
        colourPicked(colour);
    }
}

void MainComponent::updateCursorPosition(juce::Colour selectedColour) {
    
    colour = selectedColour;
    
    float h, s, v;
    colour.getHSB(h, s, v);

    int x = (int)juce::jmap(h, 0.0f, 1.0f, 0.0f, (float)getWidth());
    int y = (int)juce::jmap(s, 1.0f, 0.0f, 0.0f, (float)getHeight());
    
    cursor.setBounds(x,y,10,10);
    //cursor.setCentrePosition(x,y);
}


//Main Window//
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


//Selector Button Dispaly//

ColourSelector::ColourSelector(){

}

void ColourSelector::paint(juce::Graphics& g) {
    
    g.setColour(juce::Colours::black);
    g.drawRect(getLocalBounds(), 1.0f);
    
    g.setColour(colour);
    g.fillRect(getLocalBounds().reduced(1.0f));
}

void ColourSelector::mouseDown(const juce::MouseEvent& event) {
    
    if(mainWindow != nullptr){
        mainWindow.reset();
    }
    
    mainWindow = std::make_unique<MainWindow>("",juce::Colours::white,juce::DocumentWindow::closeButton,true);
    mainWindow->centreWithSize(100,100);
    mainWindow->setVisible(true);
    
    mainWindow->getContent()->updateCursorPosition(colour);
    mainWindow->getContent()->colourPicked =[this](juce::Colour c) {
        
        if(node != nullptr){
            node->nodeColour = c;
            node->repaint();
        }
        
        colour = c;
        repaint();
    };
}

void ColourSelector::setNode(Node* node){
    
    this->node = node;
    colour = node->nodeColour;
    repaint();
    
    if(mainWindow != nullptr && mainWindow->getContent() != nullptr){
        mainWindow->toFront(true);
        mainWindow->getContent()->updateCursorPosition(colour);
    }
}

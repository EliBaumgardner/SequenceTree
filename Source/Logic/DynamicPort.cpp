#include "DynamicPort.h"

DynamicPort::DynamicPort(juce::Component* content)
    : component(content)
{
    
    std::cout<<"panning"<<std::endl;
    setViewedComponent(component, false);
    component->setSize(3000, 3000); // set an initial large size for panning
    centerInComponent();
    
    addChildComponent(component);
}

DynamicPort::~DynamicPort(){
 
}

void DynamicPort::mouseDown(const juce::MouseEvent& e)
{
    lastMousePosition = e.getPosition();
    
    zoomPoint = e.getPosition();
}

void DynamicPort::mouseDrag(const juce::MouseEvent& e)
{
   
    if(e.mods.isLeftButtonDown()){
        
        auto delta = e.getPosition() - lastMousePosition;
        lastMousePosition = e.getPosition();
        
        float zoomChange = 1.0f + (delta.y * 0.01f);
        zoom *= zoomChange;
        zoom = std::clamp(zoom, 0.1f, 5.0f);
        
        setZoom(zoom);
    }
}

void DynamicPort::setZoom(float newZoom)
{
    zoom = newZoom;

    if (component != nullptr)
    {
        auto originalSize = component->getBounds();
        component->setTransform(juce::AffineTransform::scale(zoom,zoom,lastMousePosition.x,lastMousePosition.y));
        //component->setTopLeftPosition((getWidth() * zoom) - getWidth(),(getHeight()* zoom) - getHeight());
        //setViewPosition(lastMousePosition);
    }
    
    
}

void DynamicPort::centerInComponent(){
    
    int compCenterX = component->getWidth() / 2 + getWidth()/2;
    int compCenterY = component->getHeight() / 2 + getHeight()/2;
    
    setViewPosition(compCenterX,compCenterY);
}

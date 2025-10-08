/*
  ==============================================================================

    SelectionBar.h
    Created: 27 Aug 2025 6:30:12pm
    Author:  Eli Baimgardner

  INNER CLASSES ARE LISTED FIRST, SO THEY CAN BE IMPLEMENTED!
  
  ==============================================================================
*/

#pragma once
#include "ProjectModules.h"
#include "BinaryData.h"
#include "PianoRoll.h"
#include "ComponentContext.h"
#include "NodeCanvas.h"
#include "DynamicEditor.h"
#include "NodeBox.h"


static constexpr float buttonBoundsReduction = 3.0f;
static constexpr float buttonBorderThickness = 2.0f;
static constexpr float buttonContentBounds = 4.0f;
static constexpr float buttonSelectedThickness = 4.0f;

class SelectionBar : public juce::Component {
    
    public:
    
    SelectionBar();
    void resized() override;
    void paint(juce::Graphics& g) override;
    
    
    
    private:
    
    //BUTTONS//
    class NodeButton : public juce::Component {
        
        public:
        
        class MyTableModel : public juce::TableListBoxModel
        {
        public:
            std::function<void(int)> onSelectionChanged;
            juce::TableListBox* table = nullptr;

            int getNumRows() override { return (int) data.size(); }

            void paintCell (juce::Graphics& g,
                            int rowNumber,
                            int columnId,
                            int width,
                            int height,
                            bool rowIsSelected) override
            {
                if (columnId == 1)
                    g.drawImage (data[rowNumber].icon,
                                 juce::Rectangle<float> (0, 0, (float) width, (float) height));
                else if (columnId == 2)
                    g.drawText (data[rowNumber].name,
                                0, 0, width, height,
                                juce::Justification::centredLeft);
                else if (columnId == 3)
                    g.drawText (data[rowNumber].description,0,0,width,height,juce::Justification::centredLeft);

                g.setColour(juce::Colours::black);
                g.drawRect(0, 0, width, height, 0.5);
            }

            juce::Component* refreshComponentForCell (int, int, bool, juce::Component* existing) override
            {
                return existing;
            }
            
            void paintRowBackground (juce::Graphics& g,
                                     int rowNumber,
                                     int width,
                                     int height,
                                     bool rowIsSelected) override
            {
                if (rowIsSelected)
                    g.fillAll (juce::Colours::lightblue);
                else
                    g.fillAll (juce::Colours::transparentBlack);
            }

            void selectedRowsChanged(int lastRowSelected) override {
                if (onSelectionChanged) {
                    onSelectionChanged(lastRowSelected);
                }
            }

            struct RowData { juce::Image icon; juce::String name; juce::String description; };
            std::vector<RowData> data;

            juce::Image icon;

        };

        
        
        class NodeButtonOptions : public juce::DocumentWindow {
        public:
            std::function<void(int)> onSelectionChanged;
            std::unique_ptr<MyTableModel> myTableModel = nullptr;
            juce::TableListBox* table = nullptr;

            juce::Image nodeIcon = juce::ImageFileFormat::loadFrom( BinaryData::Node_png, BinaryData::Node_pngSize );
            juce::Image counterIcon = juce::ImageFileFormat::loadFrom ( BinaryData::Counter_png, BinaryData::Counter_pngSize );
            juce::Image traverserIcon = juce::ImageFileFormat::loadFrom ( BinaryData::Traverser_png, BinaryData::Traverser_pngSize );
            
            NodeButtonOptions(juce::String name, juce::Colour backgroundColour, int requiredButtons,bool addToDesktop) : DocumentWindow(name,backgroundColour, requiredButtons, addToDesktop){
                
                myTableModel = std::make_unique<MyTableModel>();

                myTableModel->onSelectionChanged = [this](int lastRow) {
                    onSelectionChanged(lastRow);
                };

                myTableModel.get()->data.push_back({nodeIcon,"Node","Nodes represent musical events"});
                myTableModel.get()->data.push_back({counterIcon,"NodeCounter","NodeCounter counts the amount of events triggered"});
                myTableModel.get()->data.push_back({traverserIcon,"TraversalStarter","TraversalStarter starts traversals of nodes"});


                table = new juce::TableListBox("NodeOptions",myTableModel.get());
                table->setModel(myTableModel.get());
                table->setRowHeight(20);
                table->getHeader().addColumn("Icon", 1, 100, 100, -1, true);
                table->getHeader().addColumn("Name",2,100, 50, -1, true);
                table->getHeader().addColumn("Description",3,100,50,-1,true);
                
                setContentOwned(table,true);
                setResizable(true,true);
            }
            
            void closeButtonPressed() override {
                setVisible(false);
            }

            void resized() override
            {
                juce::DocumentWindow::resized();
                auto* content = getContentComponent();
                if (auto* table = dynamic_cast<juce::TableListBox*>(content))
                {
                    int numRows = myTableModel->getNumRows();
                    if (numRows > 0)
                        table->setRowHeight(table->getHeight() / numRows);
                }

                auto* header = &table->getHeader(); // take address
                if (header != nullptr)
                {
                    int totalWidth = table->getWidth();
                    header->setColumnWidth(1, totalWidth / 6);
                    header->setColumnWidth(2, totalWidth / 3);
                    header->setColumnWidth(3, totalWidth / 2);
                }

            }


        };
        
        //NODEBUTTON IMPLEMENTATION

        juce::Image nodeIcon = juce::ImageFileFormat::loadFrom( BinaryData::Node_png, BinaryData::Node_pngSize );
        juce::Image counterIcon = juce::ImageFileFormat::loadFrom ( BinaryData::Counter_png, BinaryData::Counter_pngSize );
        juce::Image traverserIcon = juce::ImageFileFormat::loadFrom ( BinaryData::Traverser_png, BinaryData::Traverser_pngSize );

        std::unique_ptr<NodeButtonOptions> nodeButtonOptions = nullptr;
        
        std::function<void()> onClick;
        std::function<void(int)> onSelectionChanged;

        bool buttonSelected = false;

        int columnSelected = 1;
        
        NodeButton() {

        }
        void paint(juce::Graphics& g) override {

            g.fillAll(juce::Colours::transparentBlack);

            auto bounds = getLocalBounds().toFloat().reduced(buttonBoundsReduction);
            auto circleBounds = getLocalBounds().toFloat().reduced(buttonContentBounds);
            g.setColour(juce::Colours::black);
            
            if(buttonSelected){
                g.drawRect(bounds,buttonSelectedThickness);
            }
            else {
                g.drawRect(bounds,buttonBorderThickness);
            }

            if (columnSelected == 1) {
                g.drawImage(nodeIcon,circleBounds);
            }
            else if (columnSelected == 2) {
                g.drawImage(counterIcon,circleBounds);
            }
            else if (columnSelected == 3) {
                g.drawImage(traverserIcon,circleBounds);
            }

            //g.drawEllipse(circleBounds, 1.0f);
            
        }
        
        void mouseDown(const juce::MouseEvent& e) override {
            onClick();
            
            if(nodeButtonOptions != nullptr){
                nodeButtonOptions.reset();
            }
            
            nodeButtonOptions = std::make_unique<NodeButtonOptions>("",juce::Colours::white,juce::DocumentWindow::closeButton, true);
            nodeButtonOptions.get()->onSelectionChanged = [this](int lastRow) {
                columnSelected = lastRow+1;
                repaint();
                onSelectionChanged(lastRow);
            };
            nodeButtonOptions.get()->centreWithSize(200,200);
            nodeButtonOptions.get()->setVisible(true);
        }
    };
    
    
    class InspectButton : public juce::Component {
        
        public:
        
        bool buttonSelected = false;
        
        std::function<void()> onClick;
        InspectButton() {}
        
        void paint (juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat().reduced(buttonBoundsReduction);
            g.setColour(juce::Colours::black);
            
            if(buttonSelected){
                g.drawRect(bounds,buttonSelectedThickness);
            }
            else {
                g.drawRect(bounds, buttonBorderThickness);
            }
            
            auto cursorBounds = getLocalBounds().toFloat().reduced(buttonContentBounds);
            float w = cursorBounds.getWidth();
            float h = cursorBounds.getHeight();
            
            juce::Path cursorPath;
            
            cursorPath.startNewSubPath(9.0f, 0.0f);
            
            cursorPath.lineTo(0.0f, h);
            
            cursorPath.lineTo(w * 0.3f, h * 0.5f);
            
            cursorPath.lineTo(w, h );
            
            cursorPath.closeSubPath();
            
            juce::AffineTransform translation = juce::AffineTransform::translation(5.0f, 3.0f);
            cursorPath.applyTransform(translation);
            
            // Fill and outline
            g.setColour(juce::Colours::black);
            g.fillPath(cursorPath);
            
            g.setColour(juce::Colours::white);
            g.strokePath(cursorPath, juce::PathStrokeType(1.0f));
        }
        
        void mouseDown(const juce::MouseEvent& e) override {
            onClick();
        }
        
    };
    
    
    class PianoRollButton : public juce::Component {
        
        public:
        
        class PianoRollWindow : public juce::DocumentWindow {
            
            public:
            
            PianoRoll* pianoRoll = nullptr;
            
            PianoRollWindow(const juce::String name, juce::Colour backgroundColour, int requiredButtons, bool addToDeskTop) : DocumentWindow(name,backgroundColour, requiredButtons, addToDeskTop) {
                
                pianoRoll = new PianoRoll();
                setContentOwned(pianoRoll, true);
                setResizable(true,true);
                
            }
            
            void closeButtonPressed() override {
                setVisible(false);
            }
            
            
        };
        
        std::unique_ptr<PianoRollWindow> pianoRollWindow = nullptr;
        bool buttonSelected = false;
        
        PianoRollButton() {}
        
        void paint(juce::Graphics& g) override {
            //find piano icon
            
            auto bounds = getLocalBounds().toFloat().reduced(buttonBoundsReduction);
            g.setColour(juce::Colours::black);
            
            if(buttonSelected){
                g.drawRect(bounds,buttonSelectedThickness);
            }
            else {
                g.drawRect(bounds,buttonBorderThickness);
            }
        }
        
        void mouseDown(const juce::MouseEvent& e) override {
            //open documentwindow containing piano roll;
            if(pianoRollWindow != nullptr){
                pianoRollWindow.reset();
            }
            
            pianoRollWindow = std::make_unique<PianoRollWindow>("",juce::Colours::white,juce::DocumentWindow::closeButton,true);
            pianoRollWindow->centreWithSize(200,200);
            pianoRollWindow->setVisible(true);
            
        }
    };
    
    
    class SelectionButton : public juce::Component {
        
        public:
        
        class PopUpButton : public juce::Component {
            
            public:
            
            std::function<void()> onClick;
            PopUpButton() {};
            void paint(juce::Graphics& g) override {
                auto bounds = getLocalBounds().toFloat().reduced(buttonBoundsReduction);
                auto contentBounds = getLocalBounds().toFloat().reduced(buttonContentBounds);
                g.setColour(juce::Colours::black);
                g.drawRect(bounds,buttonBorderThickness);
                
                juce::Path vPath;
                vPath.startNewSubPath(bounds.getTopLeft());
                vPath.lineTo(getWidth()/2,getHeight());
                vPath.lineTo(bounds.getTopRight());
                vPath.closeSubPath();
                
                juce::AffineTransform translation = juce::AffineTransform::translation(5.0f,3.0f);
                
                vPath.applyTransform(translation);
                g.strokePath(vPath, juce::PathStrokeType(1.0f));
                
            }
            
            void mouseDown(const juce::MouseEvent& e) override {
                onClick();
            }
        };
        
        //VARIABLES
        PopUpButton button;
        DynamicEditor display;
        juce::PopupMenu menu;
        juce::String selectedOption = "";
        //
        
        SelectionButton() {
            addAndMakeVisible(display);
            addAndMakeVisible(button);
            
            menu.addItem(1, "show pitch");
            menu.addItem(2, "show velocity");
            menu.addItem(3, "show duration");
            menu.addItem(4, "show countLimit");
            
            button.onClick = [this]() {
                menu.showMenuAsync (juce::PopupMenu::Options(), [this] (int result)
                {
                    if(result == 1) { selectedOption = "show pitch"; ComponentContext::canvas->setSelectionMode(NodeBox::DisplayMode::Pitch);}
                    if(result == 2) { selectedOption = "show velocity"; ComponentContext::canvas->setSelectionMode(NodeBox::DisplayMode::Velocity);}
                    if(result == 3) { selectedOption = "show duration"; ComponentContext::canvas->setSelectionMode(NodeBox::DisplayMode::Duration);}
                    if(result == 4) { selectedOption = "show coutLimit";
                        ComponentContext::canvas->setSelectionMode(NodeBox::DisplayMode::CountLimit);}
                    
                    resized();
                });
            };
        }

        void paint(juce::Graphics& g) override {
            
            auto bounds = getLocalBounds().toFloat().reduced(buttonBoundsReduction);
            g.setColour(juce::Colours::black);
            g.drawRect(bounds,buttonBorderThickness);
        }
        
        void resized() override {
            auto contentBounds = getLocalBounds().reduced(buttonContentBounds);
            auto displayWidth = contentBounds.getWidth() * 2/3;

            display.setBounds(contentBounds.removeFromLeft(displayWidth));
            display.setText(selectedOption);
            display.refit();
            
            button.setBounds(contentBounds);
            
        }
        
        private:
    };
    
    SelectionButton selectionButton;
    PianoRollButton pianoRollButton;
    NodeButton nodeButton;
    InspectButton inspectButton;
};


cmake_minimum_required(VERSION 3.15)

project(SequenceTree VERSION 0.0.1)


# Add JUCEIO
add_subdirectory(JUCE)

# Add your plugin
juce_add_plugin(SequenceTree
        PRODUCT_NAME "SequenceTree"
        COMPANY_NAME "ArsNova"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        FORMATS AU VST3 Standalone
)

juce_add_binary_data(MyBinaryData SOURCES
        assets/Node.png
        assets/Counter.png
        assets/Traverser.png
)

target_include_directories(SequenceTree PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/juce_binarydata_MyBinaryData
)

target_compile_definitions(SequenceTree PRIVATE
        JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1
)

target_sources(SequenceTree PRIVATE
        Source/ColourSelector.cpp
        Source/DynamicEditor.cpp
        Source/DynamicField.cpp
        Source/DynamicLabel.cpp
        Source/DynamicPort.cpp
        Source/MenuItem.cpp
        Source/Node.cpp
        Source/NodeArrow.cpp
        Source/NodeBox.cpp
        Source/NodeCanvas.cpp
        Source/ObjectController.cpp
        Source/NodeData.cpp
        Source/NodeLogic.cpp
        Source/NodeMenu.cpp
        Source/PianoRoll.cpp
        Source/PluginEditor.cpp
        Source/PluginProcessor.cpp
        Source/SelectionBar.cpp
        Source/SliderBar.cpp
        Source/TitleBar.cpp
        Source/Counter.cpp
        Source/ObjectController.cpp
        Source/Traverser.cpp
)

target_link_libraries(SequenceTree PRIVATE
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_audio_basics
        juce::juce_audio_processors
        juce::juce_audio_devices
        juce::juce_audio_utils
        MyBinaryData
)

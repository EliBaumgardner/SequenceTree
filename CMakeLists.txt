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
        Source/UI/ColourSelector.cpp
        Source/UI/DynamicEditor.cpp
        Source/Logic/DynamicPort.cpp
        Source/Node/Node.cpp
        Source/Node/NodeArrow.cpp
        Source/Node/NodeBox.cpp
        Source/Node/NodeCanvas.cpp
        Source/Logic/ObjectController.cpp
        Source/Node/NodeData.cpp
        Source/UI/PianoRoll.cpp
        Source/PluginEditor.cpp
        Source/PluginProcessor.cpp
        Source/UI/SelectionBar.cpp
        Source/UI/TitleBar.cpp
        Source/Node/Counter.cpp
        Source/Node/RelayNode.cpp
        Source/CustomLookAndFeel.cpp
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

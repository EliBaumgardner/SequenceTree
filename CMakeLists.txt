cmake_minimum_required(VERSION 3.15)

project(SequenceTree VERSION 0.0.1)


# Add JUCEIO
add_subdirectory(JUCE)

# Set the path to your VST2 SDK headers
juce_set_vst2_sdk_path("/Users/elibaumgardner/Downloads/VST_SDK/VST2_SDK")

set(JUCE_COPY_PLUGIN_AFTER_BUILD ON)

# Add your plugin
juce_add_plugin(SequenceTree
        PRODUCT_NAME "SequenceTree"
        COMPANY_NAME "ArsNova"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT TRUE
        FORMATS AU VST3 VST Standalone
        PLUGIN_MANUFACTURER_CODE ANON
        PLUGIN_CODE ST01
)

juce_add_binary_data(MyBinaryData SOURCES  assets/Node.png assets/Counter.png assets/Traverser.png )

target_include_directories(SequenceTree PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/juce_binarydata_MyBinaryData
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/UI
)

target_compile_definitions(SequenceTree PRIVATE
        JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_sources(SequenceTree PRIVATE
        Source/Plugin/PluginProcessor.cpp
        Source/Plugin/PluginEditor.cpp
        Source/Audio/AudioUIBridge.cpp
        Source/Audio/EventManager.cpp
        Source/Audio/NoteScheduler.cpp
        Source/Audio/TraversalLogic.cpp
        Source/Audio/TraversalDispatcher.cpp
        Source/Graph/RTGraphBuilder.cpp
        Source/Graph/ValueTreeState.cpp
        Source/Graph/ValueTreeIdentifiers.cpp
        Source/Input/NodeController.cpp
        Source/UI/Canvas/DynamicPort.cpp
        Source/UI/Canvas/NodeCanvas.cpp
        Source/UI/Canvas/NodeCanvasTreeListener.cpp
        Source/UI/Node/Node.cpp
        Source/UI/Node/RootNode.cpp
        Source/UI/Node/NodeArrow.cpp
        Source/UI/Node/NodeTextEditor.cpp
        Source/UI/Node/ValueEditor.cpp
        Source/UI/Node/Modulator.cpp
        Source/UI/Node/Connector.cpp
        Source/UI/Theme/CustomLookAndFeel.cpp
        Source/UI/Theme/CustomTextEditor.cpp
        Source/UI/Theme/CustomTextCaret.cpp
        Source/UI/Buttons/RootRectangle.cpp
        Source/UI/ColourSelector.cpp
        Source/UI/PianoRoll.cpp
        Source/UI/Titlebar.cpp
        Source/UI/BottomBar.cpp
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
        juce::juce_animation
        MyBinaryData
)

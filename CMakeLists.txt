cmake_minimum_required(VERSION 3.22)

project(SequenceTree VERSION 0.0.1)


# Add JUCEIO
add_subdirectory(JUCE)

set(SEQUENCETREE_VST2_SDK_PATH "$ENV{HOME}/Downloads/VST_SDK/VST2_SDK"
        CACHE PATH "VST2 SDK headers; leave empty to build without the VST2 format")

set(SEQUENCETREE_FORMATS AU VST3 Standalone)

if(SEQUENCETREE_VST2_SDK_PATH AND IS_DIRECTORY "${SEQUENCETREE_VST2_SDK_PATH}")
    juce_set_vst2_sdk_path("${SEQUENCETREE_VST2_SDK_PATH}")
    list(APPEND SEQUENCETREE_FORMATS VST)
else()
    message(STATUS "VST2 SDK not found at '${SEQUENCETREE_VST2_SDK_PATH}' - building without the VST2 format")
endif()

set(JUCE_COPY_PLUGIN_AFTER_BUILD ON)

# Add your plugin
juce_add_plugin(SequenceTree
        PRODUCT_NAME "SequenceTree"
        COMPANY_NAME "ArsNova"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT TRUE
        FORMATS ${SEQUENCETREE_FORMATS}
        PLUGIN_MANUFACTURER_CODE ANON
        PLUGIN_CODE ST01
)

juce_add_binary_data(MyBinaryData SOURCES  assets/Node.png assets/Counter.png assets/Traverser.png )

target_compile_features(SequenceTree PRIVATE cxx_std_20)

target_include_directories(SequenceTree PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/juce_binarydata_MyBinaryData
)

if(MSVC)
    target_compile_options(SequenceTree PRIVATE /W4)
else()
    target_compile_options(SequenceTree PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

target_compile_definitions(SequenceTree PRIVATE
        JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_sources(SequenceTree PRIVATE
        Source/Plugin/PluginProcessor.cpp
        Source/Plugin/PluginEditor.cpp
        Source/Plugin/AudioSnapshotPublisher.cpp
        Source/Audio/EventManager.cpp
        Source/Audio/NoteScheduler.cpp
        Source/Audio/TraversalLogic.cpp
        Source/Audio/TraversalDispatcher.cpp
        Source/Audio/FlagScheduler.cpp
        Source/Audio/TraversalSession.cpp
        Source/Audio/TraversalRule.cpp
        Source/Audio/ScriptTraversalRule.cpp
        Source/Script/ScriptLexer.cpp
        Source/Script/ScriptParser.cpp
        Source/Script/ScriptEmitter.cpp
        Source/Script/ScriptCompiler.cpp
        Source/Audio/NodeStateTable.cpp
        Source/Graph/RTGraphBuilder.cpp
        Source/Graph/GraphState.cpp
        Source/Graph/TraversalRuleState.cpp
        Source/Graph/ValueTreeIdentifiers.cpp
        Source/Input/NodeController.cpp
        Source/UI/Canvas/DynamicPort.cpp
        Source/UI/Canvas/NodeCanvas.cpp
        Source/UI/Canvas/NodeCanvasTreeListener.cpp
        Source/UI/Canvas/ValueField.cpp
        Source/UI/Canvas/AudioCommandDrainer.cpp
        Source/UI/Canvas/NodeManager.cpp
        Source/UI/Canvas/ArrowManager.cpp
        Source/UI/Canvas/CanvasHitTester.cpp
        Source/Input/NodeCreationDispatcher.cpp
        Source/Input/ConnectionOps.cpp
        Source/Input/SelectionOps.cpp
        Source/UI/Node/Node.cpp
        Source/UI/Node/RootNode.cpp
        Source/UI/Node/Arrow.cpp
        Source/UI/Node/ArrowAnimation.cpp
        Source/UI/Editors/ValueEditor.cpp
        Source/UI/Editors/ValueFormat.cpp
        Source/UI/Node/Modulator.cpp
        Source/UI/Node/TraversalFlagNode.cpp
        Source/UI/Theme/CustomLookAndFeel.cpp
        Source/UI/Theme/CustomLookAndFeel_Nodes.cpp
        Source/UI/Theme/CustomLookAndFeel_Buttons.cpp
        Source/UI/Theme/CustomLookAndFeel_Menus.cpp
        Source/UI/Theme/CustomTextEditor.cpp
        Source/UI/Theme/CustomTextCaret.cpp
        Source/UI/Buttons/RootRectangle.cpp
        Source/UI/Menus/ArrowWindow.cpp
        Source/UI/Menus/ArrowBindBar.cpp
        Source/UI/Menus/ItemSelector.cpp
        Source/UI/Menus/TraversalMenu.cpp
        Source/UI/Menus/MenuArea.cpp
        Source/UI/PanelResizer.cpp
        Source/UI/Menus/NodeMenu.cpp
        Source/UI/Menus/AllowedTraversalsMenu.cpp
        Source/UI/Menus/TraversalRulesWindow.cpp
        Source/UI/PopupWindow.cpp
        Source/UI/Menus/ColourSelector.cpp
        Source/UI/Bar.cpp
        Source/UI/Titlebar.cpp
        Source/UI/BottomBar.cpp
        Source/UI/Menus/MenuBar.cpp
        Source/UI/LabelPanel.cpp
        Source/UI/Editors/FileLabel.cpp
        Source/UI/Editors/FileLine.cpp
        Source/UI/Editors/LineEditor.cpp
        Source/UI/Editors/FilePage.cpp
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

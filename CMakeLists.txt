cmake_minimum_required(VERSION 3.22)

project(SequenceTree VERSION 1.0.0)

option(SEQUENCETREE_REALTIME_SANITIZER "Build everything with -fsanitize=realtime and add SequenceTree_RealtimeTests; needs upstream clang 20 or later" OFF)

set(SEQUENCETREE_COPY_PLUGIN TRUE)

if(SEQUENCETREE_REALTIME_SANITIZER)
    add_compile_options(-fsanitize=realtime -fdelayed-template-parsing -Wno-delayed-template-parsing-in-cxx20)
    add_link_options(-fsanitize=realtime)
    set(SEQUENCETREE_COPY_PLUGIN FALSE)
endif()

# Add JUCEIO
add_subdirectory(JUCE)

set(ARSNOVAUI_PATH "${CMAKE_CURRENT_SOURCE_DIR}/ArsNovaUI" CACHE PATH "Checkout of the ArsNovaUI library")
add_subdirectory(${ARSNOVAUI_PATH} ${CMAKE_CURRENT_BINARY_DIR}/ArsNovaUI)

# Add your plugin
juce_add_plugin(SequenceTree
        PRODUCT_NAME "SequenceTree"
        COMPANY_NAME "ArsNova"
        COPY_PLUGIN_AFTER_BUILD ${SEQUENCETREE_COPY_PLUGIN}
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT TRUE
        FORMATS AU VST3 Standalone
        PLUGIN_MANUFACTURER_CODE ARNV
        PLUGIN_CODE ST01
)

target_compile_features(SequenceTree PRIVATE cxx_std_23)

if(MSVC)
    target_compile_options(SequenceTree PRIVATE /W4)
else()
    target_compile_options(SequenceTree PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

target_compile_definitions(SequenceTree PRIVATE
        JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_compile_definitions(SequenceTree PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

add_library(SequenceTreeCore STATIC
        Source/Core/Script/ScriptCompiler.cpp
        Source/Core/Script/ScriptEmitter.cpp
        Source/Core/Script/ScriptLexer.cpp
        Source/Core/Script/ScriptParser.cpp
        Source/Core/Traversal/ChildSelector.cpp
        Source/Core/Traversal/ModulatorWalk.cpp
        Source/Core/Traversal/NodeRowMap.cpp
        Source/Core/Traversal/NodeStateTable.cpp
        Source/Core/Traversal/ScriptRun.cpp
        Source/Core/Traversal/TraversalLogic.cpp
        Source/Core/Traversal/TraversalRule.cpp
        Source/Core/Traversal/WeightedPick.cpp
)

target_compile_features(SequenceTreeCore PUBLIC cxx_std_23)
target_include_directories(SequenceTreeCore PUBLIC Source)
set_target_properties(SequenceTreeCore PROPERTIES POSITION_INDEPENDENT_CODE ON CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)

if(MSVC)
    target_compile_options(SequenceTreeCore PRIVATE /W4)
else()
    target_compile_options(SequenceTreeCore PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

target_link_libraries(SequenceTreeCore PRIVATE
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
)

target_sources(SequenceTree PRIVATE
        Source/Processor/PluginProcessor.cpp
        Source/Processor/PluginDocument.cpp
        Source/Processor/Document/ArrowBindingOps.cpp
        Source/Processor/Document/Clipboard.cpp
        Source/Processor/Document/EncapsulationOps.cpp
        Source/Processor/Document/GraphState.cpp
        Source/Processor/Document/NodeFactory.cpp
        Source/Processor/Document/StateJson.cpp
        Source/Processor/Document/TraversalRuleState.cpp
        Source/Processor/Document/TraversalState.cpp
        Source/Processor/Document/TypedProperties.cpp
        Source/Processor/Document/ValueTreeIdentifiers.cpp
        Source/Processor/Document/Ornaments/OrnamentFigure.cpp
        Source/Processor/Document/Ornaments/OrnamentLibraries.cpp
        Source/Processor/Document/Ornaments/OrnamentSymbol.cpp
        Source/Processor/AudioModel/AudioSnapshotPublisher.cpp
        Source/Processor/AudioModel/RTGraphBuilder.cpp
        Source/Processor/Playback/EventManager.cpp
        Source/Processor/Playback/FlagScheduler.cpp
        Source/Processor/Playback/NoteScheduler.cpp
        Source/Processor/Playback/PlaybackEngine.cpp
        Source/Processor/Playback/TraversalDispatcher.cpp
        Source/Processor/Playback/TraversalSession.cpp
        Source/Processor/Channels/AudioUIBridge.cpp
        Source/Processor/Channels/VirtualMidiOutput.cpp
        Source/Editor/PluginEditor.cpp
        Source/Editor/Bars/BindPane.cpp
        Source/Editor/Bars/BottomBar.cpp
        Source/Editor/Bars/PaintToolSettings.cpp
        Source/Editor/Bars/PatchControls.cpp
        Source/Editor/Bars/TempoDisplay.cpp
        Source/Editor/Bars/Titlebar.cpp
        Source/Editor/Canvas/AudioCommandDrainer.cpp
        Source/Editor/Canvas/CanvasFrame.cpp
        Source/Editor/Canvas/EncapsulationView.cpp
        Source/Editor/Canvas/GraphDocument.cpp
        Source/Editor/Canvas/GridMultiplier.cpp
        Source/Editor/Canvas/NodeArrowCanvas.cpp
        Source/Editor/Canvas/NodeCanvas.cpp
        Source/Editor/Canvas/NodeLimitWarning.cpp
        Source/Editor/Canvas/ValueField.cpp
        Source/Editor/Input/NodeController.cpp
        Source/Editor/Input/Gestures/EncapsulationGestures.cpp
        Source/Editor/Input/Gestures/NotePreviewGestures.cpp
        Source/Editor/Input/Gestures/SelectionGestures.cpp
        Source/Editor/Menus/AllowedTraversalsMenu.cpp
        Source/Editor/Menus/ArrowMenu.cpp
        Source/Editor/Menus/MenuArea.cpp
        Source/Editor/Menus/MenuBar.cpp
        Source/Editor/Menus/NodeMenu.cpp
        Source/Editor/Menus/TraversalMenu.cpp
        Source/Editor/Nodes/Arrow.cpp
        Source/Editor/Nodes/Encapsulator.cpp
        Source/Editor/Nodes/Modulator.cpp
        Source/Editor/Nodes/Node.cpp
        Source/Editor/Nodes/RootNode.cpp
        Source/Editor/Nodes/RootRectangle.cpp
        Source/Editor/Nodes/TraversalFlagFormat.cpp
        Source/Editor/Nodes/TraversalFlagNode.cpp
        Source/Editor/Ornaments/FigureDocument.cpp
        Source/Editor/Ornaments/OrnamentApplier.cpp
        Source/Editor/Ornaments/OrnamentBuildPanel.cpp
        Source/Editor/Ornaments/OrnamentCanvas.cpp
        Source/Editor/Ornaments/OrnamentFigureController.cpp
        Source/Editor/Ornaments/OrnamentLibrarySection.cpp
        Source/Editor/Ornaments/OrnamentMarkView.cpp
        Source/Editor/Ornaments/OrnamentMarks.cpp
        Source/Editor/Ornaments/OrnamentNode.cpp
        Source/Editor/Ornaments/OrnamentPane.cpp
        Source/Editor/Ornaments/OrnamentSelectionPanel.cpp
        Source/Editor/Ornaments/OrnamentSymbolEditor.cpp
        Source/Editor/Ornaments/OrnamentTile.cpp
        Source/Editor/Ornaments/OrnamentWindow.cpp
        Source/Editor/Rules/RuleReorderGestures.cpp
        Source/Editor/Rules/RuleTabBar.cpp
        Source/Editor/Rules/RuleTreePanel.cpp
        Source/Editor/Rules/RuleTreeView.cpp
        Source/Editor/Rules/TraversalRulesWindow.cpp
)

target_include_directories(SequenceTree PRIVATE Source)

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
        ArsNovaUI
        SequenceTreeCore
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
)

add_subdirectory(Catch2)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Catch2/extras)
include(Catch)
enable_testing()

add_executable(SequenceTree_Tests
        Tests/Unit/TraversalTests.cpp
        Tests/Unit/ScriptCompilerTests.cpp
)

target_compile_features(SequenceTree_Tests PRIVATE cxx_std_23)
target_compile_options(SequenceTree_Tests PRIVATE -Wall -Wextra -Wno-unused-parameter)
target_link_libraries(SequenceTree_Tests PRIVATE SequenceTreeCore Catch2::Catch2WithMain)

catch_discover_tests(SequenceTree_Tests)

add_executable(SequenceTree_GraphTests
        Tests/Integration/GraphTests.cpp
        Tests/Integration/OrnamentTests.cpp
        Tests/Integration/CanvasTests.cpp
        Tests/Integration/GestureTests.cpp
        Tests/Integration/EditorTests.cpp
)

target_compile_features(SequenceTree_GraphTests PRIVATE cxx_std_23)
target_compile_options(SequenceTree_GraphTests PRIVATE -Wall -Wextra -Wno-unused-parameter)

target_include_directories(SequenceTree_GraphTests PRIVATE Source JUCE/modules ${ARSNOVAUI_PATH})

target_link_libraries(SequenceTree_GraphTests PRIVATE SequenceTree Catch2::Catch2)

catch_discover_tests(SequenceTree_GraphTests DISCOVERY_MODE PRE_TEST)

if(SEQUENCETREE_REALTIME_SANITIZER)
    add_executable(SequenceTree_RealtimeTests
            Tests/Integration/RealtimeTests.cpp
    )

    target_compile_features(SequenceTree_RealtimeTests PRIVATE cxx_std_23)
    target_compile_options(SequenceTree_RealtimeTests PRIVATE -Wall -Wextra -Wno-unused-parameter)

    target_include_directories(SequenceTree_RealtimeTests PRIVATE Source JUCE/modules ${ARSNOVAUI_PATH})

    target_link_libraries(SequenceTree_RealtimeTests PRIVATE SequenceTree Catch2::Catch2)

    catch_discover_tests(SequenceTree_RealtimeTests DISCOVERY_MODE PRE_TEST)
endif()

cmake_minimum_required(VERSION 3.22)

project(SequenceTree VERSION 0.0.1)

option(SEQUENCETREE_REALTIME_SANITIZER "Build everything with -fsanitize=realtime and add SequenceTree_RealtimeTests; needs upstream clang 20 or later" OFF)

set(SEQUENCETREE_COPY_PLUGIN TRUE)

if(SEQUENCETREE_REALTIME_SANITIZER)
    add_compile_options(-fsanitize=realtime -fdelayed-template-parsing -Wno-delayed-template-parsing-in-cxx20)
    add_link_options(-fsanitize=realtime)
    set(SEQUENCETREE_COPY_PLUGIN FALSE)
endif()

# Add JUCEIO
add_subdirectory(JUCE)

# Add your plugin
juce_add_plugin(SequenceTree
        PRODUCT_NAME "SequenceTree"
        COMPANY_NAME "ArsNova"
        COPY_PLUGIN_AFTER_BUILD ${SEQUENCETREE_COPY_PLUGIN}
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT TRUE
        FORMATS AU VST3 Standalone
        PLUGIN_MANUFACTURER_CODE ANON
        PLUGIN_CODE ST01
)

juce_add_binary_data(SequenceTreeFonts
        SOURCES
        Resources/Fonts/Geist-Regular.ttf
        Resources/Fonts/Geist-SemiBold.ttf
        Resources/Fonts/GeistMono-Regular.ttf
)

juce_add_binary_data(SequenceTreeIcons
        HEADER_NAME IconData.h
        NAMESPACE IconData
        SOURCES
        Resources/Icons/Add.svg
        Resources/Icons/Arrow.svg
        Resources/Icons/BoxDash.svg
        Resources/Icons/BoxDots.svg
        Resources/Icons/BrushHandle.svg
        Resources/Icons/BrushTip.svg
        Resources/Icons/ChevronDown.svg
        Resources/Icons/ChevronUp.svg
        Resources/Icons/Circle.svg
        Resources/Icons/CircledH.svg
        Resources/Icons/Code.svg
        Resources/Icons/DashedBox.svg
        Resources/Icons/Dot.svg
        Resources/Icons/Eraser.svg
        Resources/Icons/Eye.svg
        Resources/Icons/Fork.svg
        Resources/Icons/Import.svg
        Resources/Icons/Link.svg
        Resources/Icons/Metronome.svg
        Resources/Icons/Pause.svg
        Resources/Icons/Pencil.svg
        Resources/Icons/Play.svg
        Resources/Icons/QuaverHead.svg
        Resources/Icons/QuaverStem.svg
        Resources/Icons/Redo.svg
        Resources/Icons/Reload.svg
        Resources/Icons/Remove.svg
        Resources/Icons/Ring.svg
        Resources/Icons/RoundedSquare.svg
        Resources/Icons/Route.svg
        Resources/Icons/Settings.svg
        Resources/Icons/SkipBar.svg
        Resources/Icons/SkipTriangle.svg
        Resources/Icons/Triangle.svg
        Resources/Icons/Undo.svg
        Resources/Icons/Upload.svg
        Resources/Icons/Wave.svg
)

target_compile_features(SequenceTree PRIVATE cxx_std_20)

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
        Source/Audio/ScriptRun.cpp
        Source/Script/ScriptLexer.cpp
        Source/Script/ScriptParser.cpp
        Source/Script/ScriptEmitter.cpp
        Source/Script/ScriptCompiler.cpp
        Source/Audio/NodeStateTable.cpp
        Source/Graph/RTGraphBuilder.cpp
        Source/Graph/GraphState.cpp
        Source/Graph/ArrowBindingOps.cpp
        Source/Graph/EncapsulationOps.cpp
        Source/Graph/TraversalState.cpp
        Source/Graph/TraversalRuleState.cpp
        Source/Graph/OrnamentLibraries.cpp
        Source/Graph/ValueTreeIdentifiers.cpp
        Source/Graph/NodeFactory.cpp
        Source/Input/NodeController.cpp
        Source/UI/Canvas/DynamicPort.cpp
        Source/UI/Canvas/CanvasFrame.cpp
        Source/UI/Canvas/NodeCanvas.cpp
        Source/UI/Canvas/NodeCanvasTreeListener.cpp
        Source/UI/Canvas/ValueField.cpp
        Source/UI/Canvas/AudioCommandDrainer.cpp
        Source/UI/Canvas/NodeManager.cpp
        Source/UI/Canvas/OrnamentMarks.cpp
        Source/UI/Canvas/ArrowManager.cpp
        Source/UI/Canvas/CanvasHitTester.cpp
        Source/UI/Canvas/EncapsulationView.cpp
        Source/Input/NodeCreationDispatcher.cpp
        Source/Input/ConnectionOps.cpp
        Source/Input/OrnamentApplier.cpp
        Source/Input/SelectionOps.cpp
        Source/UI/Node/Node.cpp
        Source/UI/Node/RootNode.cpp
        Source/UI/Node/Arrow.cpp
        Source/UI/Node/ArrowAnimation.cpp
        Source/UI/Editors/ValueEditor.cpp
        Source/UI/Editors/Formats/NumberFormat.cpp
        Source/UI/Editors/Formats/PitchFormat.cpp
        Source/UI/Editors/Formats/GreekLetterFormat.cpp
        Source/UI/Editors/Formats/DualIntFormat.cpp
        Source/UI/Editors/Formats/TraversalFlagFormat.cpp
        Source/UI/Editors/Formats/TextFormat.cpp
        Source/UI/Node/Modulator.cpp
        Source/UI/Node/Encapsulator.cpp
        Source/UI/Node/TraversalFlagNode.cpp
        Source/UI/Theme/CustomLookAndFeel.cpp
        Source/UI/Theme/CustomLookAndFeelNodes.cpp
        Source/UI/Theme/CustomLookAndFeelArrows.cpp
        Source/UI/Theme/CustomLookAndFeelButtons.cpp
        Source/UI/Theme/CustomLookAndFeelOrnaments.cpp
        Source/UI/Theme/CustomLookAndFeelMenus.cpp
        Source/UI/Theme/CustomTextCaret.cpp
        Source/UI/Theme/InterfaceIdentifiers.cpp
        Source/UI/Buttons/RootRectangle.cpp
        Source/UI/Buttons/IconButton.cpp
        Source/UI/Buttons/ButtonPane.cpp
        Source/UI/Buttons/ValueSlider.cpp
        Source/UI/Buttons/TempoDisplay.cpp
        Source/UI/Buttons/PaintToolSettings.cpp
        Source/UI/Menus/ItemSelector.cpp
        Source/UI/Menus/ContextMenu.cpp
        Source/UI/Menus/TraversalMenu.cpp
        Source/UI/Menus/MenuArea.cpp
        Source/UI/PanelResizer.cpp
        Source/UI/Menus/NodeMenu.cpp
        Source/UI/Menus/AllowedTraversalsMenu.cpp
        Source/UI/Menus/TraversalRulesWindow.cpp
        Source/UI/PopupWindow.cpp
        Source/UI/ScaledView.cpp
        Source/UI/Menus/ColourSelector.cpp
        Source/UI/Menus/SettingsMenu.cpp
        Source/UI/Menus/PatchControls.cpp
        Source/UI/Bars/Bar.cpp
        Source/UI/Bars/Titlebar.cpp
        Source/UI/Bars/BottomBar.cpp
        Source/UI/Ornaments/OrnamentWindow.cpp
        Source/UI/Ornaments/OrnamentBuildPanel.cpp
        Source/UI/Ornaments/OrnamentSelectionPanel.cpp
        Source/UI/Ornaments/OrnamentLibrarySection.cpp
        Source/UI/Ornaments/OrnamentTile.cpp
        Source/UI/Ornaments/OrnamentNode.cpp
        Source/UI/Ornaments/OrnamentCanvas.cpp
        Source/UI/Ornaments/OrnamentFigureController.cpp
        Source/UI/Ornaments/OrnamentSymbolEditor.cpp
        Source/UI/Bars/MenuBar.cpp
        Source/UI/LabelPanel.cpp
        Source/UI/Editors/FileLabel.cpp
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
        SequenceTreeFonts
        SequenceTreeIcons
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
)

add_subdirectory(Catch2)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Catch2/extras)
include(Catch)
enable_testing()

add_executable(SequenceTree_Tests
        Tests/TraversalTests.cpp
        Tests/ScriptCompilerTests.cpp
        Source/Audio/TraversalLogic.cpp
        Source/Audio/TraversalRule.cpp
        Source/Audio/ScriptRun.cpp
        Source/Audio/NodeStateTable.cpp
        Source/Script/ScriptLexer.cpp
        Source/Script/ScriptParser.cpp
        Source/Script/ScriptEmitter.cpp
        Source/Script/ScriptCompiler.cpp
)

target_compile_features(SequenceTree_Tests PRIVATE cxx_std_20)
target_compile_options(SequenceTree_Tests PRIVATE -Wall -Wextra -Wno-unused-parameter)
target_include_directories(SequenceTree_Tests PRIVATE Source)
target_link_libraries(SequenceTree_Tests PRIVATE Catch2::Catch2WithMain)

catch_discover_tests(SequenceTree_Tests)

add_executable(SequenceTree_GraphTests
        Tests/GraphTests.cpp
        Tests/OrnamentTests.cpp
)

target_compile_features(SequenceTree_GraphTests PRIVATE cxx_std_20)
target_compile_options(SequenceTree_GraphTests PRIVATE -Wall -Wextra -Wno-unused-parameter)

target_include_directories(SequenceTree_GraphTests PRIVATE Source JUCE/modules)

target_link_libraries(SequenceTree_GraphTests PRIVATE SequenceTree Catch2::Catch2)

catch_discover_tests(SequenceTree_GraphTests DISCOVERY_MODE PRE_TEST)

if(SEQUENCETREE_REALTIME_SANITIZER)
    add_executable(SequenceTree_RealtimeTests
            Tests/RealtimeTests.cpp
    )

    target_compile_features(SequenceTree_RealtimeTests PRIVATE cxx_std_20)
    target_compile_options(SequenceTree_RealtimeTests PRIVATE -Wall -Wextra -Wno-unused-parameter)

    target_include_directories(SequenceTree_RealtimeTests PRIVATE Source JUCE/modules)

    target_link_libraries(SequenceTree_RealtimeTests PRIVATE SequenceTree Catch2::Catch2)

    catch_discover_tests(SequenceTree_RealtimeTests DISCOVERY_MODE PRE_TEST)
endif()

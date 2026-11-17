#include "PluginDocument.h"
#include "Processor/Document/StateJson.h"
#include "Processor/Document/TypedProperties.h"

static void applySettingDefaults(const juce::ValueTree& canvasSettings)
{
    CanvasSettingsProperties settings(canvasSettings);

    if (settings.arrowXBinding.isUsingDefault()) {
        settings.arrowXBinding.setValue(settings.arrowXBinding.getDefault(), nullptr);
    }

    if (settings.arrowYBinding.isUsingDefault()) {
        settings.arrowYBinding.setValue(settings.arrowYBinding.getDefault(), nullptr);
    }

    if (settings.arrowXMultiplier.isUsingDefault()) {
        settings.arrowXMultiplier.setValue(settings.arrowXMultiplier.getDefault(), nullptr);
    }

    if (settings.arrowYMultiplier.isUsingDefault()) {
        settings.arrowYMultiplier.setValue(settings.arrowYMultiplier.getDefault(), nullptr);
    }

    if (settings.arrowXTarget.isUsingDefault()) {
        settings.arrowXTarget.setValue(settings.arrowXTarget.getDefault(), nullptr);
    }

    if (settings.arrowYTarget.isUsingDefault()) {
        settings.arrowYTarget.setValue(settings.arrowYTarget.getDefault(), nullptr);
    }
}

static juce::File claimUntitledPatch(const juce::File& folder)
{
    static juce::StringArray claimedPatches;

    static juce::CriticalSection claimedPatchesLock;

    const juce::ScopedLock claimLock(claimedPatchesLock);

    const juce::String untitledName = "Untitled";

    juce::File untitledPatch = folder.getChildFile(untitledName).withFileExtension(StateJson::patchExtension);

    int untitledNumber = 1;

    while (untitledPatch.exists() || claimedPatches.contains(untitledPatch.getFullPathName())) {
        ++untitledNumber;

        const juce::String numberedName = untitledName + " (" + juce::String(untitledNumber) + ")";

        untitledPatch = folder.getChildFile(numberedName).withFileExtension(StateJson::patchExtension);
    }

    claimedPatches.add(untitledPatch.getFullPathName());

    return untitledPatch;
}

PluginDocument::PluginDocument()
{
    const juce::File applicationSupport = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Application Support");
    const juce::File musicFolder        = juce::File::getSpecialLocation(juce::File::userMusicDirectory);

    userFolder  = musicFolder.getChildFile(JucePlugin_Name);
    patchFolder = userFolder.getChildFile("Patches");

    const juce::File untitledPatch = claimUntitledPatch(patchFolder);

    const juce::String untitledName = untitledPatch.getFileNameWithoutExtension();
    const juce::String untitledPath = untitledPatch.getFullPathName();

    PatchProperties patchProperties(patch);

    CanvasSettingsProperties settings(canvasSettings);

    traversalRuleState.ensureDefaultRule();

    patchFolder.createDirectory();

    patchProperties.patchName.setValue(untitledName, nullptr);
    patchProperties.patchFile.setValue(untitledPath, nullptr);
    settings.graphGridMultiplier   .setValue(CanvasSettingsProperties::defaultGraphGridMultiplier,    nullptr);
    settings.ornamentGridMultiplier.setValue(CanvasSettingsProperties::defaultOrnamentGridMultiplier, nullptr);

    applySettingDefaults(canvasSettings);

    ornamentLibraries.load(userFolder.getChildFile("Ornament Libraries"));
    ornamentLibraries.importLegacyLibraries(applicationSupport.getChildFile(JucePlugin_Name).getChildFile("Ornament Libraries"));
}

juce::ValueTree PluginDocument::createState(const juce::ValueTree& parameters) const
{
    juce::ValueTree state(ValueTreeIdentifiers::PluginState);

    state.addChild(graphState.nodeMap.createCopy(),         -1, nullptr);
    state.addChild(graphState.traversals.map.createCopy(),  -1, nullptr);
    state.addChild(traversalRuleState.rules.createCopy(),   -1, nullptr);
    state.addChild(traversalRuleState.folders.createCopy(), -1, nullptr);
    state.addChild(colourPresets.createCopy(),              -1, nullptr);
    state.addChild(canvasSettings.createCopy(),             -1, nullptr);
    state.addChild(parameters,                              -1, nullptr);
    state.addChild(patch.createCopy(),                      -1, nullptr);

    state.setProperty(ValueTreeIdentifiers::SchemaVersion, schemaVersion, nullptr);

    return state;
}

void PluginDocument::restore(const juce::ValueTree& restoredTree)
{
    juce::ValueTree restoredNodeMap = restoredTree;
    juce::ValueTree restoredTraversalMap;
    juce::ValueTree restoredRules;
    juce::ValueTree restoredFolders;
    juce::ValueTree restoredPresets;
    juce::ValueTree restoredCanvasSettings;
    juce::ValueTree restoredPatch;

    if (restoredTree.getType() == ValueTreeIdentifiers::PluginState) {
        restoredNodeMap        = restoredTree.getChildWithName(ValueTreeIdentifiers::NodeMap);
        restoredTraversalMap   = restoredTree.getChildWithName(ValueTreeIdentifiers::TraversalMap);
        restoredRules          = restoredTree.getChildWithName(ValueTreeIdentifiers::TraversalRules);
        restoredFolders        = restoredTree.getChildWithName(ValueTreeIdentifiers::TraversalRuleFolders);
        restoredPresets        = restoredTree.getChildWithName(ArsNovaUI::InterfaceIdentifiers::ColourPresets);
        restoredCanvasSettings = restoredTree.getChildWithName(ValueTreeIdentifiers::CanvasSettings);
        restoredPatch          = restoredTree.getChildWithName(ValueTreeIdentifiers::Patch);
    }

    graphState.replaceState(restoredNodeMap, restoredTraversalMap);
    traversalRuleState.replaceState(restoredRules, restoredFolders);

    traversalRuleState.ensureDefaultRule();

    colourPresets.copyPropertiesAndChildrenFrom(restoredPresets, nullptr);

    if (restoredCanvasSettings.isValid()) {
        canvasSettings.copyPropertiesFrom(restoredCanvasSettings, nullptr);

        applySettingDefaults(canvasSettings);
    }

    if (restoredPatch.isValid()) {
        patch.copyPropertiesFrom(restoredPatch, nullptr);
    }
}

#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include <limits>

struct NodeProperties
{
    explicit NodeProperties(juce::ValueTree node);

    static constexpr int noEncapsulatorId = -1;

    juce::CachedValue<int>          rootNodeId;
    juce::CachedValue<int>          encapsulatorId;
    juce::CachedValue<int>          encapsulatorLabel;
    juce::CachedValue<int>          countLimit;
    juce::CachedValue<int>          triggerLimit;
    juce::CachedValue<int>          loopLimit;
    juce::CachedValue<int>          switchCountLimit;
    juce::CachedValue<int>          subLoopCountLimit;
    juce::CachedValue<int>          repeatValue;
    juce::CachedValue<int>          probability;
    juce::CachedValue<int>          modAmount;
    juce::CachedValue<int>          radius;
    juce::CachedValue<int>          traversalFlagValue;
    juce::CachedValue<int>          traversalInstance;
    juce::CachedValue<juce::String> nodeColour;
    juce::CachedValue<juce::String> ornamentUid;
};

class NodePropertyTable
{
public:

    struct Row
    {
        const juce::Identifier& identifier;

        int initialValue = 0;

        bool isSetOnCreation    = false;
        bool isInheritedByChild = false;
    };

    static bool isAudioProperty(const juce::Identifier& identifier);

    static constexpr int rowCount = 12;

    static const Row rows[rowCount];
};

struct NoteProperties
{
    explicit NoteProperties(juce::ValueTree note);

    juce::CachedValue<int> midiPitch;
    juce::CachedValue<int> midiVelocity;
    juce::CachedValue<int> midiDuration;
    juce::CachedValue<int> midiChannel;
};

struct ArrowProperties
{
    explicit ArrowProperties(juce::ValueTree arrow);

    juce::CachedValue<int>    arrowType;
    juce::CachedValue<int>    xBinding;
    juce::CachedValue<int>    yBinding;
    juce::CachedValue<double> xMultiplier;
    juce::CachedValue<double> yMultiplier;
    juce::CachedValue<bool>   isSynced;
    juce::CachedValue<int>    durationOverride;
    juce::CachedValue<int>    pitchOffset;
    juce::CachedValue<int>    countLimit;
};

struct TraversalProperties
{
    explicit TraversalProperties(juce::ValueTree traversal);

    juce::CachedValue<int>          traversalId;
    juce::CachedValue<double>       tempoMultiplier;
    juce::CachedValue<juce::String> colour;
    juce::CachedValue<int>          channel;
    juce::CachedValue<int>          transpose;
    juce::CachedValue<double>       velocity;
};

struct TraversalReferenceProperties
{
    explicit TraversalReferenceProperties(juce::ValueTree reference);

    juce::CachedValue<int> traversalId;
    juce::CachedValue<int> traversalInstance;
};


struct RuleProperties
{
    explicit RuleProperties(juce::ValueTree rule);

    juce::CachedValue<juce::String> ruleName;
    juce::CachedValue<juce::String> ruleSource;
    juce::CachedValue<int>          folderId;
};

struct FolderProperties
{
    explicit FolderProperties(juce::ValueTree folder);

    juce::CachedValue<int>          folderId;
    juce::CachedValue<juce::String> folderName;
    juce::CachedValue<int>          parentFolderId;
};


struct LibraryProperties
{
    explicit LibraryProperties(juce::ValueTree library);

    juce::CachedValue<juce::String> libraryUid;
    juce::CachedValue<juce::String> libraryName;
};

struct OrnamentProperties
{
    explicit OrnamentProperties(juce::ValueTree ornament);

    static constexpr int firstFigureNodeId = 1;
    static constexpr int unlistedIndex     = std::numeric_limits<int>::max();

    juce::CachedValue<juce::String> ornamentUid;
    juce::CachedValue<juce::String> ornamentName;
    juce::CachedValue<juce::String> symbol;
    juce::CachedValue<int>          nextFigureNodeId;
    juce::CachedValue<int>          listIndex;
};

struct FigureNoteProperties
{
    explicit FigureNoteProperties(juce::ValueTree figureNote);

    juce::CachedValue<int> offset;
    juce::CachedValue<int> velocity;
};

struct PatchProperties
{
    explicit PatchProperties(juce::ValueTree patch);

    juce::CachedValue<juce::String> patchName;
    juce::CachedValue<juce::String> patchFile;
};

struct CanvasSettingsProperties
{
    explicit CanvasSettingsProperties(juce::ValueTree canvasSettings);

    juce::CachedValue<double> graphGridMultiplier;
    juce::CachedValue<double> ornamentGridMultiplier;
    juce::CachedValue<int>    arrowXBinding;
    juce::CachedValue<int>    arrowYBinding;
    juce::CachedValue<double> arrowXMultiplier;
    juce::CachedValue<double> arrowYMultiplier;
    juce::CachedValue<int>    arrowXTarget;
    juce::CachedValue<int>    arrowYTarget;

    static constexpr double defaultGraphGridMultiplier    = 2.0;
    static constexpr double defaultOrnamentGridMultiplier = 1.0;
};

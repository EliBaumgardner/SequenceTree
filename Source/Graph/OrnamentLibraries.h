#pragma once

#include "RTData.h"
#include "ValueTreeIdentifiers.h"
#include "../UI/Theme/Visuals.h"

#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>

#include <limits>
#include <map>
#include <memory>
#include <set>
#include <vector>

enum class OrnamentStatus { Draft, Published, ChangesPending };

class OrnamentLibraries : public juce::ValueTree::Listener, private juce::Timer
{
public:

    OrnamentLibraries();
    ~OrnamentLibraries() override;

    void setFigureNodePosition(juce::ValueTree ornament, int nodeId, juce::Point<int> position, juce::UndoManager* undoManager);
    void removeFigureNode     (juce::ValueTree ornament, int nodeId, juce::UndoManager* undoManager);
    void toggleHost           (juce::ValueTree ornament, int nodeId, juce::UndoManager* undoManager);

    void addDanglingArrow   (juce::ValueTree ornament, int nodeId, juce::Point<int> tipOffset, juce::UndoManager* undoManager);
    void removeDanglingArrow(juce::ValueTree ornament, juce::UndoManager* undoManager);
    void setLastNodeLength  (juce::ValueTree ornament, juce::Point<int> tipOffset, juce::UndoManager* undoManager);

    void addSymbolStroke(juce::ValueTree ornament, const juce::Path& stroke, float width, juce::UndoManager* undoManager);
    void eraseSymbol    (juce::ValueTree ornament, juce::Point<float> centre, float radius, juce::UndoManager* undoManager);

    void publish(juce::ValueTree ornament, juce::UndoManager* undoManager);

    void load                 (const juce::File& libraryFolder);
    void importLegacyLibraries(const juce::File& legacyFolder);
    void rescan               (juce::UndoManager& undoManager);

    void removeLibrary (const juce::String& libraryUid, juce::UndoManager& undoManager);
    void removeOrnament(const juce::String& uid, juce::UndoManager* undoManager);

    void timerCallback() override;

    juce::ValueTree addLibrary   (const juce::String& name, juce::UndoManager* undoManager);
    juce::ValueTree importLibrary(const juce::File& source);

    juce::ValueTree addOrnament      (juce::ValueTree library, juce::UndoManager* undoManager);
    juce::ValueTree duplicateOrnament(const juce::String& uid, juce::UndoManager* undoManager);
    juce::ValueTree findOrnament     (const juce::String& uid) const;

    juce::ValueTree addFigureNode(juce::ValueTree ornament, int parentNodeId, juce::Point<int> position, juce::UndoManager* undoManager);

    bool                      isPublishable(const juce::ValueTree& ornament) const;
    std::vector<OrnamentStep> figureSteps  (const juce::ValueTree& figure) const;
    OrnamentStatus            statusOf     (const juce::ValueTree& ornament) const;

    static std::vector<SymbolStroke> readSymbol(const juce::String& symbol);

    struct OrnamentFile
    {
        juce::String fileName;
        juce::String savedName;
    };

    struct LibraryFolder
    {
        juce::File                           folder;
        juce::Time                           savedAt;
        juce::String                         savedName;
        std::map<juce::String, OrnamentFile> ornamentFiles;
    };

    static constexpr float defaultStrokeWidth = 0.06f;

    static constexpr const char* libraryInfoFileName = "Library.ornamentlibrary";
    static constexpr const char* ornamentExtension   = ".ornament";

    static constexpr int maximumOffset          = 48;
    static constexpr int defaultVelocityPercent = 100;
    static constexpr int maximumVelocityPercent = 200;
    static constexpr int firstNodeLengthInGrid  = 2;
    static constexpr int noHostId               = -1;

    juce::ValueTree libraries { ValueTreeIdentifiers::OrnamentLibraries };
    juce::File      folder;

    std::map<juce::String, LibraryFolder> libraryFolders;

private:

    void valueTreeChildAdded     (juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved   (juce::ValueTree& parent, juce::ValueTree& child, int index) override;
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;

    void markLibraryChanged(juce::ValueTree tree);
    void saveLibrary       (const juce::ValueTree& library, LibraryFolder& libraryFolder);

    void ensureActiveOrnament();

    static juce::Time      newestChange     (const juce::File& directory);
    static juce::ValueTree readLibraryFolder(const juce::File& directory, LibraryFolder& libraryFolder);
    static juce::ValueTree readLibraryFile  (const juce::File& file);

    static constexpr int saveDelayMs           = 500;
    static constexpr int unlistedOrnamentIndex = std::numeric_limits<int>::max();

    std::set<juce::String> changedLibraryUids;

    std::unique_ptr<juce::PropertiesFile> settings;
};

#pragma once

#include "RTData.h"
#include "ValueTreeIdentifiers.h"

#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>

#include <limits>
#include <map>
#include <memory>
#include <set>
#include <vector>

/*
    The store of ornaments, grouped into libraries, and the files they are saved in. An ornament is
    a short figure played in place of one note: a chain of figure nodes, one of them the host
    standing for the note itself, each with a pitch offset and velocity, and the arrows between
    them setting each step's share of the note's length. It also has a drawn symbol, shown on the
    canvas where it is applied. Each ornament keeps an editable draft figure and, once published, a
    separate published copy, and playback only ever uses the published copy.

    It holds the libraries and ornaments and the files they are saved in. Editing an ornament's
    figure, publishing it and turning it into the OrnamentSteps NoteScheduler plays is OrnamentFigure
    (addNode, setNodePosition, toggleHost, publish, isPublishable, statusOf, steps), and editing its
    symbol is OrnamentSymbol (addStroke, erase, read). Both are stateless functions of an ornament's tree.

    Each library is saved as a folder with one .ornament file per ornament, half a second after its
    last change. load reads the folder at start-up, rescan picks up libraries changed on disk, and
    importLegacyLibraries converts the old one-file libraries once. The processor owns it; the
    ornament window edits it, OrnamentApplier and BottomBar choose ornaments from it, and
    RTGraphBuilder reads the published figures and rebuilds when they change.
*/

class OrnamentLibraries : public juce::ValueTree::Listener, private juce::Timer
{
public:

    OrnamentLibraries();
    ~OrnamentLibraries() override;

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

    static constexpr const char* libraryInfoFileName = "Library.ornamentlibrary";
    static constexpr const char* ornamentExtension   = ".ornament";

    juce::ValueTree libraries { ValueTreeIdentifiers::OrnamentLibraries };
    juce::File      folder;

    juce::CachedValue<juce::String> activeOrnamentUid;

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

    static constexpr int saveDelayMs = 500;

    std::set<juce::String> changedLibraryUids;

    std::unique_ptr<juce::PropertiesFile> settings;
};

#include "OrnamentLibraries.h"
#include "OrnamentFigure.h"
#include "Processor/Document/Graph/ArrowBindingOps.h"
#include "Processor/Document/TypedProperties.h"
#include "Core/Model/ArrowInfo.h"
#include <ArsNovaUI/Canvas/CanvasIdentifiers.h>

#include <algorithm>

OrnamentLibraries::OrnamentLibraries()
{
    activeOrnamentUid.referTo(libraries, ValueTreeIdentifiers::ActiveOrnamentUid, nullptr);

    libraries.addListener(this);
}

OrnamentLibraries::~OrnamentLibraries()
{
    timerCallback();

    libraries.removeListener(this);
}


void OrnamentLibraries::load(const juce::File& libraryFolder)
{
    juce::PropertiesFile::Options settingsOptions;
    juce::Array<juce::File>       directories;

    folder                                   = libraryFolder;
    settingsOptions.millisecondsBeforeSaving = 0;

    folder.createDirectory();

    settings = std::make_unique<juce::PropertiesFile>(folder.getChildFile("Ornaments.settings"), settingsOptions);

    directories = folder.findChildFiles(juce::File::findDirectories, false);

    directories.sort();

    libraries.removeListener(this);

    for (const juce::File& directory : directories) {
        LibraryFolder loadedFolder;

        const juce::ValueTree library = readLibraryFolder(directory, loadedFolder);

        const LibraryProperties properties(library);

        const juce::String libraryUid = properties.libraryUid;

        if (! library.isValid() || libraries.getChildWithProperty(ValueTreeIdentifiers::LibraryUid, libraryUid).isValid()) {
            continue;
        }

        libraryFolders[libraryUid] = loadedFolder;

        libraries.appendChild(library, nullptr);
    }

    const juce::String savedActiveUid = settings->getValue(ValueTreeIdentifiers::ActiveOrnamentUid.toString());

    activeOrnamentUid.setValue(savedActiveUid, nullptr);

    ensureActiveOrnament();

    libraries.addListener(this);
}

void OrnamentLibraries::importLegacyLibraries(const juce::File& legacyFolder)
{
    juce::Array<juce::File> legacyFiles = legacyFolder.findChildFiles(juce::File::findFiles, false, "*.ornaments");

    if (settings == nullptr || settings->getBoolValue(ValueTreeIdentifiers::LegacyLibrariesImported.toString())) {
        return;
    }

    legacyFiles.sort();

    for (const juce::File& file : legacyFiles) {
        importLibrary(file);
    }

    settings->setValue(ValueTreeIdentifiers::LegacyLibrariesImported.toString(), true);
}

void OrnamentLibraries::rescan(juce::UndoManager& undoManager)
{
    juce::Array<juce::File> directories = folder.findChildFiles(juce::File::findDirectories, false);
    bool                    changed     = false;

    directories.sort();

    libraries.removeListener(this);

    for (auto libraryFolder = libraryFolders.begin(); libraryFolder != libraryFolders.end(); ) {
        if (libraryFolder->second.folder.isDirectory()) {
            ++libraryFolder;

            continue;
        }

        libraries.removeChild(libraries.getChildWithProperty(ValueTreeIdentifiers::LibraryUid, libraryFolder->first), nullptr);

        libraryFolder = libraryFolders.erase(libraryFolder);
        changed       = true;
    }

    for (const juce::File& directory : directories) {
        const auto    known = std::find_if(libraryFolders.begin(), libraryFolders.end(), [&directory](const auto& entry) { return entry.second.folder == directory; });
        LibraryFolder loadedFolder;

        if (known != libraryFolders.end() && (newestChange(directory) <= known->second.savedAt || changedLibraryUids.count(known->first) > 0)) {
            continue;
        }

        const juce::ValueTree library = readLibraryFolder(directory, loadedFolder);

        const LibraryProperties properties(library);

        const juce::String libraryUid = properties.libraryUid;

        const juce::ValueTree loaded = libraries.getChildWithProperty(ValueTreeIdentifiers::LibraryUid, libraryUid);

        if (! library.isValid() || (known == libraryFolders.end() && loaded.isValid()) || (known != libraryFolders.end() && known->first != libraryUid)) {
            continue;
        }

        libraries.addChild(library, libraries.indexOf(loaded), nullptr);
        libraries.removeChild(loaded, nullptr);

        libraryFolders[libraryUid] = loadedFolder;
        changed                    = true;
    }

    libraries.addListener(this);

    if (! changed) {
        return;
    }

    ensureActiveOrnament();

    undoManager.clearUndoHistory();
}

void OrnamentLibraries::removeLibrary(const juce::String& libraryUid, juce::UndoManager& undoManager)
{
    const juce::ValueTree library = libraries.getChildWithProperty(ValueTreeIdentifiers::LibraryUid, libraryUid);

    if (! library.isValid()) {
        return;
    }

    libraries.removeChild(library, nullptr);

    undoManager.clearUndoHistory();
}

void OrnamentLibraries::removeOrnament(const juce::String& uid, juce::UndoManager* undoManager)
{
    const juce::ValueTree ornament = findOrnament(uid);
    juce::ValueTree       library  = ornament.getParent();

    library.removeChild(ornament, undoManager);
}

void OrnamentLibraries::timerCallback()
{
    stopTimer();

    for (const juce::String& libraryUid : changedLibraryUids) {
        const juce::ValueTree library = libraries.getChildWithProperty(ValueTreeIdentifiers::LibraryUid, libraryUid);

        if (! library.isValid() || ! folder.createDirectory().wasOk()) {
            continue;
        }

        saveLibrary(library, libraryFolders[libraryUid]);
    }

    changedLibraryUids.clear();
}

juce::ValueTree OrnamentLibraries::addLibrary(const juce::String& name, juce::UndoManager* undoManager)
{
    juce::ValueTree library {ValueTreeIdentifiers::OrnamentLibrary};

    LibraryProperties properties(library);

    const juce::String libraryUid = juce::Uuid().toString();

    properties.libraryUid .setValue(libraryUid, nullptr);
    properties.libraryName.setValue(name,       nullptr);

    libraries.appendChild(library, undoManager);

    return library;
}

juce::ValueTree OrnamentLibraries::importLibrary(const juce::File& source)
{
    LibraryFolder   importedFolder;
    juce::ValueTree library;
    juce::String    libraryUid;

    if (source.isDirectory()) {
        library = readLibraryFolder(source, importedFolder);
    }
    else {
        library = readLibraryFile(source);
    }

    const LibraryProperties properties(library);

    libraryUid = properties.libraryUid;

    if (! library.isValid() || libraries.getChildWithProperty(ValueTreeIdentifiers::LibraryUid, libraryUid).isValid()) {
        return {};
    }

    if (source.isDirectory()) {
        const juce::File target = folder.getNonexistentChildFile(source.getFileName(), "", false);

        if (! folder.createDirectory().wasOk() || ! source.copyDirectoryTo(target)) {
            return {};
        }

        importedFolder.folder  = target;
        importedFolder.savedAt = newestChange(target);

        libraryFolders[libraryUid] = importedFolder;
    }

    libraries.appendChild(library, nullptr);

    return library;
}

juce::ValueTree OrnamentLibraries::addOrnament(juce::ValueTree library, juce::UndoManager* undoManager)
{
    juce::ValueTree ornament {ValueTreeIdentifiers::OrnamentData};
    juce::ValueTree figure   {ValueTreeIdentifiers::OrnamentFigure};

    if (! library.isValid()) {
        library = libraries.getChild(0);
    }

    if (! library.isValid()) {
        library = addLibrary("My Ornaments", undoManager);
    }

    const int ornamentNumber = library.getNumChildren() + 1;

    const juce::String ornamentUid  = juce::Uuid().toString();
    const juce::String ornamentName = "Ornament " + juce::String(ornamentNumber);

    OrnamentProperties properties(ornament);

    juce::CachedValue<int> hostId(figure, ValueTreeIdentifiers::OrnamentHostId, nullptr, OrnamentFigure::noHostId);

    properties.ornamentUid     .setValue(ornamentUid,                           nullptr);
    properties.ornamentName    .setValue(ornamentName,                          nullptr);
    properties.symbol          .setValue({},                                    nullptr);
    properties.nextFigureNodeId.setValue(OrnamentProperties::firstFigureNodeId, nullptr);
    hostId                     .setValue(OrnamentFigure::noHostId,              nullptr);

    ornament.appendChild(figure, nullptr);

    library.appendChild(ornament, undoManager);

    return ornament;
}

juce::ValueTree OrnamentLibraries::duplicateOrnament(const juce::String& uid, juce::UndoManager* undoManager)
{
    const juce::ValueTree source    = findOrnament(uid);
    juce::ValueTree       library   = source.getParent();
    juce::ValueTree       duplicate = source.createCopy();

    const OrnamentProperties sourceProperties(source);

    OrnamentProperties duplicateProperties(duplicate);

    const juce::String sourceName   = sourceProperties.ornamentName;
    const juce::String copyName     = sourceName + " copy";
    const juce::String duplicateUid = juce::Uuid().toString();

    if (! source.isValid()) {
        return {};
    }

    duplicateProperties.ornamentUid .setValue(duplicateUid, nullptr);
    duplicateProperties.ornamentName.setValue(copyName,     nullptr);

    const int duplicateIndex = library.indexOf(source) + 1;

    library.addChild(duplicate, duplicateIndex, undoManager);

    return duplicate;
}

juce::ValueTree OrnamentLibraries::findOrnament(const juce::String& uid) const
{
    for (const juce::ValueTree& library : libraries) {
        const juce::ValueTree ornament = library.getChildWithProperty(ValueTreeIdentifiers::OrnamentUid, uid);

        if (ornament.isValid()) {
            return ornament;
        }
    }

    return {};
}


void OrnamentLibraries::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    markLibraryChanged(child);

    ensureActiveOrnament();
}

void OrnamentLibraries::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index)
{
    const LibraryProperties removed(child);

    const juce::String removedUid = removed.libraryUid;

    const auto libraryFolder = libraryFolders.find(removedUid);

    if (parent != libraries) {
        markLibraryChanged(parent);
    }

    if (parent == libraries && libraryFolder != libraryFolders.end()) {
        libraryFolder->second.folder.moveToTrash();

        libraryFolders.erase(libraryFolder);
    }

    ensureActiveOrnament();
}

void OrnamentLibraries::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    if (property != ValueTreeIdentifiers::ActiveOrnamentUid) {
        markLibraryChanged(tree);

        return;
    }

    if (settings != nullptr) {
        settings->setValue(ValueTreeIdentifiers::ActiveOrnamentUid.toString(), tree.getProperty(property));
    }
}

void OrnamentLibraries::markLibraryChanged(juce::ValueTree tree)
{
    while (tree.isValid() && tree.getType() != ValueTreeIdentifiers::OrnamentLibrary) {
        tree = tree.getParent();
    }

    if (! tree.isValid()) {
        return;
    }

    const LibraryProperties changed(tree);

    changedLibraryUids.insert(changed.libraryUid);

    startTimer(saveDelayMs);
}

void OrnamentLibraries::saveLibrary(const juce::ValueTree& library, LibraryFolder& libraryFolder)
{
    const LibraryProperties properties(library);

    const juce::String libraryName = properties.libraryName;

    juce::String folderName = juce::File::createLegalFileName(libraryName.trim());

    juce::ValueTree libraryInfo {ValueTreeIdentifiers::OrnamentLibrary};

    if (folderName.isEmpty()) {
        folderName = "Library";
    }

    if (libraryFolder.folder == juce::File()) {
        libraryFolder.folder = folder.getNonexistentChildFile(folderName, "", false);
    }

    if (libraryFolder.savedName != libraryName && libraryFolder.folder.isDirectory()) {
        const juce::File renamed = folder.getNonexistentChildFile(folderName, "", false);

        if (libraryFolder.folder.moveFileTo(renamed)) {
            libraryFolder.folder = renamed;
        }
    }

    libraryFolder.savedName = libraryName;

    if (! libraryFolder.folder.createDirectory().wasOk()) {
        return;
    }

    libraryInfo.copyPropertiesFrom(library, nullptr);

    const juce::TemporaryFile               infoFile(libraryFolder.folder.getChildFile(libraryInfoFileName));
    const std::unique_ptr<juce::XmlElement> infoXml = libraryInfo.createXml();

    if (infoXml->writeTo(infoFile.getFile())) {
        infoFile.overwriteTargetFileWithTemporary();
    }

    for (int index = 0; index < library.getNumChildren(); ++index) {
        const juce::ValueTree ornament = library.getChild(index);

        const OrnamentProperties ornamentInfo(ornament);

        const juce::String ornamentUid  = ornamentInfo.ornamentUid;
        const juce::String ornamentName = ornamentInfo.ornamentName;

        juce::String                            fileName     = juce::File::createLegalFileName(ornamentName.trim());
        OrnamentFile&                           ornamentFile = libraryFolder.ornamentFiles[ornamentUid];
        const juce::String                      previousName = ornamentFile.fileName;
        const std::unique_ptr<juce::XmlElement> xml          = ornament.createXml();

        if (fileName.isEmpty()) {
            fileName = "Ornament";
        }

        if (previousName.isEmpty() || ornamentFile.savedName != ornamentName) {
            ornamentFile.fileName = libraryFolder.folder.getNonexistentChildFile(fileName, ornamentExtension, false).getFileName();
        }

        ornamentFile.savedName = ornamentName;

        xml->setAttribute(ValueTreeIdentifiers::OrnamentIndex, index);

        const juce::TemporaryFile temporaryFile(libraryFolder.folder.getChildFile(ornamentFile.fileName));

        if (xml->writeTo(temporaryFile.getFile()) && temporaryFile.overwriteTargetFileWithTemporary()
            && previousName.isNotEmpty() && previousName != ornamentFile.fileName) {
            libraryFolder.folder.getChildFile(previousName).deleteFile();
        }
    }

    for (auto entry = libraryFolder.ornamentFiles.begin(); entry != libraryFolder.ornamentFiles.end(); ) {
        if (library.getChildWithProperty(ValueTreeIdentifiers::OrnamentUid, entry->first).isValid()) {
            ++entry;

            continue;
        }

        libraryFolder.folder.getChildFile(entry->second.fileName).deleteFile();

        entry = libraryFolder.ornamentFiles.erase(entry);
    }

    libraryFolder.savedAt = newestChange(libraryFolder.folder);
}

void OrnamentLibraries::ensureActiveOrnament()
{
    juce::String activeUid;

    const juce::String storedActiveUid = activeOrnamentUid;

    if (findOrnament(storedActiveUid).isValid()) {
        return;
    }

    for (const juce::ValueTree& library : libraries) {
        if (library.getNumChildren() > 0) {
            const OrnamentProperties firstOrnament(library.getChild(0));

            activeUid = firstOrnament.ornamentUid;

            break;
        }
    }

    activeOrnamentUid.setValue(activeUid, nullptr);
}

juce::Time OrnamentLibraries::newestChange(const juce::File& directory)
{
    juce::Time newest = directory.getLastModificationTime();

    for (const juce::File& file : directory.findChildFiles(juce::File::findFiles, false)) {
        newest = std::max(newest, file.getLastModificationTime());
    }

    return newest;
}

juce::ValueTree OrnamentLibraries::readLibraryFolder(const juce::File& directory, LibraryFolder& libraryFolder)
{
    const std::unique_ptr<juce::XmlElement> infoXml   = juce::XmlDocument::parse(directory.getChildFile(libraryInfoFileName));
    juce::Array<juce::File>                 fileList  = directory.findChildFiles(juce::File::findFiles, false, juce::String("*") + ornamentExtension);
    std::vector<juce::ValueTree>            ornaments;
    juce::ValueTree                         library;

    if (infoXml == nullptr) {
        return {};
    }

    library = juce::ValueTree::fromXml(*infoXml);

    const LibraryProperties properties(library);

    const juce::String libraryUid = properties.libraryUid;

    if (library.getType() != ValueTreeIdentifiers::OrnamentLibrary || libraryUid.isEmpty()) {
        return {};
    }

    library.removeAllChildren(nullptr);

    fileList.sort();

    for (const juce::File& file : fileList) {
        const std::unique_ptr<juce::XmlElement> xml = juce::XmlDocument::parse(file);
        juce::ValueTree                         ornament;
        juce::String                            ornamentUid;

        if (xml == nullptr) {
            continue;
        }

        ornament = juce::ValueTree::fromXml(*xml);

        const OrnamentProperties ornamentInfo(ornament);

        ornamentUid = ornamentInfo.ornamentUid;

        if (ornament.getType() != ValueTreeIdentifiers::OrnamentData || ornamentUid.isEmpty()
            || ! ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure).isValid() || libraryFolder.ornamentFiles.count(ornamentUid) > 0) {
            continue;
        }

        libraryFolder.ornamentFiles[ornamentUid] = OrnamentFile { file.getFileName(), ornamentInfo.ornamentName };

        ornaments.push_back(ornament);
    }

    std::stable_sort(ornaments.begin(), ornaments.end(), [](const juce::ValueTree& first, const juce::ValueTree& second) {
        const OrnamentProperties firstInfo(first);
        const OrnamentProperties secondInfo(second);

        return firstInfo.listIndex < secondInfo.listIndex;
    });

    for (juce::ValueTree& ornament : ornaments) {
        OrnamentProperties ornamentInfo(ornament);

        ornamentInfo.listIndex.resetToDefault(nullptr);

        library.appendChild(ornament, nullptr);
    }

    libraryFolder.folder    = directory;
    libraryFolder.savedAt   = newestChange(directory);
    libraryFolder.savedName = properties.libraryName;

    return library;
}

juce::ValueTree OrnamentLibraries::readLibraryFile(const juce::File& file)
{
    const std::unique_ptr<juce::XmlElement> xml = juce::XmlDocument::parse(file);
    juce::ValueTree                         library;

    if (xml == nullptr) {
        return {};
    }

    library = juce::ValueTree::fromXml(*xml);

    const LibraryProperties properties(library);

    const juce::String libraryUid = properties.libraryUid;

    if (library.getType() != ValueTreeIdentifiers::OrnamentLibrary || libraryUid.isEmpty()) {
        return {};
    }

    for (int index = library.getNumChildren() - 1; index >= 0; --index) {
        const juce::ValueTree ornament = library.getChild(index);

        const OrnamentProperties ornamentInfo(ornament);

        const juce::String ornamentUid = ornamentInfo.ornamentUid;

        if (ornament.getType() != ValueTreeIdentifiers::OrnamentData
            || ornamentUid.isEmpty()
            || ! ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure).isValid()) {
            library.removeChild(index, nullptr);
        }
    }

    return library;
}

#include "OrnamentLibraries.h"
#include "ArrowBindingOps.h"
#include "../Util/ArrowInfo.h"

#include <algorithm>

OrnamentLibraries::OrnamentLibraries()
{
    libraries.addListener(this);
}

OrnamentLibraries::~OrnamentLibraries()
{
    timerCallback();

    libraries.removeListener(this);
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

void OrnamentLibraries::saveLibrary(const juce::ValueTree& library, LibraryFolder& libraryFolder)
{
    const juce::String libraryName = library.getProperty(ValueTreeIdentifiers::LibraryName).toString();
    juce::String       folderName  = juce::File::createLegalFileName(libraryName.trim());
    juce::ValueTree    libraryInfo {ValueTreeIdentifiers::OrnamentLibrary};

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
        const juce::ValueTree                   ornament     = library.getChild(index);
        const juce::String                      ornamentUid  = ornament.getProperty(ValueTreeIdentifiers::OrnamentUid).toString();
        const juce::String                      ornamentName = ornament.getProperty(ValueTreeIdentifiers::OrnamentName).toString();
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

juce::Time OrnamentLibraries::newestChange(const juce::File& directory)
{
    juce::Time newest = directory.getLastModificationTime();

    for (const juce::File& file : directory.findChildFiles(juce::File::findFiles, false)) {
        newest = std::max(newest, file.getLastModificationTime());
    }

    return newest;
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
        LibraryFolder         loadedFolder;
        const juce::ValueTree library    = readLibraryFolder(directory, loadedFolder);
        const juce::String    libraryUid = library.getProperty(ValueTreeIdentifiers::LibraryUid).toString();

        if (! library.isValid() || libraries.getChildWithProperty(ValueTreeIdentifiers::LibraryUid, libraryUid).isValid()) {
            continue;
        }

        libraryFolders[libraryUid] = loadedFolder;

        libraries.appendChild(library, nullptr);
    }

    libraries.setProperty(ValueTreeIdentifiers::ActiveOrnamentUid, settings->getValue(ValueTreeIdentifiers::ActiveOrnamentUid.toString()), nullptr);

    ensureActiveOrnament();

    libraries.addListener(this);
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

    if (library.getType() != ValueTreeIdentifiers::OrnamentLibrary || library.getProperty(ValueTreeIdentifiers::LibraryUid).toString().isEmpty()) {
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

        ornament    = juce::ValueTree::fromXml(*xml);
        ornamentUid = ornament.getProperty(ValueTreeIdentifiers::OrnamentUid).toString();

        if (ornament.getType() != ValueTreeIdentifiers::OrnamentData || ornamentUid.isEmpty()
            || ! ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure).isValid() || libraryFolder.ornamentFiles.count(ornamentUid) > 0) {
            continue;
        }

        libraryFolder.ornamentFiles[ornamentUid] = OrnamentFile { file.getFileName(), ornament.getProperty(ValueTreeIdentifiers::OrnamentName).toString() };

        ornaments.push_back(ornament);
    }

    std::stable_sort(ornaments.begin(), ornaments.end(), [](const juce::ValueTree& first, const juce::ValueTree& second) {
        return static_cast<int>(first.getProperty(ValueTreeIdentifiers::OrnamentIndex, unlistedOrnamentIndex))
               < static_cast<int>(second.getProperty(ValueTreeIdentifiers::OrnamentIndex, unlistedOrnamentIndex));
    });

    for (juce::ValueTree& ornament : ornaments) {
        ornament.removeProperty(ValueTreeIdentifiers::OrnamentIndex, nullptr);

        library.appendChild(ornament, nullptr);
    }

    libraryFolder.folder    = directory;
    libraryFolder.savedAt   = newestChange(directory);
    libraryFolder.savedName = library.getProperty(ValueTreeIdentifiers::LibraryName).toString();

    return library;
}

void OrnamentLibraries::ensureActiveOrnament()
{
    juce::String activeUid;

    if (findOrnament(libraries.getProperty(ValueTreeIdentifiers::ActiveOrnamentUid).toString()).isValid()) {
        return;
    }

    for (const juce::ValueTree& library : libraries) {
        if (library.getNumChildren() > 0) {
            activeUid = library.getChild(0).getProperty(ValueTreeIdentifiers::OrnamentUid).toString();

            break;
        }
    }

    libraries.setProperty(ValueTreeIdentifiers::ActiveOrnamentUid, activeUid, nullptr);
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

    libraryUid = library.getProperty(ValueTreeIdentifiers::LibraryUid).toString();

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

juce::ValueTree OrnamentLibraries::readLibraryFile(const juce::File& file)
{
    const std::unique_ptr<juce::XmlElement> xml = juce::XmlDocument::parse(file);
    juce::ValueTree                         library;

    if (xml == nullptr) {
        return {};
    }

    library = juce::ValueTree::fromXml(*xml);

    if (library.getType() != ValueTreeIdentifiers::OrnamentLibrary || library.getProperty(ValueTreeIdentifiers::LibraryUid).toString().isEmpty()) {
        return {};
    }

    for (int index = library.getNumChildren() - 1; index >= 0; --index) {
        const juce::ValueTree ornament = library.getChild(index);

        if (ornament.getType() != ValueTreeIdentifiers::OrnamentData
            || ornament.getProperty(ValueTreeIdentifiers::OrnamentUid).toString().isEmpty()
            || ! ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure).isValid()) {
            library.removeChild(index, nullptr);
        }
    }

    return library;
}

juce::ValueTree OrnamentLibraries::addLibrary(const juce::String& name, juce::UndoManager* undoManager)
{
    juce::ValueTree library {ValueTreeIdentifiers::OrnamentLibrary};

    library.setProperty(ValueTreeIdentifiers::LibraryUid,  juce::Uuid().toString(), nullptr);
    library.setProperty(ValueTreeIdentifiers::LibraryName, name,                    nullptr);

    libraries.appendChild(library, undoManager);

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

    ornament.setProperty(ValueTreeIdentifiers::OrnamentUid,      juce::Uuid().toString(),                                  nullptr);
    ornament.setProperty(ValueTreeIdentifiers::OrnamentName,     "Ornament " + juce::String(library.getNumChildren() + 1), nullptr);
    ornament.setProperty(ValueTreeIdentifiers::OrnamentSymbol,   juce::String(),                                           nullptr);
    ornament.setProperty(ValueTreeIdentifiers::NextFigureNodeId, 1,                                                        nullptr);

    figure.setProperty(ValueTreeIdentifiers::OrnamentHostId, noHostId, nullptr);

    ornament.appendChild(figure, nullptr);

    library.appendChild(ornament, undoManager);

    return ornament;
}

juce::ValueTree OrnamentLibraries::duplicateOrnament(const juce::String& uid, juce::UndoManager* undoManager)
{
    const juce::ValueTree source    = findOrnament(uid);
    const juce::String    copyName  = source.getProperty(ValueTreeIdentifiers::OrnamentName).toString() + " copy";
    juce::ValueTree       library   = source.getParent();
    juce::ValueTree       duplicate = source.createCopy();

    if (! source.isValid()) {
        return {};
    }

    duplicate.setProperty(ValueTreeIdentifiers::OrnamentUid,  juce::Uuid().toString(), nullptr);
    duplicate.setProperty(ValueTreeIdentifiers::OrnamentName, copyName,                nullptr);

    library.addChild(duplicate, library.indexOf(source) + 1, undoManager);

    return duplicate;
}

juce::ValueTree OrnamentLibraries::addFigureNode(juce::ValueTree ornament, int parentNodeId, juce::Point<int> position, juce::UndoManager* undoManager)
{
    juce::ValueTree figure = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);
    juce::ValueTree parent = figure.getChildWithProperty(ValueTreeIdentifiers::Id, parentNodeId);
    const int       nodeId = ornament.getProperty(ValueTreeIdentifiers::NextFigureNodeId, 1);
    juce::ValueTree node   {ValueTreeIdentifiers::OrnamentNoteData};
    ArrowInfo       arrowInfo;

    if (! figure.isValid() || (parentNodeId == -1 && figure.getNumChildren() > 0)) {
        return {};
    }

    if (parentNodeId != -1 && (! parent.isValid() || parent.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds).getNumChildren() > 0)) {
        return {};
    }

    arrowInfo.xBinding = ArrowBinding::DurationBind;
    arrowInfo.yBinding = ArrowBinding::NoBind;

    node.setProperty(ValueTreeIdentifiers::Id,               nodeId,                 nullptr);
    node.setProperty(ValueTreeIdentifiers::XPosition,        position.x,             nullptr);
    node.setProperty(ValueTreeIdentifiers::YPosition,        position.y,             nullptr);
    node.setProperty(ValueTreeIdentifiers::OrnamentOffset,   0,                      nullptr);
    node.setProperty(ValueTreeIdentifiers::OrnamentVelocity, defaultVelocityPercent, nullptr);

    node.appendChild(juce::ValueTree(ValueTreeIdentifiers::NodeChildrenIds), nullptr);

    if (parent.isValid()) {
        juce::ValueTree connection {ValueTreeIdentifiers::NodeId};

        connection.setProperty(ValueTreeIdentifiers::Id, nodeId, nullptr);

        ArrowBindingOps::setArrowInfo(connection, arrowInfo, nullptr);

        parent.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds).appendChild(connection, undoManager);
        parent.removeChild(parent.getChildWithName(ValueTreeIdentifiers::DanglingArrows), undoManager);
    }

    ornament.setProperty(ValueTreeIdentifiers::NextFigureNodeId, nodeId + 1, undoManager);

    figure.appendChild(node, undoManager);

    return node;
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

        const juce::ValueTree library    = readLibraryFolder(directory, loadedFolder);
        const juce::String    libraryUid = library.getProperty(ValueTreeIdentifiers::LibraryUid).toString();
        const juce::ValueTree loaded     = libraries.getChildWithProperty(ValueTreeIdentifiers::LibraryUid, libraryUid);

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

void OrnamentLibraries::removeFigureNode(juce::ValueTree ornament, int nodeId, juce::UndoManager* undoManager)
{
    juce::ValueTree       figure   = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);
    const juce::ValueTree node     = figure.getChildWithProperty(ValueTreeIdentifiers::Id, nodeId);
    const juce::ValueTree outgoing = node.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds).getChild(0);
    juce::ValueTree       parent;
    juce::ValueTree       incoming;

    if (! node.isValid()) {
        return;
    }

    if (static_cast<int>(figure.getProperty(ValueTreeIdentifiers::OrnamentHostId, noHostId)) == nodeId) {
        figure.setProperty(ValueTreeIdentifiers::OrnamentHostId, noHostId, undoManager);
    }

    for (const juce::ValueTree& candidate : figure) {
        const juce::ValueTree connection = candidate.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds).getChildWithProperty(ValueTreeIdentifiers::Id, nodeId);

        if (connection.isValid()) {
            parent   = candidate;
            incoming = connection;
        }
    }

    if (incoming.isValid() && outgoing.isValid()) {
        incoming.setProperty(ValueTreeIdentifiers::Id, outgoing.getProperty(ValueTreeIdentifiers::Id), undoManager);
    }

    if (incoming.isValid() && ! outgoing.isValid()) {
        const juce::ValueTree danglingArrows = node.getChildWithName(ValueTreeIdentifiers::DanglingArrows);

        parent.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds).removeChild(incoming, undoManager);

        if (danglingArrows.isValid() && danglingArrows.getNumChildren() > 0) {
            parent.appendChild(danglingArrows.createCopy(), undoManager);
        }
    }

    figure.removeChild(node, undoManager);
}

void OrnamentLibraries::toggleHost(juce::ValueTree ornament, int nodeId, juce::UndoManager* undoManager)
{
    juce::ValueTree figure = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);
    int             hostId = nodeId;

    if (! figure.getChildWithProperty(ValueTreeIdentifiers::Id, nodeId).isValid()) {
        return;
    }

    if (static_cast<int>(figure.getProperty(ValueTreeIdentifiers::OrnamentHostId, noHostId)) == nodeId) {
        hostId = noHostId;
    }

    figure.setProperty(ValueTreeIdentifiers::OrnamentHostId, hostId, undoManager);
}

void OrnamentLibraries::setFigureNodePosition(juce::ValueTree ornament, int nodeId, juce::Point<int> position, juce::UndoManager* undoManager)
{
    const juce::ValueTree  figure  = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);
    juce::ValueTree        node    = figure.getChildWithProperty(ValueTreeIdentifiers::Id, nodeId);
    const juce::Point<int> delta   = position - juce::Point<int> { node.getProperty(ValueTreeIdentifiers::XPosition), node.getProperty(ValueTreeIdentifiers::YPosition) };
    const int              oldRow  = juce::roundToInt(static_cast<double>(node.getProperty(ValueTreeIdentifiers::YPosition)) / ArrowInfo::pixelsPerGridSpace);
    const int              newRow  = juce::roundToInt(position.y / ArrowInfo::pixelsPerGridSpace);
    const int              shifted = static_cast<int>(node.getProperty(ValueTreeIdentifiers::OrnamentOffset))
                                     + juce::roundToInt((oldRow - newRow) * ArrowInfo::semitonesPerGridSpace);
    juce::ValueTree        moved   = node;

    auto clearDurationOverride = [undoManager](juce::ValueTree arrowTree) {
        if (static_cast<int>(arrowTree.getProperty(ValueTreeIdentifiers::ArrowDuration, ArrowInfo::noDurationOverride)) != ArrowInfo::noDurationOverride) {
            arrowTree.setProperty(ValueTreeIdentifiers::ArrowDuration, ArrowInfo::noDurationOverride, undoManager);
        }
    };

    if (! node.isValid()) {
        return;
    }

    node.setProperty(ValueTreeIdentifiers::OrnamentOffset, std::clamp(shifted, -maximumOffset, maximumOffset), undoManager);

    for (const juce::ValueTree& candidate : figure) {
        clearDurationOverride(candidate.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds).getChildWithProperty(ValueTreeIdentifiers::Id, nodeId));
    }

    while (moved.isValid()) {
        const juce::ValueTree outgoing = moved.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds).getChild(0);

        moved.setProperty(ValueTreeIdentifiers::XPosition, static_cast<int>(moved.getProperty(ValueTreeIdentifiers::XPosition)) + delta.x, undoManager);
        moved.setProperty(ValueTreeIdentifiers::YPosition, static_cast<int>(moved.getProperty(ValueTreeIdentifiers::YPosition)) + delta.y, undoManager);

        clearDurationOverride(outgoing);

        moved = figure.getChildWithProperty(ValueTreeIdentifiers::Id, outgoing.getProperty(ValueTreeIdentifiers::Id, -1));
    }
}

void OrnamentLibraries::setLastNodeLength(juce::ValueTree ornament, juce::Point<int> tipOffset, juce::UndoManager* undoManager)
{
    const juce::ValueTree figure = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);

    for (const juce::ValueTree& node : figure) {
        juce::ValueTree danglingArrow = node.getChildWithName(ValueTreeIdentifiers::DanglingArrows).getChild(0);

        if (danglingArrow.isValid()) {
            danglingArrow.setProperty(ValueTreeIdentifiers::ArrowTipX, tipOffset.x, undoManager);
            danglingArrow.setProperty(ValueTreeIdentifiers::ArrowTipY, tipOffset.y, undoManager);

            return;
        }
    }

    for (const juce::ValueTree& node : figure) {
        if (node.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds).getNumChildren() == 0) {
            addDanglingArrow(ornament, node.getProperty(ValueTreeIdentifiers::Id), tipOffset, undoManager);

            return;
        }
    }
}

void OrnamentLibraries::addDanglingArrow(juce::ValueTree ornament, int nodeId, juce::Point<int> tipOffset, juce::UndoManager* undoManager)
{
    const juce::ValueTree figure         = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);
    juce::ValueTree       node           = figure.getChildWithProperty(ValueTreeIdentifiers::Id, nodeId);
    juce::ValueTree       danglingArrows = node.getChildWithName(ValueTreeIdentifiers::DanglingArrows);
    juce::ValueTree       danglingArrow  {ValueTreeIdentifiers::DanglingArrow};
    ArrowInfo             arrowInfo;

    if (! node.isValid()) {
        return;
    }

    if (! danglingArrows.isValid()) {
        danglingArrows = juce::ValueTree(ValueTreeIdentifiers::DanglingArrows);

        node.appendChild(danglingArrows, undoManager);
    }

    if (danglingArrows.getNumChildren() > 0) {
        danglingArrows.getChild(0).setProperty(ValueTreeIdentifiers::ArrowTipX, tipOffset.x, undoManager);
        danglingArrows.getChild(0).setProperty(ValueTreeIdentifiers::ArrowTipY, tipOffset.y, undoManager);

        return;
    }

    arrowInfo.xBinding = ArrowBinding::DurationBind;
    arrowInfo.yBinding = ArrowBinding::NoBind;

    danglingArrow.setProperty(ValueTreeIdentifiers::ArrowTipX, tipOffset.x, nullptr);
    danglingArrow.setProperty(ValueTreeIdentifiers::ArrowTipY, tipOffset.y, nullptr);

    ArrowBindingOps::setArrowInfo(danglingArrow, arrowInfo, nullptr);

    danglingArrows.appendChild(danglingArrow, undoManager);
}

void OrnamentLibraries::removeDanglingArrow(juce::ValueTree ornament, juce::UndoManager* undoManager)
{
    const juce::ValueTree figure = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);

    for (juce::ValueTree node : figure) {
        const juce::ValueTree danglingArrows = node.getChildWithName(ValueTreeIdentifiers::DanglingArrows);

        if (danglingArrows.isValid()) {
            node.removeChild(danglingArrows, undoManager);
        }
    }
}

void OrnamentLibraries::addSymbolStroke(juce::ValueTree ornament, const juce::Path& stroke, float width, juce::UndoManager* undoManager)
{
    juce::String symbol = ornament.getProperty(ValueTreeIdentifiers::OrnamentSymbol).toString();

    if (symbol.isNotEmpty()) {
        symbol << "|";
    }

    symbol << juce::String(width) << " " << stroke.toString();

    ornament.setProperty(ValueTreeIdentifiers::OrnamentSymbol, symbol, undoManager);
}

void OrnamentLibraries::eraseSymbol(juce::ValueTree ornament, juce::Point<float> centre, float radius, juce::UndoManager* undoManager)
{
    juce::StringArray erasedStrokes;

    for (const SymbolStroke& stroke : readSymbol(ornament.getProperty(ValueTreeIdentifiers::OrnamentSymbol).toString())) {
        juce::Path           erased;
        juce::Point<float>   current;
        bool                 penDown = false;
        juce::Path::Iterator element(stroke.path);

        while (element.next()) {
            const juce::Point<float> end(element.x1, element.y1);

            if (element.elementType != juce::Path::Iterator::lineTo) {
                current = end;
                penDown = false;

                continue;
            }

            const juce::Point<float> along        = end - current;
            const juce::Point<float> toCentre     = centre - current;
            const float              length       = along.getDistanceFromOrigin();
            const juce::Point<float> direction    = along / length;
            const float              alongCentre  = toCentre.getDotProduct(direction);
            const float              acrossCentre = std::abs(direction.x * toCentre.y - direction.y * toCentre.x);
            const float              gap          = juce::jmax(0.0f, acrossCentre - stroke.width * 0.5f);
            float                    enter        = 1.0f;
            float                    leave        = 1.0f;

            if (length > 0.0f && gap < radius) {
                enter = (alongCentre - std::sqrt(radius * radius - gap * gap)) / length;
                leave = (alongCentre + std::sqrt(radius * radius - gap * gap)) / length;
            }

            if (leave <= 0.0f || enter >= 1.0f) {
                enter = 1.0f;
                leave = 1.0f;
            }

            if (enter > 0.0f && ! penDown) {
                erased.startNewSubPath(current);
            }

            if (enter > 0.0f) {
                erased.lineTo(current + along * enter);
            }

            if (leave < 1.0f) {
                erased.startNewSubPath(current + along * juce::jmax(leave, 0.0f));
                erased.lineTo(end);
            }

            penDown = enter >= 1.0f || leave < 1.0f;
            current = end;
        }

        if (! erased.isEmpty()) {
            erasedStrokes.add(juce::String(stroke.width) + " " + erased.toString());
        }
    }

    ornament.setProperty(ValueTreeIdentifiers::OrnamentSymbol, erasedStrokes.joinIntoString("|"), undoManager);
}

std::vector<SymbolStroke> OrnamentLibraries::readSymbol(const juce::String& symbol)
{
    std::vector<SymbolStroke> strokes;

    for (const juce::String& entry : juce::StringArray::fromTokens(symbol, "|", "")) {
        const juce::String widthText = entry.upToFirstOccurrenceOf(" ", false, false);
        SymbolStroke       stroke { juce::Path(), defaultStrokeWidth };

        if (widthText.containsOnly("0123456789.")) {
            stroke.width = widthText.getFloatValue();

            stroke.path.restoreFromString(entry.fromFirstOccurrenceOf(" ", false, false));
        }
        else {
            stroke.path.restoreFromString(entry);
        }

        strokes.push_back(stroke);
    }

    return strokes;
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

void OrnamentLibraries::publish(juce::ValueTree ornament, juce::UndoManager* undoManager)
{
    juce::ValueTree published {ValueTreeIdentifiers::PublishedFigure};

    if (! isPublishable(ornament)) {
        return;
    }

    published.copyPropertiesAndChildrenFrom(ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure), nullptr);

    ornament.removeChild(ornament.getChildWithName(ValueTreeIdentifiers::PublishedFigure), undoManager);
    ornament.appendChild(published, undoManager);
}

bool OrnamentLibraries::isPublishable(const juce::ValueTree& ornament) const
{
    const juce::ValueTree figure      = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);
    int                   totalWeight = 0;

    for (const OrnamentStep& step : figureSteps(figure)) {
        totalWeight += step.durationWeight;
    }

    return figure.getChildWithProperty(ValueTreeIdentifiers::Id, figure.getProperty(ValueTreeIdentifiers::OrnamentHostId, noHostId)).isValid()
           && figure.getNumChildren() > 1 && totalWeight > 0;
}

std::vector<OrnamentStep> OrnamentLibraries::figureSteps(const juce::ValueTree& figure) const
{
    const int                 hostId = figure.getProperty(ValueTreeIdentifiers::OrnamentHostId, noHostId);
    std::vector<OrnamentStep> steps;
    std::set<int>             pointedAt;
    juce::ValueTree           node;
    int                       previousDurationWeight = 0;

    for (const juce::ValueTree& candidate : figure) {
        for (const juce::ValueTree& connection : candidate.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds)) {
            pointedAt.insert(static_cast<int>(connection.getProperty(ValueTreeIdentifiers::Id)));
        }
    }

    for (const juce::ValueTree& candidate : figure) {
        if (pointedAt.count(static_cast<int>(candidate.getProperty(ValueTreeIdentifiers::Id))) == 0) {
            node = candidate;

            break;
        }
    }

    for (int visited = 0; node.isValid() && visited < figure.getNumChildren(); ++visited) {
        const juce::ValueTree connection    = node.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds).getChild(0);
        const juce::ValueTree danglingArrow = node.getChildWithName(ValueTreeIdentifiers::DanglingArrows).getChild(0);
        const int             nodeX         = node.getProperty(ValueTreeIdentifiers::XPosition);
        const int             nodeY         = node.getProperty(ValueTreeIdentifiers::YPosition);
        juce::ValueTree       next;
        OrnamentStep          step { OrnamentNodeKind::Note, node.getProperty(ValueTreeIdentifiers::OrnamentOffset),
                                     node.getProperty(ValueTreeIdentifiers::OrnamentVelocity), 0 };

        if (connection.isValid()) {
            next = figure.getChildWithProperty(ValueTreeIdentifiers::Id, connection.getProperty(ValueTreeIdentifiers::Id));
        }

        if (static_cast<int>(node.getProperty(ValueTreeIdentifiers::Id)) == hostId) {
            step.kind = OrnamentNodeKind::Host;
        }

        if (next.isValid()) {
            step.durationWeight = ArrowInfo::durationFromDelta(ArrowBindingOps::getArrowInfo(connection),
                                                               static_cast<int>(next.getProperty(ValueTreeIdentifiers::XPosition)) - nodeX,
                                                               static_cast<int>(next.getProperty(ValueTreeIdentifiers::YPosition)) - nodeY);

            previousDurationWeight = step.durationWeight;
        }
        else if (danglingArrow.isValid()) {
            step.durationWeight = ArrowInfo::durationFromDelta(ArrowBindingOps::getArrowInfo(danglingArrow),
                                                               danglingArrow.getProperty(ValueTreeIdentifiers::ArrowTipX),
                                                               danglingArrow.getProperty(ValueTreeIdentifiers::ArrowTipY));
        }
        else {
            step.durationWeight = previousDurationWeight;
        }

        if (step.durationWeight > 0) {
            steps.push_back(step);
        }

        node = next;
    }

    return steps;
}

OrnamentStatus OrnamentLibraries::statusOf(const juce::ValueTree& ornament) const
{
    const juce::ValueTree published = ornament.getChildWithName(ValueTreeIdentifiers::PublishedFigure);
    juce::ValueTree       comparable {ValueTreeIdentifiers::OrnamentFigure};

    if (! published.isValid()) {
        return OrnamentStatus::Draft;
    }

    comparable.copyPropertiesAndChildrenFrom(published, nullptr);

    if (comparable.isEquivalentTo(ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure))) {
        return OrnamentStatus::Published;
    }

    return OrnamentStatus::ChangesPending;
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

    changedLibraryUids.insert(tree.getProperty(ValueTreeIdentifiers::LibraryUid).toString());

    startTimer(saveDelayMs);
}

void OrnamentLibraries::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    markLibraryChanged(child);

    ensureActiveOrnament();
}

void OrnamentLibraries::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index)
{
    const auto libraryFolder = libraryFolders.find(child.getProperty(ValueTreeIdentifiers::LibraryUid).toString());

    if (parent != libraries) {
        markLibraryChanged(parent);
    }

    if (parent == libraries && libraryFolder != libraryFolders.end()) {
        libraryFolder->second.folder.moveToTrash();

        libraryFolders.erase(libraryFolder);
    }

    ensureActiveOrnament();
}

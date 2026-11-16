#include "OrnamentLibraries.h"
#include "ArrowBindingOps.h"
#include "TypedProperties.h"
#include "../Util/ArrowInfo.h"
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

void OrnamentLibraries::setFigureNodePosition(juce::ValueTree ornament, int nodeId, juce::Point<int> position, juce::UndoManager* undoManager)
{
    const juce::ValueTree figure = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);

    juce::ValueTree node  = figure.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, nodeId);
    juce::ValueTree moved = node;

    FigureNoteProperties properties(node);

    const int oldX          = node.getProperty(ArsNovaUI::CanvasIdentifiers::XPosition);
    const int oldY          = node.getProperty(ArsNovaUI::CanvasIdentifiers::YPosition);
    const int oldOffset     = properties.offset;
    const int oldRow        = juce::roundToInt(oldY / ArrowInfo::pixelsPerGridSpace);
    const int newRow        = juce::roundToInt(position.y / ArrowInfo::pixelsPerGridSpace);
    const int rowShift      = juce::roundToInt((oldRow - newRow) * ArrowInfo::semitonesPerGridSpace);
    const int shiftedOffset = oldOffset + rowShift;
    const int clampedOffset = std::clamp(shiftedOffset, -maximumOffset, maximumOffset);

    const juce::Point<int> oldPosition { oldX, oldY };
    const juce::Point<int> delta = position - oldPosition;

    auto clearDurationOverride = [undoManager](juce::ValueTree arrowTree) {
        ArrowProperties arrow(arrowTree);

        if (arrow.durationOverride != ArrowInfo::noDurationOverride) {
            arrow.durationOverride.setValue(ArrowInfo::noDurationOverride, undoManager);
        }
    };

    if (! node.isValid()) {
        return;
    }

    properties.offset.setValue(clampedOffset, undoManager);

    for (const juce::ValueTree& candidate : figure) {
        clearDurationOverride(candidate.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds).getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, nodeId));
    }

    while (moved.isValid()) {
        const juce::ValueTree outgoing = moved.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds).getChild(0);

        const juce::Point<int> movedFrom { moved.getProperty(ArsNovaUI::CanvasIdentifiers::XPosition), moved.getProperty(ArsNovaUI::CanvasIdentifiers::YPosition) };
        const juce::Point<int> movedTo = movedFrom + delta;

        moved.setProperty(ArsNovaUI::CanvasIdentifiers::XPosition, movedTo.x, undoManager);
        moved.setProperty(ArsNovaUI::CanvasIdentifiers::YPosition, movedTo.y, undoManager);

        clearDurationOverride(outgoing);

        moved = figure.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, outgoing.getProperty(ArsNovaUI::CanvasIdentifiers::Id, -1));
    }
}

void OrnamentLibraries::removeFigureNode(juce::ValueTree ornament, int nodeId, juce::UndoManager* undoManager)
{
    juce::ValueTree       figure   = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);
    const juce::ValueTree node     = figure.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, nodeId);
    const juce::ValueTree outgoing = node.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds).getChild(0);
    juce::ValueTree       parent;
    juce::ValueTree       incoming;

    juce::CachedValue<int> hostId(figure, ValueTreeIdentifiers::OrnamentHostId, nullptr, noHostId);

    if (! node.isValid()) {
        return;
    }

    if (hostId == nodeId) {
        hostId.setValue(noHostId, undoManager);
    }

    for (const juce::ValueTree& candidate : figure) {
        const juce::ValueTree connection = candidate.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds).getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, nodeId);

        if (connection.isValid()) {
            parent   = candidate;
            incoming = connection;
        }
    }

    if (incoming.isValid() && outgoing.isValid()) {
        incoming.setProperty(ArsNovaUI::CanvasIdentifiers::Id, outgoing.getProperty(ArsNovaUI::CanvasIdentifiers::Id), undoManager);
    }

    if (incoming.isValid() && ! outgoing.isValid()) {
        const juce::ValueTree danglingArrows = node.getChildWithName(ArsNovaUI::CanvasIdentifiers::DanglingArrows);

        parent.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds).removeChild(incoming, undoManager);

        if (danglingArrows.isValid() && danglingArrows.getNumChildren() > 0) {
            parent.appendChild(danglingArrows.createCopy(), undoManager);
        }
    }

    figure.removeChild(node, undoManager);
}

void OrnamentLibraries::toggleHost(juce::ValueTree ornament, int nodeId, juce::UndoManager* undoManager)
{
    juce::ValueTree figure = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);

    juce::CachedValue<int> storedHostId(figure, ValueTreeIdentifiers::OrnamentHostId, nullptr, noHostId);

    int hostId = nodeId;

    if (! figure.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, nodeId).isValid()) {
        return;
    }

    if (storedHostId == nodeId) {
        hostId = noHostId;
    }

    storedHostId.setValue(hostId, undoManager);
}

void OrnamentLibraries::addDanglingArrow(juce::ValueTree ornament, int nodeId, juce::Point<int> tipOffset, juce::UndoManager* undoManager)
{
    const juce::ValueTree figure         = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);
    juce::ValueTree       node           = figure.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, nodeId);
    juce::ValueTree       danglingArrows = node.getChildWithName(ArsNovaUI::CanvasIdentifiers::DanglingArrows);
    juce::ValueTree       danglingArrow  {ArsNovaUI::CanvasIdentifiers::DanglingArrow};
    ArrowInfo::Settings   arrowInfo;

    if (! node.isValid()) {
        return;
    }

    if (! danglingArrows.isValid()) {
        danglingArrows = juce::ValueTree(ArsNovaUI::CanvasIdentifiers::DanglingArrows);

        node.appendChild(danglingArrows, undoManager);
    }

    if (danglingArrows.getNumChildren() > 0) {
        danglingArrows.getChild(0).setProperty(ArsNovaUI::CanvasIdentifiers::ArrowTipX, tipOffset.x, undoManager);
        danglingArrows.getChild(0).setProperty(ArsNovaUI::CanvasIdentifiers::ArrowTipY, tipOffset.y, undoManager);

        return;
    }

    arrowInfo.xBinding = ArrowInfo::Binding::DurationBind;
    arrowInfo.yBinding = ArrowInfo::Binding::NoBind;

    danglingArrow.setProperty(ArsNovaUI::CanvasIdentifiers::ArrowTipX, tipOffset.x, nullptr);
    danglingArrow.setProperty(ArsNovaUI::CanvasIdentifiers::ArrowTipY, tipOffset.y, nullptr);

    ArrowBindingOps::setArrowInfo(danglingArrow, arrowInfo, nullptr);

    danglingArrows.appendChild(danglingArrow, undoManager);
}

void OrnamentLibraries::removeDanglingArrow(juce::ValueTree ornament, juce::UndoManager* undoManager)
{
    const juce::ValueTree figure = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);

    for (juce::ValueTree node : figure) {
        const juce::ValueTree danglingArrows = node.getChildWithName(ArsNovaUI::CanvasIdentifiers::DanglingArrows);

        if (danglingArrows.isValid()) {
            node.removeChild(danglingArrows, undoManager);
        }
    }
}

void OrnamentLibraries::setLastNodeLength(juce::ValueTree ornament, juce::Point<int> tipOffset, juce::UndoManager* undoManager)
{
    const juce::ValueTree figure = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);

    for (const juce::ValueTree& node : figure) {
        juce::ValueTree danglingArrow = node.getChildWithName(ArsNovaUI::CanvasIdentifiers::DanglingArrows).getChild(0);

        if (danglingArrow.isValid()) {
            danglingArrow.setProperty(ArsNovaUI::CanvasIdentifiers::ArrowTipX, tipOffset.x, undoManager);
            danglingArrow.setProperty(ArsNovaUI::CanvasIdentifiers::ArrowTipY, tipOffset.y, undoManager);

            return;
        }
    }

    for (const juce::ValueTree& node : figure) {
        if (node.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds).getNumChildren() == 0) {
            addDanglingArrow(ornament, node.getProperty(ArsNovaUI::CanvasIdentifiers::Id), tipOffset, undoManager);

            return;
        }
    }
}

void OrnamentLibraries::addSymbolStroke(juce::ValueTree ornament, const juce::Path& stroke, float width, juce::UndoManager* undoManager)
{
    OrnamentProperties properties(ornament);

    juce::String symbol = properties.symbol;

    if (symbol.isNotEmpty()) {
        symbol << "|";
    }

    symbol << juce::String(width) << " " << stroke.toString();

    properties.symbol.setValue(symbol, undoManager);
}

void OrnamentLibraries::eraseSymbol(juce::ValueTree ornament, juce::Point<float> centre, float radius, juce::UndoManager* undoManager)
{
    juce::StringArray erasedStrokes;

    OrnamentProperties properties(ornament);

    const juce::String symbol = properties.symbol;

    for (const ArsNovaUI::SymbolStroke& stroke : readSymbol(symbol)) {
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
                const float halfChord = std::sqrt(radius * radius - gap * gap);

                enter = (alongCentre - halfChord) / length;
                leave = (alongCentre + halfChord) / length;
            }

            if (leave <= 0.0f || enter >= 1.0f) {
                enter = 1.0f;
                leave = 1.0f;
            }

            if (enter > 0.0f && ! penDown) {
                erased.startNewSubPath(current);
            }

            if (enter > 0.0f) {
                const juce::Point<float> entryPoint = current + along * enter;

                erased.lineTo(entryPoint);
            }

            if (leave < 1.0f) {
                const juce::Point<float> exitPoint = current + along * juce::jmax(leave, 0.0f);

                erased.startNewSubPath(exitPoint);
                erased.lineTo(end);
            }

            penDown = enter >= 1.0f || leave < 1.0f;
            current = end;
        }

        if (! erased.isEmpty()) {
            erasedStrokes.add(juce::String(stroke.width) + " " + erased.toString());
        }
    }

    const juce::String erasedSymbol = erasedStrokes.joinIntoString("|");

    properties.symbol.setValue(erasedSymbol, undoManager);
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

    juce::CachedValue<int> hostId(figure, ValueTreeIdentifiers::OrnamentHostId, nullptr, noHostId);

    properties.ornamentUid     .setValue(ornamentUid,                           nullptr);
    properties.ornamentName    .setValue(ornamentName,                          nullptr);
    properties.symbol          .setValue({},                                    nullptr);
    properties.nextFigureNodeId.setValue(OrnamentProperties::firstFigureNodeId, nullptr);
    hostId                     .setValue(noHostId,                              nullptr);

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

juce::ValueTree OrnamentLibraries::addFigureNode(juce::ValueTree ornament, int parentNodeId, juce::Point<int> position, juce::UndoManager* undoManager)
{
    OrnamentProperties properties(ornament);

    juce::ValueTree figure = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);
    juce::ValueTree parent = figure.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, parentNodeId);
    juce::ValueTree node   {ValueTreeIdentifiers::OrnamentNoteData};

    const int nodeId           = properties.nextFigureNodeId;
    const int nextFigureNodeId = nodeId + 1;

    ArrowInfo::Settings arrowInfo;

    FigureNoteProperties noteProperties(node);

    if (! figure.isValid() || (parentNodeId == -1 && figure.getNumChildren() > 0)) {
        return {};
    }

    if (parentNodeId != -1 && (! parent.isValid() || parent.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds).getNumChildren() > 0)) {
        return {};
    }

    arrowInfo.xBinding = ArrowInfo::Binding::DurationBind;
    arrowInfo.yBinding = ArrowInfo::Binding::NoBind;

    node.setProperty(ArsNovaUI::CanvasIdentifiers::Id,               nodeId,                 nullptr);
    node.setProperty(ArsNovaUI::CanvasIdentifiers::XPosition,        position.x,             nullptr);
    node.setProperty(ArsNovaUI::CanvasIdentifiers::YPosition,        position.y,             nullptr);

    noteProperties.offset      .setValue(0,                      nullptr);
    noteProperties.velocity    .setValue(defaultVelocityPercent, nullptr);
    properties.nextFigureNodeId.setValue(nextFigureNodeId,       undoManager);

    node.appendChild(juce::ValueTree(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds), nullptr);

    if (parent.isValid()) {
        juce::ValueTree connection {ValueTreeIdentifiers::NodeId};

        connection.setProperty(ArsNovaUI::CanvasIdentifiers::Id, nodeId, nullptr);

        ArrowBindingOps::setArrowInfo(connection, arrowInfo, nullptr);

        parent.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds).appendChild(connection, undoManager);
        parent.removeChild(parent.getChildWithName(ArsNovaUI::CanvasIdentifiers::DanglingArrows), undoManager);
    }

    figure.appendChild(node, undoManager);

    return node;
}

bool OrnamentLibraries::isPublishable(const juce::ValueTree& ornament) const
{
    juce::ValueTree figure = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);

    const juce::CachedValue<int> hostId(figure, ValueTreeIdentifiers::OrnamentHostId, nullptr, noHostId);

    const int storedHostId = hostId;

    int totalWeight = 0;

    for (const OrnamentStep& step : figureSteps(figure)) {
        totalWeight += step.durationWeight;
    }

    return figure.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, storedHostId).isValid()
           && figure.getNumChildren() > 1 && totalWeight > 0;
}

std::vector<OrnamentStep> OrnamentLibraries::figureSteps(const juce::ValueTree& figure) const
{
    juce::ValueTree figureTree = figure;
    juce::ValueTree node;

    const juce::CachedValue<int> storedHostId(figureTree, ValueTreeIdentifiers::OrnamentHostId, nullptr, noHostId);

    const int hostId = storedHostId;

    std::vector<OrnamentStep> steps;

    std::set<int> pointedAt;

    int previousDurationWeight = 0;

    for (const juce::ValueTree& candidate : figure) {
        for (const juce::ValueTree& connection : candidate.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds)) {
            const int targetId = connection.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

            pointedAt.insert(targetId);
        }
    }

    for (const juce::ValueTree& candidate : figure) {
        const int candidateId = candidate.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

        if (pointedAt.count(candidateId) == 0) {
            node = candidate;

            break;
        }
    }

    for (int visited = 0; node.isValid() && visited < figure.getNumChildren(); ++visited) {
        const juce::ValueTree connection    = node.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds).getChild(0);
        const juce::ValueTree danglingArrow = node.getChildWithName(ArsNovaUI::CanvasIdentifiers::DanglingArrows).getChild(0);
        const int             nodeX         = node.getProperty(ArsNovaUI::CanvasIdentifiers::XPosition);
        const int             nodeY         = node.getProperty(ArsNovaUI::CanvasIdentifiers::YPosition);
        juce::ValueTree       next;

        const FigureNoteProperties note(node);

        OrnamentStep step { OrnamentNodeKind::Note, note.offset, note.velocity, 0 };

        if (connection.isValid()) {
            next = figure.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, connection.getProperty(ArsNovaUI::CanvasIdentifiers::Id));
        }

        if (static_cast<int>(node.getProperty(ArsNovaUI::CanvasIdentifiers::Id)) == hostId) {
            step.kind = OrnamentNodeKind::Host;
        }

        if (next.isValid()) {
            const int nextX  = next.getProperty(ArsNovaUI::CanvasIdentifiers::XPosition);
            const int nextY  = next.getProperty(ArsNovaUI::CanvasIdentifiers::YPosition);
            const int deltaX = nextX - nodeX;
            const int deltaY = nextY - nodeY;

            const ArrowInfo::Settings connectionInfo = ArrowBindingOps::getArrowInfo(connection);

            step.durationWeight = ArrowInfo::Settings::durationFromDelta(connectionInfo, deltaX, deltaY);

            previousDurationWeight = step.durationWeight;
        }
        else if (danglingArrow.isValid()) {
            step.durationWeight = ArrowInfo::Settings::durationFromDelta(ArrowBindingOps::getArrowInfo(danglingArrow),
                                                               danglingArrow.getProperty(ArsNovaUI::CanvasIdentifiers::ArrowTipX),
                                                               danglingArrow.getProperty(ArsNovaUI::CanvasIdentifiers::ArrowTipY));
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

std::vector<ArsNovaUI::SymbolStroke> OrnamentLibraries::readSymbol(const juce::String& symbol)
{
    std::vector<ArsNovaUI::SymbolStroke> strokes;

    for (const juce::String& entry : juce::StringArray::fromTokens(symbol, "|", "")) {
        const juce::String      widthText = entry.upToFirstOccurrenceOf(" ", false, false);
        ArsNovaUI::SymbolStroke stroke { juce::Path(), defaultStrokeWidth };

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

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Processor/Document/Ornaments/OrnamentLibraries.h"
#include "Processor/Document/Ornaments/OrnamentFigure.h"
#include "Processor/Document/Ornaments/OrnamentSymbol.h"
#include "Processor/Document/ValueTreeIdentifiers.h"
#include <ArsNovaUI/Canvas/CanvasIdentifiers.h>

#include <vector>

struct TemporaryFolder
{
    juce::File folder = juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("OrnamentTests", "", false);

    ~TemporaryFolder()
    {
        folder.deleteRecursively();
    }
};

static juce::ValueTree buildLowerGrace(OrnamentLibraries& libraries)
{
    juce::ValueTree ornament = libraries.addOrnament({}, nullptr);
    juce::ValueTree note     = OrnamentFigure::addNode(ornament, -1, { 0, 0 }, nullptr);
    const int       noteId   = note.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    note.setProperty(ValueTreeIdentifiers::OrnamentOffset,   -1, nullptr);
    note.setProperty(ValueTreeIdentifiers::OrnamentVelocity, 70, nullptr);

    const juce::ValueTree host = OrnamentFigure::addNode(ornament, noteId, { 100, 0 }, nullptr);

    REQUIRE(host.isValid());

    OrnamentFigure::toggleHost(ornament, host.getProperty(ArsNovaUI::CanvasIdentifiers::Id), nullptr);
    OrnamentFigure::setLastNodeLength(ornament, { 200, 0 }, nullptr);

    return ornament;
}

TEST_CASE("an ornament's figure plays its chain in order, each step weighted by its arrow's length", "[ornament]")
{
    TemporaryFolder   temporary;
    OrnamentLibraries libraries;

    libraries.load(temporary.folder);

    const juce::ValueTree           ornament = buildLowerGrace(libraries);
    const std::vector<OrnamentStep> steps    = OrnamentFigure::steps(ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure));

    REQUIRE(steps.size() == 2);

    CHECK(steps[0].kind            == OrnamentNodeKind::Note);
    CHECK(steps[0].offset          == -1);
    CHECK(steps[0].velocityPercent == 70);
    CHECK(steps[0].durationWeight  == 500);

    CHECK(steps[1].kind            == OrnamentNodeKind::Host);
    CHECK(steps[1].offset          == 0);
    CHECK(steps[1].velocityPercent == 100);
    CHECK(steps[1].durationWeight  == 1000);
}

TEST_CASE("a node added to the chain does not get a dangling arrow and assumes the duration of the node before it", "[ornament]")
{
    TemporaryFolder   temporary;
    OrnamentLibraries libraries;

    libraries.load(temporary.folder);

    juce::ValueTree       ornament = libraries.addOrnament({}, nullptr);
    const juce::ValueTree first    = OrnamentFigure::addNode(ornament, -1, { 0, 0 }, nullptr);
    const int             firstId  = first.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    CHECK(! first.getChildWithName(ArsNovaUI::CanvasIdentifiers::DanglingArrows).isValid());

    const juce::ValueTree second   = OrnamentFigure::addNode(ornament, firstId, { 150, 50 }, nullptr);
    const int             secondId = second.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    CHECK(! second.getChildWithName(ArsNovaUI::CanvasIdentifiers::DanglingArrows).isValid());

    const std::vector<OrnamentStep> initialSteps = OrnamentFigure::steps(ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure));

    REQUIRE(initialSteps.size() == 2);

    CHECK(initialSteps[0].durationWeight == 750);
    CHECK(initialSteps[1].durationWeight == 750);

    OrnamentFigure::addDanglingArrow(ornament, secondId, { 200, 0 }, nullptr);

    const std::vector<OrnamentStep> stepsWithDangling = OrnamentFigure::steps(ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure));

    REQUIRE(stepsWithDangling.size() == 2);

    CHECK(stepsWithDangling[0].durationWeight == 750);
    CHECK(stepsWithDangling[1].durationWeight == 1000);

    OrnamentFigure::removeDanglingArrow(ornament, nullptr);

    const std::vector<OrnamentStep> stepsAfterRemoval = OrnamentFigure::steps(ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure));

    REQUIRE(stepsAfterRemoval.size() == 2);

    CHECK(stepsAfterRemoval[0].durationWeight == 750);
    CHECK(stepsAfterRemoval[1].durationWeight == 750);
}

TEST_CASE("a figure stays one chain", "[ornament]")
{
    TemporaryFolder   temporary;
    OrnamentLibraries libraries;

    libraries.load(temporary.folder);

    juce::ValueTree       ornament = buildLowerGrace(libraries);
    const juce::ValueTree figure   = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);
    const int             noteId   = figure.getChild(0).getProperty(ArsNovaUI::CanvasIdentifiers::Id);
    const int             hostId   = figure.getChild(1).getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    CHECK(! OrnamentFigure::addNode(ornament, noteId, { 50, 0 }, nullptr).isValid());
    CHECK(! OrnamentFigure::addNode(ornament, -1, { 0, 100 }, nullptr).isValid());
    CHECK(OrnamentFigure::addNode(ornament, hostId, { 300, 0 }, nullptr).isValid());
    CHECK(figure.getNumChildren() == 3);
}

TEST_CASE("the host is a mark one node carries: picking another node moves it, picking the host again clears it", "[ornament]")
{
    TemporaryFolder   temporary;
    OrnamentLibraries libraries;
    juce::UndoManager undoManager;

    libraries.load(temporary.folder);

    juce::ValueTree       ornament = buildLowerGrace(libraries);
    juce::ValueTree       figure   = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);
    const int             noteId   = figure.getChild(0).getProperty(ArsNovaUI::CanvasIdentifiers::Id);
    const int             hostId   = figure.getChild(1).getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    OrnamentFigure::toggleHost(ornament, noteId, nullptr);

    const std::vector<OrnamentStep> moved = OrnamentFigure::steps(figure);

    REQUIRE(moved.size() == 2);

    CHECK(moved[0].kind == OrnamentNodeKind::Host);
    CHECK(moved[1].kind == OrnamentNodeKind::Note);
    CHECK(OrnamentFigure::isPublishable(ornament));

    OrnamentFigure::toggleHost(ornament, noteId, nullptr);

    CHECK(static_cast<int>(figure.getProperty(ValueTreeIdentifiers::OrnamentHostId)) == OrnamentFigure::noHostId);
    CHECK(! OrnamentFigure::isPublishable(ornament));

    OrnamentFigure::toggleHost(ornament, hostId, nullptr);
    OrnamentFigure::removeNode(ornament, hostId, &undoManager);

    CHECK(static_cast<int>(figure.getProperty(ValueTreeIdentifiers::OrnamentHostId)) == OrnamentFigure::noHostId);

    undoManager.undo();

    CHECK(static_cast<int>(figure.getProperty(ValueTreeIdentifiers::OrnamentHostId)) == hostId);
    CHECK(OrnamentFigure::isPublishable(ornament));
}

TEST_CASE("removing the middle of three nodes joins its neighbours", "[ornament]")
{
    TemporaryFolder   temporary;
    OrnamentLibraries libraries;

    libraries.load(temporary.folder);

    juce::ValueTree       ornament = libraries.addOrnament({}, nullptr);
    const juce::ValueTree first    = OrnamentFigure::addNode(ornament, -1, { 0, 0 }, nullptr);
    const juce::ValueTree middle   = OrnamentFigure::addNode(ornament, first.getProperty(ArsNovaUI::CanvasIdentifiers::Id), { 100, 0 }, nullptr);
    const juce::ValueTree last     = OrnamentFigure::addNode(ornament, middle.getProperty(ArsNovaUI::CanvasIdentifiers::Id), { 300, 0 }, nullptr);

    OrnamentFigure::toggleHost(ornament, last.getProperty(ArsNovaUI::CanvasIdentifiers::Id), nullptr);
    OrnamentFigure::removeNode(ornament, middle.getProperty(ArsNovaUI::CanvasIdentifiers::Id), nullptr);

    const std::vector<OrnamentStep> steps = OrnamentFigure::steps(ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure));

    REQUIRE(steps.size() == 2);

    CHECK(first.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds).getChild(0).getProperty(ArsNovaUI::CanvasIdentifiers::Id) == last.getProperty(ArsNovaUI::CanvasIdentifiers::Id));
    CHECK(steps[0].durationWeight == 1500);
    CHECK(steps[1].kind           == OrnamentNodeKind::Host);
    CHECK(steps[1].durationWeight == 1500);
}

TEST_CASE("removing the last node hands its length to the node before it", "[ornament]")
{
    TemporaryFolder   temporary;
    OrnamentLibraries libraries;

    libraries.load(temporary.folder);

    juce::ValueTree       ornament = buildLowerGrace(libraries);
    const juce::ValueTree figure   = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);
    const juce::ValueTree note     = figure.getChild(0);

    OrnamentFigure::removeNode(ornament, figure.getChild(1).getProperty(ArsNovaUI::CanvasIdentifiers::Id), nullptr);

    const std::vector<OrnamentStep> steps = OrnamentFigure::steps(figure);

    REQUIRE(steps.size() == 1);

    CHECK(note.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds).getNumChildren() == 0);
    CHECK(steps[0].durationWeight == 1000);
}

TEST_CASE("moving the host sideways retimes the arrow that reaches it", "[ornament]")
{
    TemporaryFolder   temporary;
    OrnamentLibraries libraries;

    libraries.load(temporary.folder);

    juce::ValueTree       ornament = buildLowerGrace(libraries);
    const juce::ValueTree figure   = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);

    OrnamentFigure::setNodePosition(ornament, figure.getChild(1).getProperty(ArsNovaUI::CanvasIdentifiers::Id), { 150, 0 }, nullptr);

    const std::vector<OrnamentStep> steps = OrnamentFigure::steps(figure);

    REQUIRE(steps.size() == 2);

    CHECK(steps[0].durationWeight == 750);
    CHECK(steps[1].durationWeight == 1000);
}

TEST_CASE("moving a node to another row shifts only its own offset", "[ornament]")
{
    TemporaryFolder   temporary;
    OrnamentLibraries libraries;

    libraries.load(temporary.folder);

    juce::ValueTree       ornament = buildLowerGrace(libraries);
    const juce::ValueTree figure   = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);
    const juce::ValueTree note     = figure.getChild(0);
    const juce::ValueTree host     = figure.getChild(1);

    OrnamentFigure::setNodePosition(ornament, note.getProperty(ArsNovaUI::CanvasIdentifiers::Id), { 0, -10 }, nullptr);

    CHECK(note.getProperty(ValueTreeIdentifiers::OrnamentOffset) == juce::var(-1));

    OrnamentFigure::setNodePosition(ornament, note.getProperty(ArsNovaUI::CanvasIdentifiers::Id), { 0, -100 }, nullptr);

    CHECK(note.getProperty(ValueTreeIdentifiers::OrnamentOffset) == juce::var(1));
    CHECK(host.getProperty(ValueTreeIdentifiers::OrnamentOffset) == juce::var(0));
}

TEST_CASE("moving a node carries the nodes after it, so the arrows after it keep their lengths", "[ornament]")
{
    TemporaryFolder   temporary;
    OrnamentLibraries libraries;

    libraries.load(temporary.folder);

    juce::ValueTree       ornament = buildLowerGrace(libraries);
    const juce::ValueTree figure   = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);
    const juce::ValueTree host     = figure.getChild(1);

    OrnamentFigure::setNodePosition(ornament, figure.getChild(0).getProperty(ArsNovaUI::CanvasIdentifiers::Id), { 50, -100 }, nullptr);

    const std::vector<OrnamentStep> steps = OrnamentFigure::steps(figure);

    REQUIRE(steps.size() == 2);

    CHECK(host.getProperty(ArsNovaUI::CanvasIdentifiers::XPosition)      == juce::var(150));
    CHECK(host.getProperty(ArsNovaUI::CanvasIdentifiers::YPosition)      == juce::var(-100));
    CHECK(host.getProperty(ValueTreeIdentifiers::OrnamentOffset) == juce::var(0));
    CHECK(steps[0].durationWeight == 500);
    CHECK(steps[1].durationWeight == 1000);
}

TEST_CASE("moving a node clears a typed duration on the arrow into it", "[ornament]")
{
    TemporaryFolder   temporary;
    OrnamentLibraries libraries;

    libraries.load(temporary.folder);

    juce::ValueTree       ornament   = buildLowerGrace(libraries);
    const juce::ValueTree figure     = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);
    juce::ValueTree       connection = figure.getChild(0).getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds).getChild(0);

    connection.setProperty(ValueTreeIdentifiers::ArrowDuration, 300, nullptr);

    REQUIRE(OrnamentFigure::steps(figure)[0].durationWeight == 300);

    OrnamentFigure::setNodePosition(ornament, figure.getChild(1).getProperty(ArsNovaUI::CanvasIdentifiers::Id), { 150, 0 }, nullptr);

    CHECK(OrnamentFigure::steps(figure)[0].durationWeight == 750);
}

TEST_CASE("publishing needs a connected host, and an unpublishable edit keeps the last published figure", "[ornament]")
{
    TemporaryFolder   temporary;
    OrnamentLibraries libraries;

    libraries.load(temporary.folder);

    juce::ValueTree hostOnly    = libraries.addOrnament({}, nullptr);
    juce::ValueTree withoutHost = libraries.addOrnament({}, nullptr);

    CHECK(libraries.libraries.getNumChildren() == 1);

    OrnamentFigure::addNode(hostOnly,    -1, { 0, 0 }, nullptr);
    OrnamentFigure::addNode(withoutHost, -1, { 0, 0 }, nullptr);

    OrnamentFigure::toggleHost(hostOnly, hostOnly.getChildWithName(ValueTreeIdentifiers::OrnamentFigure).getChild(0).getProperty(ArsNovaUI::CanvasIdentifiers::Id), nullptr);

    CHECK(! OrnamentFigure::isPublishable(hostOnly));
    CHECK(! OrnamentFigure::isPublishable(withoutHost));

    juce::ValueTree       ornament = buildLowerGrace(libraries);
    const juce::ValueTree figure   = ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure);
    const int             noteId   = figure.getChild(0).getProperty(ArsNovaUI::CanvasIdentifiers::Id);
    const int             hostId   = figure.getChild(1).getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    REQUIRE(OrnamentFigure::isPublishable(ornament));
    CHECK(OrnamentFigure::statusOf(ornament) == OrnamentStatus::Draft);

    OrnamentFigure::publish(ornament, nullptr);

    CHECK(OrnamentFigure::statusOf(ornament) == OrnamentStatus::Published);

    OrnamentFigure::removeNode(ornament, hostId, nullptr);
    OrnamentFigure::publish(ornament, nullptr);

    CHECK(OrnamentFigure::statusOf(ornament) == OrnamentStatus::ChangesPending);
    CHECK(! OrnamentFigure::isPublishable(ornament));
    CHECK(OrnamentFigure::steps(ornament.getChildWithName(ValueTreeIdentifiers::PublishedFigure)).size() == 2);

    const juce::ValueTree newHost = OrnamentFigure::addNode(ornament, noteId, { 100, 0 }, nullptr);

    OrnamentFigure::toggleHost(ornament, newHost.getProperty(ArsNovaUI::CanvasIdentifiers::Id), nullptr);
    OrnamentFigure::publish(ornament, nullptr);

    CHECK(OrnamentFigure::statusOf(ornament) == OrnamentStatus::Published);
}

TEST_CASE("duplicating an ornament gives it a new uid and an equal figure", "[ornament]")
{
    TemporaryFolder   temporary;
    OrnamentLibraries libraries;

    libraries.load(temporary.folder);

    const juce::ValueTree ornament  = buildLowerGrace(libraries);
    const juce::String    uid       = ornament.getProperty(ValueTreeIdentifiers::OrnamentUid);
    const juce::ValueTree duplicate = libraries.duplicateOrnament(uid, nullptr);

    REQUIRE(duplicate.isValid());

    CHECK(duplicate.getProperty(ValueTreeIdentifiers::OrnamentUid).toString() != uid);
    CHECK(duplicate.getParent() == ornament.getParent());
    CHECK(duplicate.getChildWithName(ValueTreeIdentifiers::OrnamentFigure).isEquivalentTo(ornament.getChildWithName(ValueTreeIdentifiers::OrnamentFigure)));
}

TEST_CASE("a symbol's strokes survive saving and reloading its library", "[ornament]")
{
    TemporaryFolder temporary;
    juce::Path      firstStroke;
    juce::Path      secondStroke;
    juce::String    uid;
    juce::String    savedSymbol;

    firstStroke.startNewSubPath(0.1f, 0.5f);
    firstStroke.lineTo(0.9f, 0.5f);

    secondStroke.startNewSubPath(0.5f, 0.1f);
    secondStroke.lineTo(0.5f, 0.9f);

    {
        OrnamentLibraries libraries;

        libraries.load(temporary.folder);

        juce::ValueTree ornament = libraries.addOrnament({}, nullptr);

        OrnamentSymbol::addStroke(ornament, firstStroke, 0.05f, nullptr);
        OrnamentSymbol::addStroke(ornament, secondStroke, 0.1f, nullptr);

        libraries.timerCallback();

        uid         = ornament.getProperty(ValueTreeIdentifiers::OrnamentUid).toString();
        savedSymbol = ornament.getProperty(ValueTreeIdentifiers::OrnamentSymbol).toString();
    }

    OrnamentLibraries reloaded;

    reloaded.load(temporary.folder);

    const std::vector<ArsNovaUI::SymbolStroke> strokes = OrnamentSymbol::read(reloaded.findOrnament(uid).getProperty(ValueTreeIdentifiers::OrnamentSymbol).toString());

    CHECK(reloaded.findOrnament(uid).getProperty(ValueTreeIdentifiers::OrnamentSymbol).toString() == savedSymbol);
    REQUIRE(strokes.size() == 2);
    CHECK(strokes[0].width == Catch::Approx(0.05f));
    CHECK(strokes[1].width == Catch::Approx(0.1f));
    CHECK((strokes[0].path.getBounds() * 10.0f).toNearestInt() == juce::Rectangle<int>(1, 5, 8, 0));
    CHECK((strokes[1].path.getBounds() * 10.0f).toNearestInt() == juce::Rectangle<int>(5, 1, 0, 8));
}

TEST_CASE("a symbol saved before line widths reads at the default width", "[ornament]")
{
    const std::vector<ArsNovaUI::SymbolStroke> strokes = OrnamentSymbol::read("m 0.1 0.5 l 0.9 0.5");

    REQUIRE(strokes.size() == 1);
    CHECK(strokes[0].width == Catch::Approx(OrnamentSymbol::defaultStrokeWidth));
    CHECK((strokes[0].path.getBounds() * 10.0f).toNearestInt() == juce::Rectangle<int>(1, 5, 8, 0));
}

TEST_CASE("the eraser removes every part of a line's width under its circle, and only that", "[ornament]")
{
    OrnamentLibraries libraries;
    TemporaryFolder   temporary;
    juce::Path        line;

    line.startNewSubPath(0.1f, 0.5f);
    line.lineTo(0.9f, 0.5f);

    libraries.load(temporary.folder);

    juce::ValueTree ornament = libraries.addOrnament({}, nullptr);

    OrnamentSymbol::addStroke(ornament, line, 0.1f, nullptr);
    OrnamentSymbol::erase(ornament, { 0.5f, 0.58f }, 0.05f, nullptr);

    const std::vector<ArsNovaUI::SymbolStroke> strokes = OrnamentSymbol::read(ornament.getProperty(ValueTreeIdentifiers::OrnamentSymbol).toString());
    const juce::Rectangle<float>               kept    = strokes.at(0).path.getBounds();

    REQUIRE(strokes.size() == 1);
    CHECK(kept.getX() == Catch::Approx(0.1f).margin(0.001f));
    CHECK(kept.getRight() == Catch::Approx(0.9f).margin(0.001f));
    CHECK(strokes[0].path.getLength() == Catch::Approx(0.72f).margin(0.002f));
}

TEST_CASE("a rescan picks up libraries another instance added, changed or removed", "[ornament]")
{
    TemporaryFolder   temporary;
    OrnamentLibraries first;
    OrnamentLibraries second;
    juce::UndoManager undoManager;

    first.load(temporary.folder);
    second.load(temporary.folder);

    const juce::ValueTree added = second.addLibrary("Shared", nullptr);
    const juce::String    uid   = added.getProperty(ValueTreeIdentifiers::LibraryUid).toString();

    second.addOrnament(added, nullptr);
    second.timerCallback();

    first.rescan(undoManager);

    REQUIRE(first.libraries.getNumChildren() == 1);
    CHECK(first.libraries.getChild(0).isEquivalentTo(added));

    second.libraries.getChild(0).setProperty(ValueTreeIdentifiers::LibraryName, "Renamed", nullptr);
    second.timerCallback();
    second.libraryFolders[uid].folder.setLastModificationTime(juce::Time::getCurrentTime() + juce::RelativeTime::seconds(1.0));

    first.rescan(undoManager);

    REQUIRE(first.libraries.getNumChildren() == 1);
    CHECK(first.libraries.getChild(0).getProperty(ValueTreeIdentifiers::LibraryName).toString() == "Renamed");

    REQUIRE(second.libraryFolders[uid].folder.deleteRecursively());

    first.rescan(undoManager);

    CHECK(first.libraries.getNumChildren() == 0);
    CHECK(first.libraries.getProperty(ValueTreeIdentifiers::ActiveOrnamentUid).toString().isEmpty());
}

TEST_CASE("every library in the folder loads, a loaded library is never added twice, and a saved library reloads equal", "[ornament]")
{
    TemporaryFolder temporary;
    juce::ValueTree savedFirst;
    juce::ValueTree savedSecond;
    juce::String    activeUid;

    {
        OrnamentLibraries libraries;

        libraries.load(temporary.folder);

        buildLowerGrace(libraries);

        const juce::ValueTree second = libraries.addLibrary("Second", nullptr);

        libraries.addOrnament(second, nullptr);

        libraries.timerCallback();

        savedFirst  = libraries.libraries.getChild(0).createCopy();
        savedSecond = libraries.libraries.getChild(1).createCopy();
        activeUid   = libraries.libraries.getProperty(ValueTreeIdentifiers::ActiveOrnamentUid);
    }

    OrnamentLibraries reloaded;

    reloaded.load(temporary.folder);

    REQUIRE(reloaded.libraries.getNumChildren() == 2);

    const juce::ValueTree firstReloaded  = reloaded.libraries.getChildWithProperty(ValueTreeIdentifiers::LibraryUid, savedFirst.getProperty(ValueTreeIdentifiers::LibraryUid));
    const juce::ValueTree secondReloaded = reloaded.libraries.getChildWithProperty(ValueTreeIdentifiers::LibraryUid, savedSecond.getProperty(ValueTreeIdentifiers::LibraryUid));

    CHECK(firstReloaded.isEquivalentTo(savedFirst));
    CHECK(secondReloaded.isEquivalentTo(savedSecond));
    CHECK(reloaded.libraries.getProperty(ValueTreeIdentifiers::ActiveOrnamentUid).toString() == activeUid);
    CHECK(activeUid.isNotEmpty());

    const juce::File copied = temporary.folder.getSiblingFile("SharedOrnaments");

    REQUIRE(temporary.folder.findChildFiles(juce::File::findDirectories, false).getFirst().copyDirectoryTo(copied));

    CHECK(! reloaded.importLibrary(copied).isValid());
    CHECK(reloaded.libraries.getNumChildren() == 2);

    copied.deleteRecursively();
}

TEST_CASE("a library is a folder holding one file per ornament, and renaming or deleting an ornament renames or deletes its file", "[ornament]")
{
    TemporaryFolder   temporary;
    OrnamentLibraries libraries;

    libraries.load(temporary.folder);

    const juce::ValueTree library       = libraries.addLibrary("Graces", nullptr);
    juce::ValueTree       lower         = libraries.addOrnament(library, nullptr);
    juce::ValueTree       upper         = libraries.addOrnament(library, nullptr);
    const juce::File      libraryFolder = temporary.folder.getChildFile("Graces");

    lower.setProperty(ValueTreeIdentifiers::OrnamentName, "Lower", nullptr);
    upper.setProperty(ValueTreeIdentifiers::OrnamentName, "Upper", nullptr);

    libraries.timerCallback();

    CHECK(libraryFolder.getChildFile(OrnamentLibraries::libraryInfoFileName).existsAsFile());
    CHECK(libraryFolder.getChildFile(juce::String("Lower") + OrnamentLibraries::ornamentExtension).existsAsFile());
    CHECK(libraryFolder.getChildFile(juce::String("Upper") + OrnamentLibraries::ornamentExtension).existsAsFile());

    upper.setProperty(ValueTreeIdentifiers::OrnamentName, "Mordent", nullptr);

    libraries.removeOrnament(lower.getProperty(ValueTreeIdentifiers::OrnamentUid).toString(), nullptr);
    libraries.timerCallback();

    CHECK(! libraryFolder.getChildFile(juce::String("Lower") + OrnamentLibraries::ornamentExtension).exists());
    CHECK(! libraryFolder.getChildFile(juce::String("Upper") + OrnamentLibraries::ornamentExtension).exists());
    CHECK(libraryFolder.getChildFile(juce::String("Mordent") + OrnamentLibraries::ornamentExtension).existsAsFile());
}

TEST_CASE("a library saved as one .ornaments file converts to a folder once, and is not converted again", "[ornament]")
{
    TemporaryFolder temporary;
    juce::ValueTree legacyLibrary {ValueTreeIdentifiers::OrnamentLibrary};
    juce::ValueTree trill         {ValueTreeIdentifiers::OrnamentData};
    const juce::File libraryRoot  = temporary.folder.getChildFile("Libraries");
    const juce::File legacyFolder = temporary.folder.getChildFile("Legacy");

    legacyLibrary.setProperty(ValueTreeIdentifiers::LibraryUid,  juce::Uuid().toString(), nullptr);
    legacyLibrary.setProperty(ValueTreeIdentifiers::LibraryName, "Old",                   nullptr);
    trill.setProperty(ValueTreeIdentifiers::OrnamentUid,  juce::Uuid().toString(), nullptr);
    trill.setProperty(ValueTreeIdentifiers::OrnamentName, "Trill",                 nullptr);

    trill.appendChild(juce::ValueTree(ValueTreeIdentifiers::OrnamentFigure), nullptr);
    legacyLibrary.appendChild(trill, nullptr);

    REQUIRE(legacyFolder.createDirectory().wasOk());
    REQUIRE(legacyLibrary.createXml()->writeTo(legacyFolder.getChildFile("Old.ornaments")));

    {
        OrnamentLibraries libraries;

        libraries.load(libraryRoot);
        libraries.importLegacyLibraries(legacyFolder);
        libraries.timerCallback();

        REQUIRE(libraries.libraries.getNumChildren() == 1);
        CHECK(libraries.libraries.getChild(0).isEquivalentTo(legacyLibrary));
        CHECK(libraryRoot.getChildFile("Old").getChildFile(juce::String("Trill") + OrnamentLibraries::ornamentExtension).existsAsFile());
    }

    REQUIRE(libraryRoot.getChildFile("Old").deleteRecursively());

    OrnamentLibraries reloaded;

    reloaded.load(libraryRoot);
    reloaded.importLegacyLibraries(legacyFolder);

    CHECK(reloaded.libraries.getNumChildren() == 0);
    CHECK(legacyFolder.getChildFile("Old.ornaments").existsAsFile());
}

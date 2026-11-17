#include "OrnamentLibrarySection.h"
#include <ArsNovaUI/Theme/IconLibrary.h>
#include "Processor/Document/TypedProperties.h"
#include <ArsNovaUI/Menus/ContextMenu.h>
#include <ArsNovaUI/Menus/Dialog.h>
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "Processor/Document/Ornaments/OrnamentLibraries.h"

#include <algorithm>

OrnamentLibrarySection::OrnamentLibrarySection(OrnamentLibraries& ornamentLibraries, juce::ValueTree library, juce::UndoManager& undoManager)
    : library          (library),
      ornamentLibraries(ornamentLibraries),
      undoManager      (undoManager),
      nameEditor       (undoManager)
{
    nameEditor.autoFitText = true;
    nameEditor.fontStyle   = ArsNovaUI::Theme::FontStyle::SemiBold;

    nameEditor.setFormat(std::make_unique<ArsNovaUI::TextFormat>(ArsNovaUI::TextFormat::labelTextLength, ArsNovaUI::TextFormat::labelCharacters));
    nameEditor.setJustification(juce::Justification::centredLeft);
    nameEditor.bindEditor(library, ValueTreeIdentifiers::LibraryName);
    nameEditor.setTooltip("Library name");

    foldButton.icon    = &ArsNovaUI::IconLibrary::get().chevronUp;
    foldButton.onClick = [this]() { toggleFold(); };
    foldButton.setTooltip("Fold library");

    addCard.onClick = [this]() { addOrnament(); };

    addCard.painter = [this](juce::Graphics& graphics, juce::Rectangle<float> bounds, const ArsNovaUI::ButtonState& state) {
        ArsNovaUI::CustomLookAndFeel& lookAndFeel = ArsNovaUI::CustomLookAndFeel::get(*this);

        const auto card = bounds.reduced(ArsNovaUI::Theme::borderThickness);

        lookAndFeel.drawButtonTile(graphics, card, state);

        graphics.setColour(lookAndFeel.borderColour);
        graphics.drawRoundedRectangle(card, ArsNovaUI::Theme::paneCornerRadius, ArsNovaUI::Theme::borderThickness);

        lookAndFeel.drawIcon(graphics, card.withSizeKeepingCentre(addGlyphSide, addGlyphSide), ArsNovaUI::IconLibrary::get().add, state);
    };

    addCard.setTooltip("Add ornament");
    addCard.setSize(OrnamentTile::tileWidth, OrnamentTile::tileHeight);

    addAndMakeVisible(nameEditor);
    addAndMakeVisible(foldButton);
    addAndMakeVisible(body);

    body.addAndMakeVisible(addCard);

    syncTiles();
}

void OrnamentLibrarySection::paint(juce::Graphics& graphics)
{
    const ArsNovaUI::Theme& theme = ArsNovaUI::CustomLookAndFeel::get(*this);

    if (getY() > 0) {
        const float lineLeft  = static_cast<float>(tileGap);
        const float lineRight = static_cast<float>(getWidth() - tileGap);

        graphics.setColour(theme.borderColour);
        graphics.drawHorizontalLine(0, lineLeft, lineRight);
    }
}

void OrnamentLibrarySection::resized()
{
    auto bounds = getLocalBounds();
    auto header = bounds.removeFromTop(headerHeight).reduced(tileGap, headerInset);

    const int columns = juce::jmax(1, (getWidth() - tileGap) / (OrnamentTile::tileWidth + tileGap));
    const int count   = static_cast<int>(tiles.size());

    foldButton.setBounds(header.removeFromRight(header.getHeight()));
    nameEditor.setBounds(header);
    body.setBounds(bounds);

    for (int index = 0; index <= count; ++index) {
        juce::Component* entry = &addCard;

        const int tileX = tileGap + (index % columns) * (OrnamentTile::tileWidth + tileGap);
        const int tileY = tileGap + (index / columns) * (OrnamentTile::tileHeight + tileGap);

        if (index < count) {
            entry = tiles[static_cast<size_t>(index)].get();
        }

        entry->setTopLeftPosition(tileX, tileY);
    }
}

void OrnamentLibrarySection::syncTiles()
{
    std::vector<std::unique_ptr<OrnamentTile>> synced;

    for (const juce::ValueTree& ornament : library) {
        const auto match = std::find_if(tiles.begin(), tiles.end(), [&ornament](const std::unique_ptr<OrnamentTile>& tile) {
            return tile != nullptr && tile->ornament == ornament;
        });

        if (match != tiles.end()) {
            synced.push_back(std::move(*match));

            continue;
        }

        synced.push_back(std::make_unique<OrnamentTile>(ornamentLibraries, ornament, undoManager));

        body.addAndMakeVisible(*synced.back());
    }

    tiles = std::move(synced);

    resized();
}

void OrnamentLibrarySection::mouseDown(const juce::MouseEvent& event)
{
    if (event.y < headerHeight && event.mods.isPopupMenu()) {
        showMenu();
    }
}

int OrnamentLibrarySection::heightForWidth(int width) const
{
    const int columns = juce::jmax(1, (width - tileGap) / (OrnamentTile::tileWidth + tileGap));
    const int rows    = (static_cast<int>(tiles.size()) + columns) / columns;

    if (fold == Fold::Closed) {
        return headerHeight;
    }

    return headerHeight + tileGap + rows * (OrnamentTile::tileHeight + tileGap);
}

void OrnamentLibrarySection::toggleFold()
{
    if (fold == Fold::Open) {
        fold            = Fold::Closed;
        foldButton.icon = &ArsNovaUI::IconLibrary::get().chevronDown;
    }
    else {
        fold            = Fold::Open;
        foldButton.icon = &ArsNovaUI::IconLibrary::get().chevronUp;
    }

    body.setVisible(fold == Fold::Open);

    foldButton.repaint();

    if (onFoldChanged) {
        onFoldChanged();
    }
}

void OrnamentLibrarySection::addOrnament()
{
    juce::ValueTree ornament;

    undoManager.beginNewTransaction();

    ornament = ornamentLibraries.addOrnament(library, &undoManager);

    const OrnamentProperties added(ornament);

    ornamentLibraries.activeOrnamentUid.setValue(added.ornamentUid, nullptr);
}

void OrnamentLibrarySection::showMenu()
{
    const LibraryProperties properties(library);

    const juce::String libraryUid = properties.libraryUid;

    const auto libraryFolder = ornamentLibraries.libraryFolders.find(libraryUid);

    juce::File revealed = ornamentLibraries.folder;

    ArsNovaUI::ContextMenu menu;

    if (libraryFolder != ornamentLibraries.libraryFolders.end()) {
        revealed = libraryFolder->second.folder;
    }

    menu.addItem("reveal in finder", ArsNovaUI::ContextMenu::ItemKind::Action, [revealed]() { revealed.revealToUser(); });
    menu.addItem("remove library...", ArsNovaUI::ContextMenu::ItemKind::Action, [this]() { confirmRemove(); });

    menu.show(*this);
}

void OrnamentLibrarySection::confirmRemove()
{
    const LibraryProperties properties(library);

    const juce::String uid  = properties.libraryUid;
    const juce::String name = properties.libraryName;

    ArsNovaUI::Dialog dialog;

    dialog.title   = "Remove library";
    dialog.message = "Remove \"" + name + "\"? Its folder moves to the Trash.";

    dialog.addButton("Remove", [&libraries = ornamentLibraries, &undoManager = undoManager, uid]() {
        libraries.removeLibrary(uid, undoManager);
    });

    dialog.addButton("Cancel", nullptr);
    dialog.show(*this);
}

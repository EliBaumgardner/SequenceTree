#include "OrnamentLibrarySection.h"
#include <ArsNovaUI/Menus/ContextMenu.h>
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "../../Graph/OrnamentLibraries.h"

#include <algorithm>

OrnamentLibrarySection::OrnamentLibrarySection(OrnamentLibraries& ornamentLibraries, juce::ValueTree library, juce::UndoManager& undoManager)
    : library(library),
      ornamentLibraries(ornamentLibraries),
      undoManager(undoManager),
      nameEditor(undoManager)
{
    nameEditor.autoFitText = true;
    nameEditor.fontStyle   = ArsNovaUI::Theme::FontStyle::SemiBold;

    nameEditor.setFormat(std::make_unique<ArsNovaUI::TextFormat>(ArsNovaUI::TextFormat::labelTextLength, ArsNovaUI::TextFormat::labelCharacters));
    nameEditor.setJustification(juce::Justification::centredLeft);
    nameEditor.bindEditor(library, ValueTreeIdentifiers::LibraryName);

    foldButton.icon    = &ArsNovaUI::CustomLookAndFeel::drawChevronUpIcon;
    foldButton.onClick = [this]() { toggleFold(); };
    addCard.onClick    = [this]() { addOrnament(); };

    addCard.painter = [this](juce::Graphics& graphics, juce::Rectangle<float> bounds, const ArsNovaUI::ButtonState& state) {
        ArsNovaUI::CustomLookAndFeel& lookAndFeel = ArsNovaUI::CustomLookAndFeel::get(*this);
        const auto                    card        = bounds.reduced(ArsNovaUI::Theme::borderThickness);

        lookAndFeel.drawButtonTile(graphics, card, state);

        graphics.setColour(lookAndFeel.borderColour);
        graphics.drawRoundedRectangle(card, ArsNovaUI::Theme::paneCornerRadius, ArsNovaUI::Theme::borderThickness);

        lookAndFeel.drawAddIcon(graphics, card.withSizeKeepingCentre(addGlyphSide, addGlyphSide), state);
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
        graphics.setColour(theme.borderColour);
        graphics.drawHorizontalLine(0, static_cast<float>(tileGap), static_cast<float>(getWidth() - tileGap));
    }
}

void OrnamentLibrarySection::resized()
{
    auto      bounds  = getLocalBounds();
    auto      header  = bounds.removeFromTop(headerHeight).reduced(tileGap, headerInset);
    const int columns = juce::jmax(1, (getWidth() - tileGap) / (OrnamentTile::tileWidth + tileGap));
    const int count   = static_cast<int>(tiles.size());

    foldButton.setBounds(header.removeFromRight(header.getHeight()));
    nameEditor.setBounds(header);
    body.setBounds(bounds);

    for (int index = 0; index <= count; ++index) {
        juce::Component* entry = &addCard;

        if (index < count) {
            entry = tiles[static_cast<size_t>(index)].get();
        }

        entry->setTopLeftPosition(tileGap + (index % columns) * (OrnamentTile::tileWidth + tileGap),
                                  tileGap + (index / columns) * (OrnamentTile::tileHeight + tileGap));
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
        foldButton.icon = &ArsNovaUI::CustomLookAndFeel::drawChevronDownIcon;
    }
    else {
        fold            = Fold::Open;
        foldButton.icon = &ArsNovaUI::CustomLookAndFeel::drawChevronUpIcon;
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

    ornamentLibraries.libraries.setProperty(ValueTreeIdentifiers::ActiveOrnamentUid, ornament.getProperty(ValueTreeIdentifiers::OrnamentUid), nullptr);
}

void OrnamentLibrarySection::showMenu()
{
    const auto             libraryFolder = ornamentLibraries.libraryFolders.find(library.getProperty(ValueTreeIdentifiers::LibraryUid).toString());
    juce::File             revealed      = ornamentLibraries.folder;
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
    const juce::String uid     = library.getProperty(ValueTreeIdentifiers::LibraryUid).toString();
    const juce::String name    = library.getProperty(ValueTreeIdentifiers::LibraryName).toString();
    const auto         options = juce::MessageBoxOptions().withIconType(juce::MessageBoxIconType::QuestionIcon).withTitle("Remove library")
                                     .withMessage("Remove \"" + name + "\"? Its folder moves to the Trash.").withButton("Remove").withButton("Cancel")
                                     .withAssociatedComponent(this);

    juce::AlertWindow::showAsync(options, [&libraries = ornamentLibraries, &undoManager = undoManager, uid](int result) {
        if (result == 1) {
            libraries.removeLibrary(uid, undoManager);
        }
    });
}

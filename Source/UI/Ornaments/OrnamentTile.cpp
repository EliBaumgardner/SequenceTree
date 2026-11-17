#include "OrnamentTile.h"
#include "../../Graph/TypedProperties.h"
#include <ArsNovaUI/Menus/ContextMenu.h>
#include <ArsNovaUI/Menus/Dialog.h>
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "../../Graph/OrnamentLibraries.h"
#include "../../Graph/OrnamentFigure.h"
#include "../../Graph/OrnamentSymbol.h"

OrnamentTile::OrnamentTile(OrnamentLibraries& ornamentLibraries, juce::ValueTree ornament, juce::UndoManager& undoManager)
    : ornament         (ornament),
      ornamentLibraries(ornamentLibraries),
      undoManager      (undoManager),
      nameTag          (undoManager)
{
    nameTag.autoFitText = true;
    nameTag.fontStyle   = ArsNovaUI::Theme::FontStyle::Regular;

    nameTag.setFormat(std::make_unique<ArsNovaUI::TextFormat>(ArsNovaUI::TextFormat::labelTextLength, ArsNovaUI::TextFormat::labelCharacters));
    nameTag.bindEditor(ornament, ValueTreeIdentifiers::OrnamentName);
    nameTag.setTooltip("Ornament name");
    setTooltip("Select ornament");

    setSize(tileWidth, tileHeight);

    nameTag.setBounds(getLocalBounds().removeFromBottom(nameTagHeight).reduced(nameTagInset, 0));

    addAndMakeVisible(nameTag);
}

void OrnamentTile::paint(juce::Graphics& graphics)
{
    ArsNovaUI::CustomLookAndFeel& lookAndFeel = ArsNovaUI::CustomLookAndFeel::get(*this);

    const auto bounds = getLocalBounds().toFloat().reduced(ArsNovaUI::Theme::borderThickness);
    const auto symbol = getLocalBounds().withTrimmedTop(markerHeight).withTrimmedBottom(nameTagHeight).reduced(nameTagInset).toFloat();
    const auto marker = getLocalBounds().removeFromTop(markerHeight).reduced(nameTagInset, 0);

    const juce::String activeUid = ornamentLibraries.activeOrnamentUid;

    juce::Colour fill   = lookAndFeel.raisedColour;
    juce::Colour border = lookAndFeel.borderColour;

    juce::String status;

    const OrnamentProperties properties(ornament);

    const juce::String ornamentUid  = properties.ornamentUid;
    const juce::String storedSymbol = properties.symbol;

    if (activeUid == ornamentUid) {
        fill   = lookAndFeel.selectedColour;
        border = lookAndFeel.accentColour;
    }

    graphics.setColour(fill);
    graphics.fillRoundedRectangle(bounds, ArsNovaUI::Theme::paneCornerRadius);

    graphics.setColour(border);
    graphics.drawRoundedRectangle(bounds, ArsNovaUI::Theme::paneCornerRadius, ArsNovaUI::Theme::borderThickness);

    switch (OrnamentFigure::statusOf(ornament)) {
        case OrnamentStatus::Draft:          status = "draft";       break;
        case OrnamentStatus::ChangesPending: status = "unpublished"; break;
        case OrnamentStatus::Published:                              break;
    }

    graphics.setColour(lookAndFeel.mutedTextColour);
    graphics.setFont(lookAndFeel.font(ArsNovaUI::Theme::FontStyle::Regular, ArsNovaUI::Theme::labelFontHeight));
    graphics.drawText(status, marker, juce::Justification::centredRight, true);

    graphics.setColour(lookAndFeel.textColour);

    const std::vector<ArsNovaUI::SymbolStroke> symbolStrokes = OrnamentSymbol::read(storedSymbol);

    lookAndFeel.drawStrokeSymbol(graphics, symbol, symbolStrokes);
}

void OrnamentTile::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isPopupMenu()) {
        showMenu();

        return;
    }

    const OrnamentProperties properties(ornament);

    ornamentLibraries.activeOrnamentUid.setValue(properties.ornamentUid, nullptr);
}

void OrnamentTile::showMenu()
{
    const OrnamentProperties properties(ornament);

    const juce::String uid = properties.ornamentUid;

    ArsNovaUI::ContextMenu menu;

    menu.addItem("duplicate", ArsNovaUI::ContextMenu::ItemKind::Action, [this, uid]() {
        undoManager.beginNewTransaction();

        ornamentLibraries.duplicateOrnament(uid, &undoManager);
    });

    menu.addItem("delete...", ArsNovaUI::ContextMenu::ItemKind::Action, [this]() { confirmDelete(); });

    menu.show(*this);
}

void OrnamentTile::confirmDelete()
{
    const OrnamentProperties properties(ornament);

    const juce::String uid  = properties.ornamentUid;
    const juce::String name = properties.ornamentName;

    ArsNovaUI::Dialog dialog;

    dialog.title   = "Delete ornament";
    dialog.message = "Delete \"" + name + "\"?";

    dialog.addButton("Delete", [&libraries = ornamentLibraries, &undoManager = undoManager, uid]() {
        undoManager.beginNewTransaction();

        libraries.removeOrnament(uid, &undoManager);
    });

    dialog.addButton("Cancel", nullptr);
    dialog.show(*this);
}

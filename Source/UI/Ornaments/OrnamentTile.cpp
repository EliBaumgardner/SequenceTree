#include "OrnamentTile.h"
#include "../Menus/ContextMenu.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../../Graph/OrnamentLibraries.h"

OrnamentTile::OrnamentTile(OrnamentLibraries& ornamentLibraries, juce::ValueTree ornament, juce::UndoManager& undoManager)
    : ornament(ornament),
      ornamentLibraries(ornamentLibraries),
      undoManager(undoManager),
      nameTag(undoManager)
{
    nameTag.autoFitText = true;
    nameTag.fontStyle   = Theme::FontStyle::Regular;

    nameTag.setFormat(std::make_unique<TextFormat>(TextFormat::labelTextLength, TextFormat::labelCharacters));
    nameTag.bindEditor(ornament, ValueTreeIdentifiers::OrnamentName);

    setSize(tileWidth, tileHeight);

    nameTag.setBounds(getLocalBounds().removeFromBottom(nameTagHeight).reduced(nameTagInset, 0));

    addAndMakeVisible(nameTag);
}

void OrnamentTile::paint(juce::Graphics& graphics)
{
    CustomLookAndFeel& lookAndFeel = CustomLookAndFeel::get(*this);
    const auto         bounds      = getLocalBounds().toFloat().reduced(Theme::borderThickness);
    const auto         symbol      = getLocalBounds().withTrimmedTop(markerHeight).withTrimmedBottom(nameTagHeight).reduced(nameTagInset).toFloat();
    const juce::String activeUid   = ornamentLibraries.libraries.getProperty(ValueTreeIdentifiers::ActiveOrnamentUid).toString();
    const auto         marker      = getLocalBounds().removeFromTop(markerHeight).reduced(nameTagInset, 0);
    juce::Colour       fill        = lookAndFeel.raisedColour;
    juce::Colour       border      = lookAndFeel.borderColour;
    juce::String       status;

    if (activeUid == ornament.getProperty(ValueTreeIdentifiers::OrnamentUid).toString()) {
        fill   = lookAndFeel.selectedColour;
        border = lookAndFeel.accentColour;
    }

    graphics.setColour(fill);
    graphics.fillRoundedRectangle(bounds, Theme::paneCornerRadius);

    graphics.setColour(border);
    graphics.drawRoundedRectangle(bounds, Theme::paneCornerRadius, Theme::borderThickness);

    switch (ornamentLibraries.statusOf(ornament)) {
        case OrnamentStatus::Draft:          status = "draft";       break;
        case OrnamentStatus::ChangesPending: status = "unpublished"; break;
        case OrnamentStatus::Published:                              break;
    }

    graphics.setColour(lookAndFeel.mutedTextColour);
    graphics.setFont(lookAndFeel.font(Theme::FontStyle::Regular, Theme::labelFontHeight));
    graphics.drawText(status, marker, juce::Justification::centredRight, true);

    graphics.setColour(lookAndFeel.textColour);

    lookAndFeel.drawStrokeSymbol(graphics, symbol, OrnamentLibraries::readSymbol(ornament.getProperty(ValueTreeIdentifiers::OrnamentSymbol).toString()));
}

void OrnamentTile::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isPopupMenu()) {
        showMenu();

        return;
    }

    ornamentLibraries.libraries.setProperty(ValueTreeIdentifiers::ActiveOrnamentUid, ornament.getProperty(ValueTreeIdentifiers::OrnamentUid), nullptr);
}

void OrnamentTile::showMenu()
{
    const juce::String uid = ornament.getProperty(ValueTreeIdentifiers::OrnamentUid).toString();
    ContextMenu        menu;

    menu.addItem("duplicate", ContextMenu::ItemKind::Action, [this, uid]() {
        undoManager.beginNewTransaction();

        ornamentLibraries.duplicateOrnament(uid, &undoManager);
    });

    menu.addItem("delete...", ContextMenu::ItemKind::Action, [this]() { confirmDelete(); });

    menu.show(*this);
}

void OrnamentTile::confirmDelete()
{
    const juce::String uid     = ornament.getProperty(ValueTreeIdentifiers::OrnamentUid).toString();
    const juce::String name    = ornament.getProperty(ValueTreeIdentifiers::OrnamentName).toString();
    const auto         options = juce::MessageBoxOptions().withIconType(juce::MessageBoxIconType::QuestionIcon).withTitle("Delete ornament")
                                     .withMessage("Delete \"" + name + "\"?").withButton("Delete").withButton("Cancel").withAssociatedComponent(this);

    juce::AlertWindow::showAsync(options, [&libraries = ornamentLibraries, &undoManager = undoManager, uid](int result) {
        if (result != 1) {
            return;
        }

        undoManager.beginNewTransaction();

        libraries.removeOrnament(uid, &undoManager);
    });
}

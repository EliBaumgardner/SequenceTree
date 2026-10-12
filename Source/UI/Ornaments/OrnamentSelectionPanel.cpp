#include "OrnamentSelectionPanel.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../../Graph/OrnamentLibraries.h"

#include <algorithm>

OrnamentSelectionPanel::OrnamentSelectionPanel(OrnamentLibraries& ornamentLibraries, juce::UndoManager& undoManager)
    : resizer(PanelResizer::Edge::Left),
      ornamentLibraries(ornamentLibraries),
      undoManager(undoManager)
{
    titlebar.newLibraryButton.onClick = [this]() {
        this->undoManager.beginNewTransaction();

        this->ornamentLibraries.addLibrary("New Library", &this->undoManager);
    };

    titlebar.addLibraryButton.onClick = [this]() { chooseLibraryFile(); };

    viewport.setViewedComponent(&sectionList, false);
    viewport.setScrollBarsShown(true, false);

    addAndMakeVisible(titlebar);
    addAndMakeVisible(viewport);
    addAndMakeVisible(resizer);

    ornamentLibraries.libraries.addListener(this);

    triggerAsyncUpdate();
}

OrnamentSelectionPanel::~OrnamentSelectionPanel()
{
    ornamentLibraries.libraries.removeListener(this);
}

void OrnamentSelectionPanel::paint(juce::Graphics& graphics)
{
    CustomLookAndFeel& lookAndFeel = CustomLookAndFeel::get(*this);

    graphics.setColour(lookAndFeel.windowColour);
    graphics.fillRect(getLocalBounds());

    graphics.setColour(lookAndFeel.borderColour);
    graphics.drawVerticalLine(0, 0.0f, static_cast<float>(getHeight()));

    lookAndFeel.drawPane(graphics, viewport.getBounds().toFloat());
}

void OrnamentSelectionPanel::resized()
{
    auto bounds = getLocalBounds();

    titlebar.setBounds(bounds.removeFromTop(titlebarHeight));
    viewport.setBounds(bounds.reduced(Theme::windowGutter));
    resizer.setBounds(bounds.removeFromLeft(Theme::windowGutter));

    layoutSections();
}

juce::Point<int> OrnamentSelectionPanel::narrowestSize() const
{
    const int barInset      = juce::roundToInt(titlebarHeight * titlebarInsetRatio);
    const int buttonSize    = titlebarHeight - barInset * 2;
    const int titlebarWidth = barInset * 2 + buttonSize * 2 + Bar::contentSpacing;
    const int tileArea      = OrnamentLibrarySection::tileGap * 2 + OrnamentTile::tileWidth;
    const int viewportWidth = Theme::windowGutter * 2 + viewport.getScrollBarThickness() + tileArea;
    const int sectionHeight = OrnamentLibrarySection::headerHeight + OrnamentLibrarySection::tileGap * 2 + OrnamentTile::tileHeight;

    return { juce::jmax(titlebarWidth, viewportWidth), titlebarHeight + Theme::windowGutter * 2 + sectionHeight };
}

void OrnamentSelectionPanel::chooseLibraryFile()
{
    const int chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::canSelectDirectories;

    fileChooser = std::make_unique<juce::FileChooser>("Add a library", juce::File::getSpecialLocation(juce::File::userHomeDirectory), "*.ornaments");

    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& chooser) {
        const juce::File file = chooser.getResult();

        if (file == juce::File() || ornamentLibraries.importLibrary(file).isValid()) {
            return;
        }

        juce::AlertWindow::showAsync(juce::MessageBoxOptions().withIconType(juce::MessageBoxIconType::WarningIcon).withTitle("Library not added")
                                         .withMessage(file.getFileName() + " is already added, or is not an ornament library.").withButton("OK")
                                         .withAssociatedComponent(this), nullptr);
    });
}

void OrnamentSelectionPanel::layoutSections()
{
    const int width = viewport.getMaximumVisibleWidth();
    int       top   = 0;

    for (const std::unique_ptr<OrnamentLibrarySection>& section : sections) {
        const int height = section->heightForWidth(width);

        section->setBounds(0, top, width, height);

        top += height;
    }

    sectionList.setSize(width, top);
}

void OrnamentSelectionPanel::handleAsyncUpdate()
{
    std::vector<std::unique_ptr<OrnamentLibrarySection>> synced;

    for (const juce::ValueTree& library : ornamentLibraries.libraries) {
        const auto match = std::find_if(sections.begin(), sections.end(), [&library](const std::unique_ptr<OrnamentLibrarySection>& section) {
            return section != nullptr && section->library == library;
        });

        if (match != sections.end()) {
            synced.push_back(std::move(*match));

            synced.back()->syncTiles();

            continue;
        }

        synced.push_back(std::make_unique<OrnamentLibrarySection>(ornamentLibraries, library, undoManager));

        synced.back()->onFoldChanged = [this]() { layoutSections(); };

        sectionList.addAndMakeVisible(*synced.back());
    }

    sections = std::move(synced);

    layoutSections();
}

void OrnamentSelectionPanel::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    if (child.getType() == ValueTreeIdentifiers::OrnamentLibrary || child.getType() == ValueTreeIdentifiers::OrnamentData) {
        triggerAsyncUpdate();

        return;
    }

    repaint();
}

void OrnamentSelectionPanel::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index)
{
    if (child.getType() == ValueTreeIdentifiers::OrnamentLibrary || child.getType() == ValueTreeIdentifiers::OrnamentData) {
        triggerAsyncUpdate();

        return;
    }

    repaint();
}

void OrnamentSelectionPanel::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    if (property != ValueTreeIdentifiers::OrnamentName && property != ValueTreeIdentifiers::LibraryName) {
        repaint();
    }
}

OrnamentSelectionPanel::PanelTitlebar::PanelTitlebar()
    : Bar(Orientation::Horizontal, titlebarInsetRatio)
{
    newLibraryButton.icon = &CustomLookAndFeel::drawAddIcon;
    addLibraryButton.icon = &CustomLookAndFeel::drawImportIcon;

    newLibraryButton.setTooltip("New library");
    addLibraryButton.setTooltip("Add library...");

    addAndMakeVisible(newLibraryButton);
    addAndMakeVisible(addLibraryButton);
}

void OrnamentSelectionPanel::PanelTitlebar::resized()
{
    auto      bounds     = getContentBounds();
    const int buttonSize = bounds.getHeight();

    newLibraryButton.setBounds(bounds.removeFromLeft(buttonSize));
    bounds.removeFromLeft(contentSpacing);

    addLibraryButton.setBounds(bounds.removeFromLeft(buttonSize));
}

#include "OrnamentMarks.h"
#include "NodeCanvas.h"
#include "../Node/Node.h"
#include "../../Graph/OrnamentLibraries.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../Theme/CustomLookAndFeel.h"

void OrnamentMarkView::paint(juce::Graphics& graphics)
{
    CustomLookAndFeel::get(*this).drawNodeOrnament(graphics, getLocalBounds().toFloat(), mark, symbol);
}

OrnamentMarks::OrnamentMarks(NodeCanvas& nodeCanvas, OrnamentLibraries& ornamentLibraries)
    : nodeCanvas(nodeCanvas),
      ornamentLibraries(ornamentLibraries)
{
    ornamentLibraries.libraries.addListener(this);
}

OrnamentMarks::~OrnamentMarks()
{
    ornamentLibraries.libraries.removeListener(this);

    for (const auto& [node, view] : views) {
        node->removeComponentListener(this);
    }
}

void OrnamentMarks::refresh(Node& node)
{
    const juce::String    uid      = node.nodeValueTree.getProperty(ValueTreeIdentifiers::OrnamentUid).toString();
    const juce::ValueTree ornament = ornamentLibraries.findOrnament(uid);
    OrnamentMark          mark     = OrnamentMark::None;

    if (uid.isNotEmpty()) {
        mark = OrnamentMark::Missing;
    }

    if (uid.isNotEmpty() && ornamentLibraries.statusOf(ornament) != OrnamentStatus::Draft) {
        mark = OrnamentMark::Symbol;
    }

    if (mark == OrnamentMark::None) {
        views.erase(&node);

        node.removeComponentListener(this);

        return;
    }

    std::unique_ptr<OrnamentMarkView>& view = views[&node];

    if (view == nullptr) {
        view = std::make_unique<OrnamentMarkView>();

        view->setInterceptsMouseClicks(true, false);
        view->setMouseCursor(juce::MouseCursor::ParentCursor);

        nodeCanvas.addChildComponent(*view);

        node.addComponentListener(this);
    }

    view->mark          = mark;
    view->symbol        = ornament.getProperty(ValueTreeIdentifiers::OrnamentSymbol).toString();
    view->nodeValueTree = node.nodeValueTree;

    place(node);

    view->repaint();
}

void OrnamentMarks::place(Node& node)
{
    OrnamentMarkView* const    view       = views[&node].get();
    const juce::Rectangle<int> nodeSquare = node.getBounds().withTrimmedLeft(node.interiorLeftInset);
    const int                  markHeight = juce::roundToInt(static_cast<float>(nodeSquare.getHeight()) * markHeightRatio);

    view->setBounds(nodeSquare.withHeight(markHeight).withBottomY(nodeSquare.getY()));
    view->setVisible(node.isVisible());
}

void OrnamentMarks::valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier& property)
{
    if (property == ValueTreeIdentifiers::OrnamentSymbol) {
        refreshAll();
    }
}

void OrnamentMarks::refreshAll()
{
    for (const auto& [nodeId, node] : nodeCanvas.nodeManager.all()) {
        if (node != nullptr) {
            refresh(*node);
        }
    }
}

void OrnamentMarks::valueTreeChildAdded(juce::ValueTree&, juce::ValueTree&)
{
    refreshAll();
}

void OrnamentMarks::valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree&, int)
{
    refreshAll();
}

void OrnamentMarks::componentMovedOrResized(juce::Component& component, bool, bool)
{
    place(static_cast<Node&>(component));
}

void OrnamentMarks::componentVisibilityChanged(juce::Component& component)
{
    place(static_cast<Node&>(component));
}

void OrnamentMarks::componentBeingDeleted(juce::Component& component)
{
    views.erase(&component);
}

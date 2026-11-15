#include <algorithm>
#include <array>
#include <atomic>
#include <bitset>
#include <chrono>
#include <cmath>
#include <concepts>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <queue>
#include <random>
#include <set>
#include <span>
#include <sstream>
#include <stack>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>
#include <ranges>
#include <any>
#include <bit>
#include <charconv>
#include <compare>
#include <filesystem>
#include <iomanip>
#include <numbers>
#include <regex>
#include <shared_mutex>
#include <source_location>
#include <valarray>
#include <forward_list>
#include <cxxabi.h>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

#define private public
#define protected public
#include "Plugin/PluginProcessor.h"
#include "Plugin/PluginEditor.h"
#include "UI/Menus/AllowedTraversalsMenu.h"
#include "UI/Menus/MenuArea.h"
#include "UI/Bars/MenuBar.h"
#include <ArsNovaUI/Menus/ColourSelector.h>
#include <ArsNovaUI/Bars/Bar.h>
#include <ArsNovaUI/Editors/ValueEditor.h>
#include <ArsNovaUI/Canvas/CanvasIdentifiers.h>
#undef private
#undef protected

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

namespace
{

juce::String number(double value)
{
    if (std::abs(value) < 0.0005) {
        return "0";
    }

    return juce::String(value, 3).trimCharactersAtEnd("0").trimCharactersAtEnd(".");
}

juce::String hexColour(juce::Colour colour, float opacity = 1.0f)
{
    const auto alpha = static_cast<juce::uint8>(juce::jlimit(0, 255, juce::roundToInt(colour.getFloatAlpha() * opacity * 255.0f)));

    return "#" + juce::String::toHexString(colour.getRed()).paddedLeft('0', 2) + juce::String::toHexString(colour.getGreen()).paddedLeft('0', 2)
           + juce::String::toHexString(colour.getBlue()).paddedLeft('0', 2) + juce::String::toHexString(alpha).paddedLeft('0', 2);
}

juce::String svgPath(const juce::Path& source, const juce::AffineTransform& transform)
{
    juce::Path path(source);

    path.applyTransform(transform);

    juce::String           data;
    juce::Path::Iterator   iterator(path);

    while (iterator.next()) {
        switch (iterator.elementType) {
            case juce::Path::Iterator::startNewSubPath:
                data << "M " << number(iterator.x1) << " " << number(iterator.y1) << " ";
                break;
            case juce::Path::Iterator::lineTo:
                data << "L " << number(iterator.x1) << " " << number(iterator.y1) << " ";
                break;
            case juce::Path::Iterator::quadraticTo:
                data << "Q " << number(iterator.x1) << " " << number(iterator.y1) << " " << number(iterator.x2) << " " << number(iterator.y2) << " ";
                break;
            case juce::Path::Iterator::cubicTo:
                data << "C " << number(iterator.x1) << " " << number(iterator.y1) << " " << number(iterator.x2) << " " << number(iterator.y2) << " "
                     << number(iterator.x3) << " " << number(iterator.y3) << " ";
                break;
            case juce::Path::Iterator::closePath:
                data << "Z ";
                break;
        }
    }

    return data.trimEnd();
}

juce::var rectangleVar(juce::Rectangle<float> area)
{
    juce::Array<juce::var> values { number(area.getX()).getDoubleValue(), number(area.getY()).getDoubleValue(),
                                    number(area.getWidth()).getDoubleValue(), number(area.getHeight()).getDoubleValue() };

    return values;
}

juce::var pointVar(juce::Point<float> point)
{
    juce::Array<juce::var> values { number(point.x).getDoubleValue(), number(point.y).getDoubleValue() };

    return values;
}

float transformScale(const juce::AffineTransform& transform)
{
    return std::sqrt(std::abs(transform.getDeterminant()));
}

bool isAxisAligned(const juce::AffineTransform& transform)
{
    return std::abs(transform.mat01) < 1.0e-5f && std::abs(transform.mat10) < 1.0e-5f && transform.mat00 > 0.0f && transform.mat11 > 0.0f;
}

juce::String demangle(const char* name)
{
    int         status    = 0;
    char* const demangled = abi::__cxa_demangle(name, nullptr, nullptr, &status);
    juce::String result(name);

    if (status == 0 && demangled != nullptr) {
        result = demangled;
    }

    std::free(demangled);

    return result;
}

std::map<const juce::Typeface*, std::unordered_map<uint32_t, juce::String>> reverseCharacterMaps;

const std::unordered_map<uint32_t, juce::String>& reverseCharacterMap(const juce::Typeface::Ptr& typeface)
{
    auto found = reverseCharacterMaps.find(typeface.get());

    if (found != reverseCharacterMaps.end()) {
        return found->second;
    }

    std::unordered_map<uint32_t, juce::String> map;

    const std::array<std::pair<juce::juce_wchar, juce::juce_wchar>, 5> ranges { {
        { 0x20, 0x24f }, { 0x370, 0x3ff }, { 0x2000, 0x2bff }, { 0x3000, 0x303f }, { 0x1d100, 0x1d1ff } } };

    for (const auto& range : ranges) {
        for (juce::juce_wchar character = range.second; character >= range.first; --character) {
            if (const auto glyph = typeface->getNominalGlyphForCodepoint(character)) {
                map[*glyph] = juce::String::charToString(character);
            }
        }
    }

    const juce::Font   font(juce::FontOptions(typeface).withHeight(20.0f));
    const juce::String letters = "abcdefghijklmnopqrstuvwxyz";
    juce::StringArray  candidates { "ffi", "ffl", "ffj", "fft", "ttf", "fff" };

    for (const juce::juce_wchar first : letters) {
        for (const juce::juce_wchar second : letters) {
            candidates.add(juce::String::charToString(first) + juce::String::charToString(second));
        }
    }

    for (const juce::String& candidate : candidates) {
        juce::GlyphArrangement arrangement;

        arrangement.addLineOfText(font, candidate, 0.0f, 0.0f);

        std::vector<int> glyphs;

        for (int index = 0; index < arrangement.getNumGlyphs(); ++index) {
            if (! arrangement.getGlyph(index).isWhitespace()) {
                glyphs.push_back(arrangement.getGlyph(index).getGlyphIndex());
            }
        }

        if (glyphs.size() == 1 && map.find(static_cast<uint32_t>(glyphs.front())) == map.end()) {
            map[static_cast<uint32_t>(glyphs.front())] = candidate;
        }
    }

    return reverseCharacterMaps.emplace(typeface.get(), std::move(map)).first->second;
}

std::map<juce::String, juce::String> imageStore;

juce::String storeImage(const juce::Image& image)
{
    juce::MemoryOutputStream stream;
    juce::PNGImageFormat     format;

    format.writeImageToStream(image, stream);

    const juce::String encoded = juce::Base64::toBase64(stream.getData(), stream.getDataSize());
    const juce::String key     = "img" + juce::String::toHexString(encoded.hashCode64());

    imageStore[key] = encoded;

    return key;
}

class Recorder : public juce::LowLevelGraphicsContext
{
public:

    Recorder(juce::Array<juce::var>& output, const juce::AffineTransform& base, juce::Rectangle<float> clip)
        : ops(output)
    {
        state.transform = base;
        state.clip      = clip;
    }

    bool isVectorDevice() const override { return false; }

    void setOrigin(juce::Point<int> origin) override
    {
        state.transform = juce::AffineTransform::translation(origin.toFloat()).followedBy(state.transform);
    }

    void addTransform(const juce::AffineTransform& transform) override { state.transform = transform.followedBy(state.transform); }

    float getPhysicalPixelScaleFactor() const override { return 2.0f * transformScale(state.transform); }

    bool clipToRectangle(const juce::Rectangle<int>& area) override
    {
        state.clip = state.clip.getIntersection(area.toFloat().transformedBy(state.transform));

        return ! state.clip.isEmpty();
    }

    bool clipToRectangleList(const juce::RectangleList<int>& areas) override { return clipToRectangle(areas.getBounds()); }

    void excludeClipRectangle(const juce::Rectangle<int>&) override {}

    void clipToPath(const juce::Path& path, const juce::AffineTransform& transform) override
    {
        const juce::AffineTransform full = transform.followedBy(state.transform);
        juce::Path                  absolute(path);

        absolute.applyTransform(full);

        state.clip = state.clip.getIntersection(absolute.getBounds());

        juce::DynamicObject::Ptr clipPath = new juce::DynamicObject();

        clipPath->setProperty("d", svgPath(path, full));
        clipPath->setProperty("winding", path.isUsingNonZeroWinding() ? "NONZERO" : "EVENODD");

        state.clipPaths.add(juce::var(clipPath.get()));
    }

    void clipToImageAlpha(const juce::Image& image, const juce::AffineTransform& transform) override
    {
        state.clip = state.clip.getIntersection(image.getBounds().toFloat().transformedBy(transform.followedBy(state.transform)));
    }

    bool clipRegionIntersects(const juce::Rectangle<int>& area) override
    {
        return state.clip.intersects(area.toFloat().transformedBy(state.transform));
    }

    juce::Rectangle<int> getClipBounds() const override
    {
        return state.clip.transformedBy(state.transform.inverted()).getSmallestIntegerContainer();
    }

    bool isClipEmpty() const override { return state.clip.isEmpty(); }

    void saveState() override { stack.push_back(state); }

    void restoreState() override
    {
        if (! stack.empty()) {
            state = stack.back();
            stack.pop_back();
        }
    }

    void beginTransparencyLayer(float opacity) override
    {
        saveState();

        juce::DynamicObject::Ptr op = new juce::DynamicObject();

        op->setProperty("type", "beginLayer");
        op->setProperty("opacity", opacity);

        ops.add(juce::var(op.get()));

        state.opacity = 1.0f;
    }

    void endTransparencyLayer() override
    {
        juce::DynamicObject::Ptr op = new juce::DynamicObject();

        op->setProperty("type", "endLayer");

        ops.add(juce::var(op.get()));

        restoreState();
    }

    void setFill(const juce::FillType& fill) override { state.fill = fill; }

    void setOpacity(float opacity) override { state.opacity = opacity; }

    void setInterpolationQuality(juce::Graphics::ResamplingQuality) override {}

    void fillRect(const juce::Rectangle<int>& area, bool) override { fillRect(area.toFloat()); }

    void fillRect(const juce::Rectangle<float>& area) override
    {
        if (! isAxisAligned(state.transform)) {
            juce::Path path;

            path.addRectangle(area);
            fillPath(path, {});

            return;
        }

        const auto absolute = area.transformedBy(state.transform);

        emit("rect", absolute, [&](juce::DynamicObject& op) { op.setProperty("fill", paintVar()); });
    }

    void fillRectList(const juce::RectangleList<float>& areas) override
    {
        for (const auto& area : areas) {
            fillRect(area);
        }
    }

    void fillPath(const juce::Path& path, const juce::AffineTransform& transform) override
    {
        if (path.isEmpty()) {
            return;
        }

        const juce::AffineTransform full = transform.followedBy(state.transform);

        emit("path", path.getBoundsTransformed(full), [&](juce::DynamicObject& op) {
            op.setProperty("d", svgPath(path, full));
            op.setProperty("winding", path.isUsingNonZeroWinding() ? "NONZERO" : "EVENODD");
            op.setProperty("fill", paintVar());
        });
    }

    void strokePath(const juce::Path& path, const juce::PathStrokeType& strokeType, const juce::AffineTransform& transform) override
    {
        if (path.isEmpty()) {
            return;
        }

        const juce::AffineTransform full  = transform.followedBy(state.transform);
        const float                 width = strokeType.getStrokeThickness() * transformScale(state.transform);

        emit("stroke", path.getBoundsTransformed(full).expanded(width * 0.5f), [&](juce::DynamicObject& op) {
            op.setProperty("d", svgPath(path, full));
            op.setProperty("width", width);
            op.setProperty("join", strokeType.getJointStyle() == juce::PathStrokeType::mitered ? "MITER"
                                   : strokeType.getJointStyle() == juce::PathStrokeType::curved ? "ROUND" : "BEVEL");
            op.setProperty("cap", strokeType.getEndStyle() == juce::PathStrokeType::butt ? "NONE"
                                  : strokeType.getEndStyle() == juce::PathStrokeType::square ? "SQUARE" : "ROUND");
            op.setProperty("fill", paintVar());
        });
    }

    void drawImage(const juce::Image& image, const juce::AffineTransform& transform) override
    {
        const juce::AffineTransform full     = transform.followedBy(state.transform);
        const auto                  absolute = image.getBounds().toFloat().transformedBy(full);
        const float                 frostScale = 1.0f / ArsNovaUI::Theme::frostScale;

        if (std::abs(transform.mat00 - frostScale) < 0.01f && std::abs(transform.mat11 - frostScale) < 0.01f) {
            emit("frost", state.clip, [&](juce::DynamicObject& op) { op.setProperty("radius", ArsNovaUI::Theme::frostBlurRadius / ArsNovaUI::Theme::frostScale); });

            return;
        }

        emit("image", absolute, [&](juce::DynamicObject& op) {
            op.setProperty("image", storeImage(image));
            op.setProperty("opacity", state.fill.getOpacity() * state.opacity);
            op.setProperty("axisAligned", isAxisAligned(full));
        });
    }

    void drawLine(const juce::Line<float>& line) override { drawLineWithThickness(line, 1.0f); }

    void drawLineWithThickness(const juce::Line<float>& line, float thickness) override
    {
        juce::Path path;

        path.startNewSubPath(line.getStart());
        path.lineTo(line.getEnd());

        strokePath(path, juce::PathStrokeType(thickness, juce::PathStrokeType::mitered, juce::PathStrokeType::butt), {});
    }

    void fillEllipse(const juce::Rectangle<float>& area) override
    {
        if (! isAxisAligned(state.transform)) {
            juce::Path path;

            path.addEllipse(area);
            fillPath(path, {});

            return;
        }

        emit("ellipse", area.transformedBy(state.transform), [&](juce::DynamicObject& op) { op.setProperty("fill", paintVar()); });
    }

    void drawEllipse(const juce::Rectangle<float>& area, float thickness) override
    {
        if (! isAxisAligned(state.transform)) {
            juce::Path path;

            path.addEllipse(area);
            strokePath(path, juce::PathStrokeType(thickness), {});

            return;
        }

        const float width = thickness * transformScale(state.transform);

        emit("ellipse", area.transformedBy(state.transform).expanded(width * 0.5f), [&](juce::DynamicObject& op) {
            op.setProperty("strokeArea", rectangleVar(area.transformedBy(state.transform)));
            op.setProperty("stroke", paintVar());
            op.setProperty("width", width);
        });
    }

    void fillRoundedRectangle(const juce::Rectangle<float>& area, float cornerSize) override
    {
        if (! isAxisAligned(state.transform)) {
            juce::Path path;

            path.addRoundedRectangle(area, cornerSize);
            fillPath(path, {});

            return;
        }

        const float scale = transformScale(state.transform);

        emit("rect", area.transformedBy(state.transform), [&](juce::DynamicObject& op) {
            op.setProperty("fill", paintVar());
            op.setProperty("radius", juce::jmin(cornerSize, area.getWidth() * 0.5f, area.getHeight() * 0.5f) * scale);
        });
    }

    void drawRoundedRectangle(const juce::Rectangle<float>& area, float cornerSize, float thickness) override
    {
        if (! isAxisAligned(state.transform)) {
            juce::Path path;

            path.addRoundedRectangle(area, cornerSize);
            strokePath(path, juce::PathStrokeType(thickness), {});

            return;
        }

        const float scale = transformScale(state.transform);
        const float width = thickness * scale;

        emit("rect", area.transformedBy(state.transform).expanded(width * 0.5f), [&](juce::DynamicObject& op) {
            op.setProperty("strokeArea", rectangleVar(area.transformedBy(state.transform)));
            op.setProperty("stroke", paintVar());
            op.setProperty("width", width);
            op.setProperty("radius", juce::jmin(cornerSize, area.getWidth() * 0.5f, area.getHeight() * 0.5f) * scale);
        });
    }

    void setFont(const juce::Font& font) override { state.font = font; }

    const juce::Font& getFont() override { return state.font; }

    void drawGlyphs(juce::Span<const uint16_t> glyphs, juce::Span<const juce::Point<float>> positions, const juce::AffineTransform& transform) override
    {
        const juce::AffineTransform full     = transform.followedBy(state.transform);
        const float                 angle    = std::atan2(full.mat10, full.mat00);
        const juce::AffineTransform unrotate = juce::AffineTransform::rotation(-angle);
        size_t                      first    = 0;

        const auto lineY = [&](size_t index) { return positions[index].transformedBy(full).transformedBy(unrotate).y; };

        for (size_t index = 1; index <= glyphs.size(); ++index) {
            if (index == glyphs.size() || std::abs(lineY(index) - lineY(first)) > 0.5f) {
                drawGlyphLine(juce::Span<const uint16_t>(glyphs.data() + first, index - first), juce::Span<const juce::Point<float>>(positions.data() + first, index - first), full,
                              angle);
                first = index;
            }
        }
    }

    void drawGlyphLine(juce::Span<const uint16_t> glyphs, juce::Span<const juce::Point<float>> positions, const juce::AffineTransform& full, float angle)
    {
        if (glyphs.empty()) {
            return;
        }

        static const std::map<juce::juce_wchar, juce::String> ligatures { { 0xfb00, "ff" }, { 0xfb01, "fi" }, { 0xfb02, "fl" }, { 0xfb03, "ffi" },
                                                                          { 0xfb04, "ffl" }, { 0xfb05, "st" }, { 0xfb06, "st" } };

        const juce::Typeface::Ptr typeface = state.font.getTypefacePtr();
        const float               scale    = transformScale(full);
        const auto&               map      = reverseCharacterMap(typeface);
        juce::String              text;
        juce::String              lastCharacter;
        juce::Array<juce::var>    xs;

        for (size_t index = 0; index < glyphs.size(); ++index) {
            const auto found = map.find(glyphs[index]);

            lastCharacter = found != map.end() ? found->second : juce::String("?");

            const auto ligature = lastCharacter.length() == 1 ? ligatures.find(lastCharacter[0]) : ligatures.end();

            if (ligature != ligatures.end()) {
                lastCharacter = ligature->second;
            }

            text << lastCharacter;

            xs.add(number(positions[index].transformedBy(full).x).getDoubleValue());
        }

        const auto  start       = positions.front().transformedBy(full);
        const float ascent      = state.font.getAscent() * scale;
        const float descent     = state.font.getDescent() * scale;
        const float lastAdvance = juce::GlyphArrangement::getStringWidth(state.font, lastCharacter) * scale;
        const float span        = positions.back().transformedBy(full).getDistanceFrom(start) + lastAdvance;
        const float endX        = start.x + span;

        juce::Rectangle<float> extent(start.x, start.y - ascent, juce::jmax(1.0f, span), ascent + descent);

        if (std::abs(angle) > 1.0e-4f) {
            extent = extent.transformedBy(juce::AffineTransform::rotation(angle, start.x, start.y));
        }

        emit("text", extent, [&](juce::DynamicObject& op) {
            op.setProperty("text", text);
            op.setProperty("family", typeface->getName());
            op.setProperty("style", typeface->getStyle());
            op.setProperty("size", state.font.getHeightInPoints() * scale);
            op.setProperty("height", state.font.getHeight() * scale);
            op.setProperty("ascent", ascent);
            op.setProperty("descent", descent);
            op.setProperty("baseline", start.y);
            op.setProperty("x", start.x);
            op.setProperty("endX", endX);
            op.setProperty("xs", xs);
            op.setProperty("horizontalScale", state.font.getHorizontalScale());
            op.setProperty("rotation", juce::radiansToDegrees(angle));
            op.setProperty("fill", paintVar());
        });
    }

    std::unique_ptr<juce::ImageType> getPreferredImageTypeForTemporaryImages() const override
    {
        return std::make_unique<juce::SoftwareImageType>();
    }

    uint64_t getFrameId() const override { return 0; }

private:

    struct State
    {
        juce::AffineTransform  transform;
        juce::Rectangle<float> clip;
        juce::Array<juce::var> clipPaths;
        juce::FillType         fill { juce::Colours::black };
        float                  opacity = 1.0f;
        juce::Font             font { juce::FontOptions {} };
    };

    juce::var paintVar() const
    {
        juce::DynamicObject::Ptr paint = new juce::DynamicObject();
        const juce::FillType&    fill  = state.fill;

        if (fill.isGradient()) {
            const juce::ColourGradient& gradient = *fill.gradient;
            const juce::AffineTransform full     = fill.transform.followedBy(state.transform);
            juce::Array<juce::var>      stops;

            for (int index = 0; index < gradient.getNumColours(); ++index) {
                juce::Array<juce::var> stop { gradient.getColourPosition(index), hexColour(gradient.getColour(index), fill.getOpacity() * state.opacity) };

                stops.add(stop);
            }

            paint->setProperty("type", gradient.isRadial ? "radial" : "linear");
            paint->setProperty("p1", pointVar(gradient.point1.transformedBy(full)));
            paint->setProperty("p2", pointVar(gradient.point2.transformedBy(full)));
            paint->setProperty("stops", stops);

            return paint.get();
        }

        if (fill.isTiledImage()) {
            paint->setProperty("type", "solid");
            paint->setProperty("c", hexColour(juce::Colours::grey, 0.5f));

            return paint.get();
        }

        paint->setProperty("type", "solid");
        paint->setProperty("c", hexColour(fill.colour, state.opacity));

        return paint.get();
    }

    template <typename Fill>
    void emit(const char* type, juce::Rectangle<float> bounds, Fill&& fill)
    {
        if (state.clip.isEmpty() || ! state.clip.intersects(bounds.expanded(0.5f))) {
            return;
        }

        juce::DynamicObject::Ptr op = new juce::DynamicObject();

        op->setProperty("type", type);
        op->setProperty("bounds", rectangleVar(bounds));

        if (! state.clip.contains(bounds.reduced(0.01f)) || ! state.clipPaths.isEmpty()) {
            op->setProperty("clip", rectangleVar(state.clip));
        }

        if (! state.clipPaths.isEmpty()) {
            op->setProperty("clipPaths", state.clipPaths);
        }

        fill(*op);

        ops.add(juce::var(op.get()));
    }

    juce::Array<juce::var>& ops;
    State                   state;
    std::vector<State>      stack;
};

juce::String describe(juce::Component& component)
{
    juce::String label = component.getName();

    if (label.isEmpty()) {
        label = component.getComponentID();
    }

    if (auto* tooltipClient = dynamic_cast<juce::SettableTooltipClient*>(&component)) {
        const juce::String tooltip = tooltipClient->getTooltip();

        if (tooltip.isNotEmpty() && label.isEmpty()) {
            label = tooltip;
        }
    }

    if (auto* button = dynamic_cast<ArsNovaUI::IconButton*>(&component)) {
        if (label.isEmpty() && button->state.text.isNotEmpty()) {
            label = button->state.text;
        }
    }

    if (auto* textLabel = dynamic_cast<juce::Label*>(&component)) {
        if (label.isEmpty()) {
            label = textLabel->getText();
        }
    }

    return label;
}

juce::var exportComponent(juce::Component& component, const juce::AffineTransform& toRoot, juce::Rectangle<float> parentClip)
{
    if (! component.isVisible() || component.getWidth() <= 0 || component.getHeight() <= 0) {
        return {};
    }

    const auto absolute = component.getLocalBounds().toFloat().transformedBy(toRoot);
    const auto clip     = absolute.getIntersection(parentClip);

    if (clip.isEmpty()) {
        return {};
    }

    juce::DynamicObject::Ptr node = new juce::DynamicObject();
    juce::Array<juce::var>   below;
    juce::Array<juce::var>   above;
    juce::Array<juce::var>   children;

    node->setProperty("class", demangle(typeid(component).name()));
    node->setProperty("label", describe(component));
    node->setProperty("bounds", rectangleVar(absolute));
    node->setProperty("scale", transformScale(toRoot));
    node->setProperty("alpha", component.getAlpha());

    {
        Recorder         recorder(below, toRoot, clip);
        juce::Graphics   graphics(recorder);

        component.paint(graphics);

        if (auto* valueEditor = dynamic_cast<ArsNovaUI::ValueEditor*>(&component); valueEditor != nullptr && ! valueEditor->isEditing && ! valueEditor->persistentEditor) {
            const int count = below.size();

            if (count >= 2 && below[count - 2]["type"].toString() == "path" && below[count - 1]["type"].toString() == "stroke") {
                const juce::Colour ink = juce::Colour::fromString(below[count - 2]["fill"]["c"].toString().substring(1, 7)).withAlpha(
                    static_cast<juce::uint8>(below[count - 2]["fill"]["c"].toString().substring(7, 9).getHexValue32()));
                const auto&            key  = valueEditor->textOutlineKey;
                const juce::Font       font = valueEditor->displayFont(key.text);
                juce::GlyphArrangement arrangement;

                below.removeRange(count - 2, 2);

                arrangement.addCurtailedLineOfText(font, key.text, 0.0f, 0.0f, static_cast<float>(key.bounds.getWidth()), false);
                arrangement.justifyGlyphs(0, arrangement.getNumGlyphs(), 0.0f, 0.0f, static_cast<float>(key.bounds.getWidth()),
                                          static_cast<float>(key.bounds.getHeight()), key.justification);

                graphics.setColour(ink);
                arrangement.draw(graphics, juce::AffineTransform::translation(key.bounds.toFloat().getPosition()));

                juce::String drawnText;

                for (int index = count - 2; index < below.size(); ++index) {
                    drawnText << below[index]["text"].toString();
                }

                const juce::String intended = key.text.removeCharacters(" ");

                if (drawnText.containsChar('?') && drawnText.removeCharacters(" ").length() == intended.length()) {
                    int cursor = 0;

                    for (int index = count - 2; index < below.size(); ++index) {
                        juce::String corrected;

                        for (const juce::juce_wchar character : below[index]["text"].toString()) {
                            corrected << juce::String::charToString(character == ' ' ? character : intended[cursor++]);
                        }

                        if (auto* op = below[index].getDynamicObject()) {
                            op->setProperty("text", corrected);
                        }
                    }
                }
            }
        }
    }

    for (juce::Component* child : component.getChildren()) {
        const juce::AffineTransform childToRoot = juce::AffineTransform::translation(child->getPosition().toFloat())
                                                      .followedBy(child->getTransform())
                                                      .followedBy(toRoot);

        const juce::var exported = exportComponent(*child, childToRoot, clip);

        if (! exported.isVoid()) {
            children.add(exported);
        }
    }

    {
        Recorder       recorder(above, toRoot, clip);
        juce::Graphics graphics(recorder);

        component.paintOverChildren(graphics);
    }

    node->setProperty("below", below);
    node->setProperty("children", children);
    node->setProperty("above", above);

    return node.get();
}

juce::var exportScene(const juce::String& name, const juce::String& group, juce::Component& root, juce::Colour background, const juce::String& note = {})
{
    juce::DynamicObject::Ptr scene = new juce::DynamicObject();

    scene->setProperty("name", name);
    scene->setProperty("group", group);
    scene->setProperty("width", root.getWidth());
    scene->setProperty("height", root.getHeight());
    scene->setProperty("background", hexColour(background));
    scene->setProperty("note", note);
    scene->setProperty("root", exportComponent(root, {}, root.getLocalBounds().toFloat()));
    scene->setProperty("reference", storeImage(root.createComponentSnapshot(root.getLocalBounds(), true, 2.0f)));

    return scene.get();
}

juce::var themeVar(const ArsNovaUI::Theme& theme)
{
    juce::DynamicObject::Ptr colours = new juce::DynamicObject();

#define THEME_COLOUR(member) colours->setProperty(#member, hexColour(theme.member));
    THEME_COLOUR(windowColour)
    THEME_COLOUR(surfaceColour)
    THEME_COLOUR(raisedColour)
    THEME_COLOUR(selectedColour)
    THEME_COLOUR(canvasColour)
    THEME_COLOUR(defaultNodeColour)
    THEME_COLOUR(textColour)
    THEME_COLOUR(softTextColour)
    THEME_COLOUR(mutedTextColour)
    THEME_COLOUR(dimTextColour)
    THEME_COLOUR(accentColour)
    THEME_COLOUR(onAccentColour)
    THEME_COLOUR(accentSoftColour)
    THEME_COLOUR(borderColour)
    THEME_COLOUR(borderStrongColour)
    THEME_COLOUR(hoverColour)
    THEME_COLOUR(gridColour)
    THEME_COLOUR(barColour)
    THEME_COLOUR(arrowColour)
    THEME_COLOUR(errorColour)
    THEME_COLOUR(successColour)
    THEME_COLOUR(dropShadowColour)
    THEME_COLOUR(captionColour)
    THEME_COLOUR(lineNumberColour)
    THEME_COLOUR(selectionBoxColour)
    THEME_COLOUR(selectionRingColour)
    THEME_COLOUR(hoverRingColour)
    THEME_COLOUR(outlineRingColour)
    THEME_COLOUR(popupMenuColour)
    THEME_COLOUR(popupMenuBorderColour)
    THEME_COLOUR(popupMenuTextColour)
    THEME_COLOUR(popupMenuHighlightColour)
    THEME_COLOUR(popupMenuHighlightTextColour)
    THEME_COLOUR(scrollBarTrackColour)
    THEME_COLOUR(scrollBarThumbColour)
    THEME_COLOUR(scrollBarThumbHoverColour)
    THEME_COLOUR(arrowHeadColour)
#undef THEME_COLOUR

    return colours.get();
}

juce::var themeNumbers()
{
    juce::DynamicObject::Ptr numbers = new juce::DynamicObject();
    using T = ArsNovaUI::Theme;

#define THEME_NUMBER(member) numbers->setProperty(#member, static_cast<double>(T::member));
    THEME_NUMBER(barHeight)
    THEME_NUMBER(textHeight)
    THEME_NUMBER(labelFontHeight)
    THEME_NUMBER(borderThickness)
    THEME_NUMBER(paneCornerRadius)
    THEME_NUMBER(groupCornerRadius)
    THEME_NUMBER(canvasCornerRadius)
    THEME_NUMBER(windowGutter)
    THEME_NUMBER(menuContentWidth)
    THEME_NUMBER(menuEdgeInset)
    THEME_NUMBER(buttonGap)
    THEME_NUMBER(nodeRadius)
    THEME_NUMBER(popupMenuItemHeight)
    THEME_NUMBER(popupMenuPadding)
    THEME_NUMBER(scrollBarThickness)
    THEME_NUMBER(selectionRingWidth)
    THEME_NUMBER(selectionRingGap)
    THEME_NUMBER(outerRingWidth)
    THEME_NUMBER(arrowShaftThickness)
#undef THEME_NUMBER

    return numbers.get();
}

}

namespace
{

struct MenuEntry
{
    juce::String     label;
    bool             separator   = false;
    bool             enabled     = true;
    bool             ticked      = false;
    bool             highlighted = false;
    juce::Component* component   = nullptr;
    int              idealWidth  = 0;
    int              idealHeight = 0;
};

class PopupMenuMock : public juce::Component
{
public:

    PopupMenuMock(ArsNovaUI::CustomLookAndFeel& lookAndFeel, std::vector<MenuEntry> menuEntries)
        : entries(std::move(menuEntries))
    {
        setLookAndFeel(&lookAndFeel);
        setName("PopupMenu");
        setVisible(true);

        const int border = lookAndFeel.getPopupMenuBorderSize();
        int       width  = 0;
        int       y      = border;

        for (auto& entry : entries) {
            int idealWidth  = entry.idealWidth;
            int idealHeight = entry.idealHeight;

            if (entry.component == nullptr) {
                lookAndFeel.getIdealPopupMenuItemSize(entry.label, entry.separator, 0, idealWidth, idealHeight);
            }

            width = juce::jmax(width, idealWidth);
            rows.push_back({ 0, y, 0, idealHeight });
            y += idealHeight;
        }

        for (size_t index = 0; index < entries.size(); ++index) {
            rows[index].setWidth(width);

            if (entries[index].component != nullptr) {
                addAndMakeVisible(entries[index].component);
                entries[index].component->setBounds(rows[index]);
            }
        }

        setSize(width + border * 2, y + border);
    }

    ~PopupMenuMock() override
    {
        for (auto& entry : entries) {
            if (entry.component != nullptr) {
                removeChildComponent(entry.component);
            }
        }

        setLookAndFeel(nullptr);
    }

    void paint(juce::Graphics& graphics) override
    {
        auto& lookAndFeel = ArsNovaUI::CustomLookAndFeel::get(*this);

        lookAndFeel.drawPopupMenuBackgroundWithOptions(graphics, getWidth(), getHeight(), juce::PopupMenu::Options());

        for (size_t index = 0; index < entries.size(); ++index) {
            const MenuEntry& entry = entries[index];

            if (entry.component != nullptr) {
                continue;
            }

            lookAndFeel.drawPopupMenuItem(graphics, rows[index], entry.separator, entry.enabled, entry.highlighted, entry.ticked, false, entry.label, {}, nullptr,
                                          nullptr);
        }
    }

    std::vector<MenuEntry>            entries;
    std::vector<juce::Rectangle<int>> rows;
};

class Stage : public juce::Component
{
public:

    Stage(ArsNovaUI::CustomLookAndFeel& lookAndFeel, juce::Colour fill) : background(fill)
    {
        setLookAndFeel(&lookAndFeel);
        setVisible(true);
    }

    ~Stage() override { setLookAndFeel(nullptr); }

    void paint(juce::Graphics& graphics) override { graphics.fillAll(background); }

    juce::Colour background;
};

std::vector<MenuEntry> actionEntries(std::initializer_list<juce::String> labels, int highlighted = -1)
{
    std::vector<MenuEntry> entries;
    int                    index = 0;

    for (const auto& label : labels) {
        MenuEntry entry;

        entry.separator   = label == "-";
        entry.label       = entry.separator ? juce::String() : label;
        entry.enabled     = ! entry.separator;
        entry.highlighted = index == highlighted;

        entries.push_back(entry);
        ++index;
    }

    return entries;
}

juce::var exportMenuScene(ArsNovaUI::CustomLookAndFeel& lookAndFeel, const juce::String& name, std::vector<MenuEntry> entries, const juce::String& note)
{
    PopupMenuMock menu(lookAndFeel, std::move(entries));

    return exportScene(name, "Context menus", menu, juce::Colours::transparentBlack, note);
}

class StatesBoard : public juce::Component
{
public:

    StatesBoard(ArsNovaUI::CustomLookAndFeel& lookAndFeel) : theme(lookAndFeel)
    {
        setLookAndFeel(&lookAndFeel);
        setVisible(true);
        setName("Component states");

        const std::array<ArsNovaUI::ButtonState::Look, 3> looks { ArsNovaUI::ButtonState::Look::Tinted, ArsNovaUI::ButtonState::Look::Accent,
                                                                  ArsNovaUI::ButtonState::Look::Raised };

        for (size_t row = 0; row < looks.size() + 1; ++row) {
            for (int column = 0; column < columnCount; ++column) {
                auto* button = buttons.add(new ArsNovaUI::IconButton());
                const bool textRow = row == looks.size();

                button->state.look = textRow ? ArsNovaUI::ButtonState::Look::Tinted : looks[row];

                if (textRow) {
                    button->state.text = "X";
                } else {
                    button->icon = &ArsNovaUI::CustomLookAndFeel::drawCircleIcon;
                }

                button->state.isHovered  = column == 1;
                button->state.isDown     = column == 2;
                button->state.isSelected = column == 3;

                button->setBounds(labelWidth + column * cellWidth, top + static_cast<int>(row) * rowHeight, buttonWidth, buttonHeight);
                addAndMakeVisible(button);

                if (column == 4) {
                    button->setEnabled(false);
                }

                button->setName(juce::String(rowNames[row]) + " - " + columnNames[column]);
            }
        }

        setSize(labelWidth + columnCount * cellWidth + 16, top + 4 * rowHeight + nodeSectionHeight);
    }

    ~StatesBoard() override { setLookAndFeel(nullptr); }

    void paint(juce::Graphics& graphics) override
    {
        graphics.fillAll(theme.windowColour);
        graphics.setFont(theme.font(ArsNovaUI::Theme::FontStyle::Regular, ArsNovaUI::Theme::labelFontHeight));
        graphics.setColour(theme.mutedTextColour);

        for (int column = 0; column < columnCount; ++column) {
            graphics.drawText(columnNames[column], labelWidth + column * cellWidth, 6, cellWidth, 12, juce::Justification::centredLeft);
        }

        for (int row = 0; row < 4; ++row) {
            graphics.drawText(rowNames[row], 8, top + row * rowHeight, labelWidth - 8, buttonHeight, juce::Justification::centredLeft);
        }

        const int nodeTop = top + 4 * rowHeight + 20;

        graphics.drawText("Nodes", 8, nodeTop - 18, 100, 12, juce::Justification::centredLeft);

        const juce::Colour nodeColour = juce::Colour::fromRGB(196, 134, 30);
        const std::map<int, juce::Colour> none;
        const std::map<int, juce::Colour> highlighted { { 0, theme.accentColour } };

        struct Variant { const char* name; bool hovered, selected, outlined, outerRing, innerRim; bool highlight; };

        const std::array<Variant, 7> variants { { { "default", false, false, false, false, false, false },
                                                   { "hovered", true, false, false, false, false, false },
                                                   { "selected", false, true, false, false, false, false },
                                                   { "outlined", false, false, true, false, false, false },
                                                   { "outer ring", false, false, false, true, false, false },
                                                   { "inner rim", false, false, false, false, true, false },
                                                   { "highlighted", false, false, false, false, false, true } } };

        for (size_t index = 0; index < variants.size(); ++index) {
            const Variant& variant = variants[index];

            for (int shape = 0; shape < 2; ++shape) {
                const auto area = juce::Rectangle<float>(static_cast<float>(labelWidth + static_cast<int>(index) * nodeCellWidth),
                                                         static_cast<float>(nodeTop + shape * nodeRowHeight), 48.0f, 48.0f);

                ArsNovaUI::NodeVisual visual { area, nodeColour, variant.highlight ? highlighted : none };

                visual.isHovered       = variant.hovered;
                visual.isSelected      = variant.selected;
                visual.isOutlined      = variant.outlined;
                visual.hasOuterRing    = variant.outerRing;
                visual.hasInnerRim     = variant.innerRim;
                visual.outerRingColour = theme.accentColour;

                if (shape == 0) {
                    theme.drawCircleNode(graphics, visual);
                } else {
                    visual.bounds = ArsNovaUI::CustomLookAndFeel::getNodeCircleBounds(area);
                    theme.drawSquareNode(graphics, visual);
                }
            }

            graphics.setColour(theme.mutedTextColour);
            graphics.drawText(variant.name, labelWidth + static_cast<int>(index) * nodeCellWidth, nodeTop + 2 * nodeRowHeight - 6, nodeCellWidth, 12,
                              juce::Justification::centredLeft);
        }

        graphics.drawText("circle", 8, nodeTop + 18, labelWidth - 8, 12, juce::Justification::centredLeft);
        graphics.drawText("square", 8, nodeTop + nodeRowHeight + 18, labelWidth - 8, 12, juce::Justification::centredLeft);
    }

    static constexpr int columnCount       = 5;
    static constexpr int labelWidth        = 64;
    static constexpr int cellWidth         = 56;
    static constexpr int buttonWidth       = 17;
    static constexpr int buttonHeight      = 16;
    static constexpr int rowHeight         = 26;
    static constexpr int top               = 24;
    static constexpr int nodeCellWidth     = 70;
    static constexpr int nodeRowHeight     = 62;
    static constexpr int nodeSectionHeight = 170;

    static constexpr std::array<const char*, 5> columnNames { "normal", "hovered", "pressed", "selected", "disabled" };
    static constexpr std::array<const char*, 4> rowNames    { "tinted", "accent", "raised", "text button" };

    ArsNovaUI::CustomLookAndFeel& theme;
    juce::OwnedArray<ArsNovaUI::IconButton> buttons;
};

juce::String readFilterState()
{
    juce::PropertiesFile::Options options;

    options.applicationName     = "SequenceTree";
    options.filenameSuffix      = ".settings";
    options.osxLibrarySubFolder = "Application Support";

    juce::PropertiesFile settings(options);

    return settings.getValue("filterState");
}

}

int main(int argumentCount, char** arguments)
{
    if (argumentCount < 4) {
        std::cerr << "usage: FigmaExporter <out.json> <window width> <window height>\n";
        return 2;
    }

    juce::ScopedJuceInitialiser_GUI initialiser;

    const juce::File outputFile   = juce::File::getCurrentWorkingDirectory().getChildFile(arguments[1]);
    const int        windowWidth  = juce::String(arguments[2]).getIntValue();
    const int        windowHeight = juce::String(arguments[3]).getIntValue();

    juce::AudioProcessor::setTypeOfNextNewPlugin(juce::AudioProcessor::wrapperType_VST3);

    std::unique_ptr<juce::AudioProcessor> processorBase(createPluginFilter());
    auto& processor = dynamic_cast<SequenceTreeAudioProcessor&>(*processorBase);

    const juce::String filterState = readFilterState();

    if (filterState.isNotEmpty()) {
        juce::MemoryBlock state;

        state.fromBase64Encoding(filterState);
        processor.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    }

    std::unique_ptr<juce::AudioProcessorEditor> editorBase(processor.createEditorIfNeeded());
    auto& editor = dynamic_cast<SequenceTreeAudioProcessorEditor&>(*editorBase);

    editor.setSize(windowWidth, windowHeight);

    ArsNovaUI::CustomLookAndFeel& lookAndFeel = editor.lookAndFeel;
    juce::Array<juce::var>        scenes;

    const float zoom = editor.scaledView->zoom;

    scenes.add(exportScene("Main window - in a DAW", "Main window", editor.workspace, lookAndFeel.windowColour,
                           "The plugin inside a DAW, editor content at zoom 1 (the app showed it at zoom " + juce::String(zoom) + ")."));

    {
        juce::AudioProcessor::setTypeOfNextNewPlugin(juce::AudioProcessor::wrapperType_Standalone);

        std::unique_ptr<juce::AudioProcessor> standaloneBase(createPluginFilter());
        auto& standalone = dynamic_cast<SequenceTreeAudioProcessor&>(*standaloneBase);

        if (filterState.isNotEmpty()) {
            juce::MemoryBlock state;

            state.fromBase64Encoding(filterState);
            standalone.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
        }

        std::unique_ptr<juce::AudioProcessorEditor> standaloneEditorBase(standalone.createEditorIfNeeded());
        auto& standaloneEditor = dynamic_cast<SequenceTreeAudioProcessorEditor&>(*standaloneEditorBase);

        standaloneEditor.setSize(windowWidth, windowHeight);

        scenes.add(exportScene("Main window - standalone", "Main window", standaloneEditor.workspace, lookAndFeel.windowColour,
                               "The standalone app: no sync-to-host button in the transport pane. Window title bar drawn by the standalone wrapper is not included."));

        standalone.editorBeingDeleted(standaloneEditorBase.get());
        standaloneEditorBase.reset();
    }

    const int fullMenuWidth = 10000;

    editor.menuArea->togglePanel(MenuArea::ActivePanel::Node);
    editor.menuArea->resizer.onWidthDragged(fullMenuWidth);
    scenes.add(exportScene("Main window - node menu open", "Main window", editor.workspace, lookAndFeel.windowColour, "Node panel opened from the menu column."));

    editor.menuArea->togglePanel(MenuArea::ActivePanel::Traversal);
    scenes.add(exportScene("Main window - traversal menu open", "Main window", editor.workspace, lookAndFeel.windowColour,
                           "Traversal panel opened from the menu column."));

    editor.menuArea->togglePanel(MenuArea::ActivePanel::Traversal);
    editor.menuArea->resizer.onWidthDragged(0);

    const auto exportWindow = [&](ArsNovaUI::PopupWindowLauncher& launcher, juce::Point<float> openingScale, const juce::String& note) {
        std::unique_ptr<juce::Component> content = launcher.contentFactory();
        Stage                            stage(lookAndFeel, juce::Colour::fromRGB(30, 30, 30));
        const int                        width  = juce::roundToInt(content->getWidth() * openingScale.x);
        const int                        height = juce::roundToInt(content->getHeight() * openingScale.y);

        stage.setSize(width, height);
        stage.addAndMakeVisible(*content);
        content->setBounds(stage.getLocalBounds());

        scenes.add(exportScene(launcher.windowTitle + " window", "Windows", stage, juce::Colour::fromRGB(30, 30, 30), note));

        stage.removeChildComponent(content.get());
    };

    exportWindow(editor.settingsLauncher, SettingsMenu::openingScale, "Opened from the settings icon. Native title bar not drawn.");
    exportWindow(editor.ornamentLauncher, OrnamentWindow::openingScale, "Opened from the ornament button in the bottom bar. Native title bar not drawn.");
    exportWindow(editor.traversalRulesLauncher, { 1.0f, 1.0f }, "Opened from the traversal and node menus. Native title bar not drawn.");

    {
        juce::ValueTree connection;

        for (const auto& node : processor.graphState.nodeMap) {
            const juce::ValueTree children = node.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds);

            if (children.getNumChildren() > 0) {
                connection = children.getChild(0);
                break;
            }
        }

        if (connection.isValid()) {
            auto  content = std::make_unique<AllowedTraversalsMenu>(lookAndFeel, processor.graphState, processor.undoManager, connection);
            Stage stage(lookAndFeel, juce::Colour::fromRGB(30, 30, 30));

            content->setSize(AllowedTraversalsMenu::defaultWidth, content->getIdealHeight());
            stage.setSize(content->getWidth(), content->getHeight());
            stage.addAndMakeVisible(*content);

            scenes.add(exportScene("Allowed Traversals window", "Windows", stage, juce::Colour::fromRGB(30, 30, 30),
                                   "Opened from an arrow's context menu (edit allowed traversals). Native title bar not drawn."));

            stage.removeChildComponent(content.get());
        }
    }

    {
        Stage                     stage(lookAndFeel, juce::Colours::transparentBlack);
        ArsNovaUI::ColourSelector selector(processor.colourPresets, processor.undoManager);
        auto&                     picker = selector.picker;

        selector.colour = lookAndFeel.accentColour;

        stage.setSize(400, 400);
        picker.setSize(ArsNovaUI::ColourPicker::pickerWidth, juce::roundToInt(ArsNovaUI::ColourPicker::pickerWidth * ArsNovaUI::ColourPicker::heightToWidthRatio));
        picker.showColour(selector.colour);

        juce::CallOutBox box(picker, juce::Rectangle<int>(180, 4, 24, 24), &stage);

        box.setLookAndFeel(&lookAndFeel);

        const auto boxBounds = box.getBounds();

        stage.setSize(boxBounds.getRight() + 8, boxBounds.getBottom() + 8);

        scenes.add(exportScene("Colour picker call-out", "Popups", stage, juce::Colours::transparentBlack,
                               "Shown when any colour swatch (node, traversal, theme, paint tool) is clicked."));

        box.setLookAndFeel(nullptr);
    }

    {
        Stage stage(lookAndFeel, juce::Colours::transparentBlack);
        const juce::String tooltip = "Play / stop";
        const auto bounds = lookAndFeel.getTooltipBounds(tooltip, {}, juce::Rectangle<int>(0, 0, 400, 200));

        struct TooltipMock : public juce::Component
        {
            juce::String text;
            void paint(juce::Graphics& graphics) override { getLookAndFeel().drawTooltip(graphics, text, getWidth(), getHeight()); }
        } mock;

        mock.text = tooltip;
        mock.setName("TooltipWindow");
        stage.setSize(bounds.getWidth(), bounds.getHeight());
        mock.setBounds(stage.getLocalBounds());
        stage.addAndMakeVisible(mock);

        scenes.add(exportScene("Tooltip", "Popups", stage, juce::Colours::transparentBlack, "juce::TooltipWindow drawn by the look-and-feel."));

        stage.removeChildComponent(&mock);
    }

    scenes.add(exportMenuScene(lookAndFeel, "Selection menu", actionEntries({ "copy", "paste", "delete" }, 0), "Right-click on the canvas (SelectionGestures::showSelectionMenu)."));

    {
        auto entries = actionEntries({ "edit allowed traversals", "traversal arrow", "sync" }, 0);

        entries[1].ticked = true;

        scenes.add(exportMenuScene(lookAndFeel, "Arrow menu", entries, "Right-click on an arrow (ArrowMenu::showArrowContextMenu); sync only on modulator-root arrows."));
    }

    {
        std::vector<MenuEntry> entries;

        for (const auto& item : editor.titleBar->displaySelector.items) {
            MenuEntry entry;

            entry.label   = item.label;
            entry.enabled = item.isEnabled;

            entries.push_back(entry);
        }

        if (! entries.empty()) {
            entries.front().highlighted = true;
        }

        scenes.add(exportMenuScene(lookAndFeel, "Display mode menu", entries, "The display selector in the title bar (ItemSelector::showMenu)."));
    }

    {
        auto& axis    = editor.bottomBar->xAxis;
        auto  entries = actionEntries({ "pitch", "duration" });

        entries[1].ticked = true;

        MenuEntry slider;

        slider.component   = &axis.multiplierSlider;
        slider.idealWidth  = BottomBar::multiplierSliderWidth;
        slider.idealHeight = ArsNovaUI::Theme::popupMenuItemHeight;

        juce::Component* const previousParent = axis.multiplierSlider.getParentComponent();

        entries.push_back(slider);

        scenes.add(exportMenuScene(lookAndFeel, "Bind axis menu", entries, "The X / Y bind buttons in the bottom bar (BottomBar::showAxisMenu)."));

        if (previousParent != nullptr) {
            previousParent->addChildComponent(axis.multiplierSlider);
        }
    }

    {
        auto entries = actionEntries({ "repeat" });

        entries[0].ticked = true;

        scenes.add(exportMenuScene(lookAndFeel, "Quaver menu", entries, "Right-click the quaver (note preview) button in the bottom bar."));
    }

    scenes.add(exportMenuScene(lookAndFeel, "Ornament mark menu", actionEntries({ "delete ornament" }, 0), "Right-click an ornament mark on the canvas (OrnamentApplier)."));
    scenes.add(exportMenuScene(lookAndFeel, "Rule tree menu", actionEntries({ "open", "make live", "-", "new rule", "new folder", "-", "rename", "delete..." }, 0),
                               "Right-click a rule in the Traversal Rules window (RuleTreeView::showRowMenu)."));
    scenes.add(exportMenuScene(lookAndFeel, "Rule panel menu", actionEntries({ "new rule", "new folder", "-", "expand all folders", "collapse all folders" }, 0),
                               "The more button above the rule tree (RuleTreePanel)."));
    scenes.add(exportMenuScene(lookAndFeel, "Ornament library menu", actionEntries({ "reveal in finder", "remove library..." }, 0),
                               "Right-click a library header in the Ornaments window (OrnamentLibrarySection::showMenu)."));
    scenes.add(exportMenuScene(lookAndFeel, "Ornament tile menu", actionEntries({ "duplicate", "delete..." }, 0), "Right-click an ornament tile (OrnamentTile::showMenu)."));

    {
        StatesBoard board(lookAndFeel);

        scenes.add(exportScene("Component states", "Popups", board, lookAndFeel.windowColour,
                               "Icon and text button states for each look, and the node visual states, drawn by the look-and-feel."));
    }

    juce::DynamicObject::Ptr document = new juce::DynamicObject();
    juce::DynamicObject::Ptr images   = new juce::DynamicObject();
    const ArsNovaUI::Theme   defaultTheme;

    for (const auto& [key, encoded] : imageStore) {
        images->setProperty(key, encoded);
    }

    document->setProperty("generated", juce::Time::getCurrentTime().toISO8601(true));
    document->setProperty("zoom", zoom);
    document->setProperty("theme", themeVar(lookAndFeel));
    document->setProperty("defaultTheme", themeVar(defaultTheme));
    document->setProperty("numbers", themeNumbers());
    document->setProperty("scenes", scenes);
    document->setProperty("images", images.get());

    outputFile.replaceWithText(juce::JSON::toString(juce::var(document.get()), juce::JSON::FormatOptions {}.withSpacing(juce::JSON::Spacing::none)));

    std::cout << "wrote " << outputFile.getFullPathName() << " (" << scenes.size() << " scenes, " << imageStore.size() << " images)\n";

    processor.editorBeingDeleted(editorBase.get());
    editorBase.reset();

    return 0;
}

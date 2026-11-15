# Figma mockup

A Figma copy of every SequenceTree window, built from what the app actually draws. Each component in the Figma file is a frame named after its C++ class, holding editable shapes and text, so the look can be redesigned in Figma without touching the code.

## Open it in Figma

Development plugins need the Figma desktop app.

1. Run `design/figma/export.sh`. It builds the standalone, builds the exporter against `cmake-build-debug`, and writes `plugin/code.js`.
2. In Figma desktop: Plugins > Development > Import plugin from manifest, then pick `design/figma/plugin/manifest.json`.
3. Open a new, empty design file and run Plugins > Development > SequenceTree Mockup.

Run `export.sh` again after any UI change and re-run the plugin in a fresh file. The optional arguments are the window size in points (default `1580 805`, the standalone window's content area).

## What the file contains

- **Pages.** *Main window* (in a DAW, standalone, node panel open, traversal panel open); *Windows* (Settings, Ornaments, Traversal Rules, Allowed Traversals); *Popups* (colour picker call-out, tooltip, component states); *Context menus* (all ten); *Components* (one example of every component class).
- **Layers.** Each JUCE component becomes a clipping frame called `Class - label`, nested as in the app. Inside it are the shapes, strokes and text it paints, in paint order. Text layers use Geist and Geist Mono at the app's sizes. Frosted bars use a background blur.
- **Theme variables.** The *SequenceTree Theme* collection holds every `ArsNovaUI::Theme` colour in two modes: *Current theme* (your saved theme colour) and *Default theme*. Fills that match a theme colour exactly are bound to its variable, so changing one variable recolours every use. *SequenceTree Sizes* lists bar height, corner radii, gutters and font heights.
- **Reference layer.** Every frame has a hidden, locked image named `Reference - app render`. Show it to compare against the app pixel for pixel.

Everything is at zoom 1 (one Figma pixel per JUCE logical pixel). The app showed it at your saved interface zoom.

## How it works

`exporter/FigmaExporter.cpp` links `libSequenceTree_SharedCode.a`, restores the standalone's saved patch and builds the real editor and windows offscreen. It paints every visible component into a recording graphics context. That context logs each fill, stroke, ellipse, rounded rectangle, gradient, image and glyph run, with its colour, clip and position. `plugin/code.template.js` turns that log into Figma layers. The exporter reads private members through `#define private public`; it is a build tool and never runs in the plugin.

`verify/` checks the plugin without Figma. It runs `code.js` against a mock of the Figma API, renders the layers in headless Chrome and diffs each frame against the app render (`npm install` there once, then `python3 verify/verify.py`).

## Known differences

- The app draws sharps (♯) in Hiragino Sans, α on group nodes in Helvetica Bold and tooltips in Lucida Grande Bold (macOS fallbacks). Figma desktop on a Mac loads these as local fonts; elsewhere it substitutes Geist.
- Hover, pressed and selected states appear only on the *Component states* frame and in the menus' highlighted first item.
- The native window title bars are not part of the mockup.
- Changes made in Figma do not flow back into the code.

const DATA = __SCENE_DATA__;

const PAGE_ORDER = ["Main window", "Windows", "Popups", "Context menus"];
const SCENE_GAP = 120;
const CATALOGUE_GAP = 48;
const FROST_BLUR_SCALE = 2;

const FONT_MAP = {
  "Geist|Regular": { family: "Geist", style: "Regular" },
  "Geist|SemiBold": { family: "Geist", style: "SemiBold" },
  "Geist Mono|Regular": { family: "Geist Mono", style: "Regular" }
};

const FALLBACK_FONT = { family: "Geist", style: "Regular" };

const themeVariables = {};
const fontCache = {};
const imageCache = {};
const classExamples = {};

function shortClassName(name) {
  return name
    .replace(/\(anonymous namespace\)::/g, "")
    .replace(/^main::/, "")
    .replace(/^ArsNovaUI::/, "");
}

function hexToRgba(hex) {
  const value = hex.replace("#", "");
  return {
    r: parseInt(value.substr(0, 2), 16) / 255,
    g: parseInt(value.substr(2, 2), 16) / 255,
    b: parseInt(value.substr(4, 2), 16) / 255,
    a: value.length >= 8 ? parseInt(value.substr(6, 2), 16) / 255 : 1
  };
}

function solidPaint(hex) {
  const colour = hexToRgba(hex);
  const variable = themeVariables[hex.toLowerCase()];
  /** @type {SolidPaint} */
  const paint = { type: "SOLID", color: { r: colour.r, g: colour.g, b: colour.b }, opacity: colour.a };

  if (variable) {
    return figma.variables.setBoundVariableForPaint(paint, "color", variable);
  }

  return paint;
}

function isGradient(fill) {
  return fill && (fill.type === "linear" || fill.type === "radial");
}

function opPaint(op) {
  return op.fill || op.stroke;
}

function escapeXml(text) {
  return String(text).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/"/g, "&quot;");
}

function svgPaint(paint, id, defs) {
  if (!isGradient(paint)) {
    const colour = hexToRgba(paint.c);
    return { value: "#" + paint.c.replace("#", "").substr(0, 6), opacity: colour.a };
  }

  const stops = paint.stops.map(function (stop) {
    const colour = hexToRgba(stop[1]);
    return '<stop offset="' + stop[0] + '" stop-color="#' + stop[1].replace("#", "").substr(0, 6) + '" stop-opacity="' + colour.a + '"/>';
  }).join("");

  if (paint.type === "linear") {
    defs.push('<linearGradient id="' + id + '" gradientUnits="userSpaceOnUse" x1="' + paint.p1[0] + '" y1="' + paint.p1[1] + '" x2="' + paint.p2[0] + '" y2="' + paint.p2[1] + '">' + stops + "</linearGradient>");
  } else {
    const radius = Math.hypot(paint.p2[0] - paint.p1[0], paint.p2[1] - paint.p1[1]);
    defs.push('<radialGradient id="' + id + '" gradientUnits="userSpaceOnUse" cx="' + paint.p1[0] + '" cy="' + paint.p1[1] + '" r="' + radius + '">' + stops + "</radialGradient>");
  }

  return { value: "url(#" + id + ")", opacity: 1 };
}

function svgShape(op) {
  const fill = op.fill || op.stroke;
  const defs = [];
  const paint = svgPaint(fill, "g", defs);
  let shape = "";

  if (op.type === "path") {
    shape = '<path d="' + op.d + '" fill-rule="' + (op.winding === "EVENODD" ? "evenodd" : "nonzero") + '" fill="' + paint.value + '" fill-opacity="' + paint.opacity + '"/>';
  } else if (op.type === "stroke") {
    const cap = op.cap === "ROUND" ? "round" : op.cap === "SQUARE" ? "square" : "butt";
    const join = op.join === "ROUND" ? "round" : op.join === "MITER" ? "miter" : "bevel";
    shape = '<path d="' + op.d + '" fill="none" stroke="' + paint.value + '" stroke-opacity="' + paint.opacity + '" stroke-width="' + op.width + '" stroke-linecap="' + cap + '" stroke-linejoin="' + join + '"/>';
  } else if (op.type === "ellipse") {
    const area = op.strokeArea || op.bounds;
    const geometry = 'cx="' + (area[0] + area[2] / 2) + '" cy="' + (area[1] + area[3] / 2) + '" rx="' + area[2] / 2 + '" ry="' + area[3] / 2 + '"';
    shape = op.strokeArea
      ? "<ellipse " + geometry + ' fill="none" stroke="' + paint.value + '" stroke-opacity="' + paint.opacity + '" stroke-width="' + op.width + '"/>'
      : "<ellipse " + geometry + ' fill="' + paint.value + '" fill-opacity="' + paint.opacity + '"/>';
  } else if (op.type === "rect") {
    const area = op.strokeArea || op.bounds;
    const geometry = 'x="' + area[0] + '" y="' + area[1] + '" width="' + area[2] + '" height="' + area[3] + '" rx="' + (op.radius || 0) + '"';
    shape = op.strokeArea
      ? "<rect " + geometry + ' fill="none" stroke="' + paint.value + '" stroke-opacity="' + paint.opacity + '" stroke-width="' + op.width + '"/>'
      : "<rect " + geometry + ' fill="' + paint.value + '" fill-opacity="' + paint.opacity + '"/>';
  }

  return { defs: defs.join(""), shape: shape };
}

function placeFromSvg(op, parent, origin, name) {
  const svg = svgShape(op);
  const width = Math.max(1, Math.ceil(op.bounds[0] + op.bounds[2] - origin[0]) + 2);
  const height = Math.max(1, Math.ceil(op.bounds[1] + op.bounds[3] - origin[1]) + 2);
  const markup = '<svg xmlns="http://www.w3.org/2000/svg" width="' + width + '" height="' + height + '" viewBox="0 0 ' + width + " " + height + '">'
    + (svg.defs ? "<defs>" + svg.defs + "</defs>" : "")
    + '<g transform="translate(' + (-origin[0]) + " " + (-origin[1]) + ')">' + svg.shape + "</g></svg>";
  const imported = figma.createNodeFromSvg(markup);
  const placed = [];

  for (const child of imported.children.slice()) {
    const x = child.x;
    const y = child.y;
    parent.appendChild(child);
    child.x = x;
    child.y = y;
    child.name = name;
    placed.push(child);
  }

  imported.remove();

  if (!isGradient(opPaint(op))) {
    for (const node of placed) {
      bindSvgPaints(node, opPaint(op).c);
    }
  }

  return placed;
}

function bindSvgPaints(node, hex) {
  if (!themeVariables[hex.toLowerCase()]) {
    return;
  }

  if ("fills" in node && Array.isArray(node.fills) && node.fills.length > 0) {
    node.fills = [solidPaint(hex)];
  }

  if ("strokes" in node && Array.isArray(node.strokes) && node.strokes.length > 0) {
    node.strokes = [solidPaint(hex)];
  }

  if ("children" in node) {
    for (const child of node.children) {
      bindSvgPaints(child, hex);
    }
  }
}

function relative(area, origin) {
  return { x: area[0] - origin[0], y: area[1] - origin[1], width: Math.max(0.01, area[2]), height: Math.max(0.01, area[3]) };
}

function nativeShape(op, parent, origin, name) {
  const area = op.strokeArea || op.bounds;
  const place = relative(area, origin);
  const node = op.type === "ellipse" ? figma.createEllipse() : figma.createRectangle();

  parent.appendChild(node);
  node.name = name;
  node.x = place.x;
  node.y = place.y;
  node.resize(place.width, place.height);

  if (op.type === "rect" && op.radius) {
    node.cornerRadius = op.radius;
  }

  if (op.strokeArea) {
    node.fills = [];
    node.strokes = [solidPaint(op.stroke.c)];
    node.strokeWeight = op.width;
    node.strokeAlign = "CENTER";
  } else {
    node.fills = [solidPaint(op.fill.c)];
    node.strokes = [];
  }

  return [node];
}

async function loadFont(family, style) {
  const key = family + "|" + style;

  if (fontCache[key] !== undefined) {
    return fontCache[key];
  }

  const candidates = [];

  if (FONT_MAP[key]) {
    candidates.push(FONT_MAP[key]);
  }

  candidates.push({ family: family, style: style }, { family: family, style: "Regular" }, FALLBACK_FONT, { family: "Inter", style: "Regular" });

  for (const candidate of candidates) {
    try {
      await figma.loadFontAsync(candidate);
      fontCache[key] = candidate;
      return candidate;
    } catch (error) {
      continue;
    }
  }

  fontCache[key] = null;
  return null;
}

function textFill(op) {
  return isGradient(op.fill) ? op.fill.stops[0][1] : op.fill.c;
}

function textNode(op, parent, origin, name) {
  const segments = op.segments || [op];
  const first = segments[0];
  const fontName = fontCache[first.family + "|" + first.style];

  if (!fontName) {
    return [];
  }

  const node = figma.createText();
  const characters = segments.map(function (segment) { return segment.text; }).join("");

  parent.appendChild(node);
  node.name = characters;
  node.fontName = fontName;
  node.fontSize = first.size;
  node.lineHeight = { unit: "PIXELS", value: first.ascent + first.descent };
  node.textAutoResize = "WIDTH_AND_HEIGHT";
  node.characters = characters;

  let start = 0;

  for (const segment of segments) {
    const end = start + segment.text.length;
    const segmentFont = fontCache[segment.family + "|" + segment.style] || fontName;

    if (end > start) {
      node.setRangeFontName(start, end, segmentFont);
      node.setRangeFontSize(start, end, segment.size);
      node.setRangeFills(start, end, [solidPaint(textFill(segment))]);
    }

    start = end;
  }

  node.x = first.x - origin[0];
  node.y = first.baseline - first.ascent - origin[1];

  if (segments.length === 1) {
    const difference = (first.endX - first.x) - node.width;

    if (characters.length > 0 && Math.abs(difference) > 0.25) {
      node.letterSpacing = { unit: "PIXELS", value: difference / characters.length };
    }

    rotateAboutBaseline(node, first, origin);

    return [node];
  }

  let rangeStart = 0;

  for (const segment of segments) {
    const length = segment.text.length;
    const probe = figma.createText();

    parent.appendChild(probe);
    probe.fontName = fontCache[segment.family + "|" + segment.style] || fontName;
    probe.fontSize = segment.size;
    probe.textAutoResize = "WIDTH_AND_HEIGHT";
    probe.characters = segment.text;

    const difference = (segment.endX - segment.x) - probe.width;

    probe.remove();

    if (length > 0 && Math.abs(difference) > 0.25) {
      node.setRangeLetterSpacing(rangeStart, rangeStart + length, { unit: "PIXELS", value: difference / length });
    }

    rangeStart += length;
  }

  return [node];
}

function rotateAboutBaseline(node, op, origin) {
  const angle = ((op.rotation || 0) * Math.PI) / 180;

  if (Math.abs(angle) < 1e-4) {
    return;
  }

  const cos = Math.cos(angle);
  const sin = Math.sin(angle);
  const baselineX = op.x - origin[0];
  const baselineY = op.baseline - origin[1];

  node.relativeTransform = [[cos, -sin, baselineX + sin * op.ascent], [sin, cos, baselineY - cos * op.ascent]];
}

function sameColour(a, b) {
  return !isGradient(a.fill) && !isGradient(b.fill) && a.fill.c === b.fill.c;
}

function mergeTextRuns(ops) {
  const merged = [];

  for (const op of ops) {
    const previous = merged[merged.length - 1];

    if (op.type === "text" && previous && previous.type === "text" && !op.clip && !previous.clip
        && Math.abs(op.rotation || 0) < 0.01 && Math.abs(previous.rotation || 0) < 0.01
        && Math.abs(op.baseline - previous.segments[previous.segments.length - 1].baseline) < 0.3
        && op.x - previous.segments[previous.segments.length - 1].endX > -1 && op.x - previous.segments[previous.segments.length - 1].endX < 1.5
        && sameColour(op, previous.segments[0])) {
      previous.segments.push(op);
      previous.endX = op.endX;
      previous.text = previous.segments.map(function (segment) { return segment.text; }).join("");
      continue;
    }

    if (op.type === "text") {
      merged.push(Object.assign({}, op, { segments: [op] }));
      continue;
    }

    merged.push(op);
  }

  return merged;
}

function imageNode(op, parent, origin, name) {
  if (!imageCache[op.image]) {
    imageCache[op.image] = figma.createImage(figma.base64Decode(DATA.images[op.image]));
  }

  const place = relative(op.bounds, origin);
  const node = figma.createRectangle();

  parent.appendChild(node);
  node.name = name;
  node.x = place.x;
  node.y = place.y;
  node.resize(place.width, place.height);
  node.fills = [{ type: "IMAGE", imageHash: imageCache[op.image].hash, scaleMode: "FILL" }];
  node.opacity = op.opacity === undefined ? 1 : op.opacity;

  return [node];
}

function describeOp(op) {
  if (op.type === "text") {
    return op.text;
  }

  if (op.type === "stroke") {
    return "Stroke";
  }

  if (op.type === "path") {
    return "Shape";
  }

  if (op.type === "ellipse") {
    return op.strokeArea ? "Ring" : "Disc";
  }

  if (op.type === "rect") {
    if (op.strokeArea) {
      return "Outline";
    }

    return op.radius ? "Rounded fill" : "Fill";
  }

  return op.type;
}

function drawOp(op, parent, origin) {
  const name = describeOp(op);

  if (op.type === "text") {
    return textNode(op, parent, origin, name);
  }

  if (op.type === "image") {
    return imageNode(op, parent, origin, name);
  }

  if (op.type === "path" || op.type === "stroke" || isGradient(opPaint(op))) {
    return placeFromSvg(op, parent, origin, name);
  }

  return nativeShape(op, parent, origin, name);
}

function clipContainer(op, parent, origin) {
  const frame = figma.createFrame();
  const place = relative(op.clip, origin);

  parent.appendChild(frame);
  frame.name = "Clip";
  frame.x = place.x;
  frame.y = place.y;
  frame.resize(place.width, place.height);
  frame.fills = [];
  frame.clipsContent = true;

  return { frame: frame, origin: [op.clip[0], op.clip[1]] };
}

function maskedOp(op, parent, origin) {
  const container = clipContainer(op, parent, origin);
  const masks = [];

  for (const clipPath of op.clipPaths) {
    const maskOp = { type: "path", d: clipPath.d, winding: clipPath.winding, fill: { type: "solid", c: "#000000ff" }, bounds: op.clip };
    masks.push.apply(masks, placeFromSvg(maskOp, container.frame, container.origin, "Mask"));
  }

  const drawn = drawOp(op, container.frame, container.origin);

  if (masks.length > 0 && drawn.length > 0) {
    const group = figma.group(masks.concat(drawn), container.frame);
    group.name = describeOp(op);
    masks.forEach(function (mask) { mask.isMask = true; });
  }

  return [container.frame];
}

function drawOps(rawOps, parent, origin) {
  const ops = mergeTextRuns(rawOps);
  const layerStack = [{ parent: parent, origin: origin }];
  let pendingFrost = null;

  for (const op of ops) {
    const target = layerStack[layerStack.length - 1];

    if (op.type === "beginLayer") {
      const frame = figma.createFrame();
      frame.name = "Transparency layer";
      target.parent.appendChild(frame);
      frame.x = 0;
      frame.y = 0;
      frame.resize(Math.max(0.01, parent.width), Math.max(0.01, parent.height));
      frame.fills = [];
      frame.clipsContent = false;
      frame.opacity = op.opacity;
      layerStack.push({ parent: frame, origin: target.origin });
      continue;
    }

    if (op.type === "endLayer") {
      if (layerStack.length > 1) {
        layerStack.pop();
      }
      continue;
    }

    if (op.type === "frost") {
      pendingFrost = op;
      continue;
    }

    let created;

    if (op.clipPaths && op.clipPaths.length > 0) {
      created = maskedOp(op, target.parent, target.origin);
    } else if (op.clip) {
      const container = clipContainer(op, target.parent, target.origin);
      created = drawOp(op, container.frame, container.origin);
      created = [container.frame];
    } else {
      created = drawOp(op, target.parent, target.origin);
    }

    if (pendingFrost && op.type === "rect" && !op.strokeArea && created.length > 0) {
      const node = created[0];
      node.name = "Frosted glass";
      node.effects = [{ type: "BACKGROUND_BLUR", radius: pendingFrost.radius * FROST_BLUR_SCALE, visible: true }];
      pendingFrost = null;
    }
  }
}

function frameName(component) {
  const className = shortClassName(component.class);
  return component.label ? className + " - " + component.label : className;
}

function buildComponent(component, parent, parentOrigin) {
  const frame = figma.createFrame();
  const bounds = component.bounds;
  const origin = [bounds[0], bounds[1]];

  parent.appendChild(frame);
  frame.name = frameName(component);
  frame.x = bounds[0] - parentOrigin[0];
  frame.y = bounds[1] - parentOrigin[1];
  frame.resize(Math.max(0.01, bounds[2]), Math.max(0.01, bounds[3]));
  frame.fills = [];
  frame.clipsContent = true;
  frame.opacity = component.alpha;

  drawOps(component.below, frame, origin);

  for (const child of component.children) {
    buildComponent(child, frame, origin);
  }

  drawOps(component.above, frame, origin);

  const className = shortClassName(component.class);

  if (!classExamples[className]) {
    classExamples[className] = frame;
  }

  return frame;
}

function buildScene(scene, page, x) {
  const frame = figma.createFrame();

  page.appendChild(frame);
  frame.name = scene.name;
  frame.x = x;
  frame.y = 0;
  frame.resize(scene.width, scene.height);
  frame.fills = hexToRgba(scene.background).a > 0 ? [solidPaint(scene.background)] : [];
  frame.clipsContent = true;

  if (scene.note) {
    frame.setPluginData("note", scene.note);
  }

  if (scene.root) {
    const root = buildComponent(scene.root, frame, [0, 0]);
    root.name = frameName(scene.root);
  }

  if (scene.reference && DATA.images[scene.reference]) {
    const reference = figma.createRectangle();
    if (!imageCache[scene.reference]) {
      imageCache[scene.reference] = figma.createImage(figma.base64Decode(DATA.images[scene.reference]));
    }
    frame.appendChild(reference);
    reference.name = "Reference - app render (show to compare)";
    reference.resize(scene.width, scene.height);
    reference.fills = [{ type: "IMAGE", imageHash: imageCache[scene.reference].hash, scaleMode: "FILL" }];
    reference.visible = false;
    reference.locked = true;
  }

  return frame;
}

async function buildThemeVariables() {
  const collection = figma.variables.createVariableCollection("SequenceTree Theme");
  const currentMode = collection.modes[0].modeId;
  const defaultMode = collection.addMode("Default theme");

  collection.renameMode(currentMode, "Current theme");

  for (const name of Object.keys(DATA.theme)) {
    const variable = figma.variables.createVariable(name, collection, "COLOR");
    const current = hexToRgba(DATA.theme[name]);
    const base = hexToRgba(DATA.defaultTheme[name]);

    variable.setValueForMode(currentMode, current);
    variable.setValueForMode(defaultMode, base);

    const key = DATA.theme[name].toLowerCase();

    if (!themeVariables[key]) {
      themeVariables[key] = variable;
    }
  }

  const sizes = figma.variables.createVariableCollection("SequenceTree Sizes");
  const sizeMode = sizes.modes[0].modeId;

  sizes.renameMode(sizeMode, "Theme");

  for (const name of Object.keys(DATA.numbers)) {
    const variable = figma.variables.createVariable(name, sizes, "FLOAT");
    variable.setValueForMode(sizeMode, DATA.numbers[name]);
  }
}

function collectFonts(component, fonts) {
  for (const op of component.below.concat(component.above)) {
    if (op.type === "text") {
      fonts[op.family + "|" + op.style] = [op.family, op.style];
    }
  }

  for (const child of component.children) {
    collectFonts(child, fonts);
  }
}

function buildCatalogue(page) {
  const names = Object.keys(classExamples).sort();
  const columnWidth = 360;
  const columns = 4;
  const rowHeights = [];
  let index = 0;
  let y = 0;

  for (const name of names) {
    const source = classExamples[name];
    const column = index % columns;

    if (column === 0 && index > 0) {
      y += rowHeights[rowHeights.length - 1] + CATALOGUE_GAP;
    }

    if (column === 0) {
      rowHeights.push(0);
    }

    const label = figma.createText();
    page.appendChild(label);
    label.fontName = fontCache["Geist|Regular"] || FALLBACK_FONT;
    label.fontSize = 12;
    label.characters = name;
    label.fills = [{ type: "SOLID", color: { r: 0.55, g: 0.58, b: 0.63 } }];
    label.x = column * columnWidth;
    label.y = y;

    const copy = source.clone();
    page.appendChild(copy);
    copy.name = source.name;
    copy.x = column * columnWidth;
    copy.y = y + 20;

    if (copy.width > columnWidth - 24) {
      copy.rescale((columnWidth - 24) / copy.width);
    }

    rowHeights[rowHeights.length - 1] = Math.max(rowHeights[rowHeights.length - 1], copy.height + 20);
    index += 1;
  }
}

async function main() {
  figma.notify("Building the SequenceTree mockup...", { timeout: 4000 });

  const fonts = {};

  for (const scene of DATA.scenes) {
    if (scene.root) {
      collectFonts(scene.root, fonts);
    }
  }

  fonts["Geist|Regular"] = ["Geist", "Regular"];

  for (const key of Object.keys(fonts)) {
    await loadFont(fonts[key][0], fonts[key][1]);
  }

  await buildThemeVariables();

  const pages = {};
  const firstPage = figma.root.children[0];
  let usedFirstPage = false;

  if ("loadAsync" in firstPage) {
    await firstPage.loadAsync();
  }

  for (const group of PAGE_ORDER) {
    const scenes = DATA.scenes.filter(function (scene) { return scene.group === group; });

    if (scenes.length === 0) {
      continue;
    }

    let page;

    if (!usedFirstPage && firstPage.children.length === 0) {
      page = firstPage;
      usedFirstPage = true;
    } else {
      page = figma.createPage();
    }

    page.name = group;
    pages[group] = page;

    if ("loadAsync" in page) {
      await page.loadAsync();
    }

    let x = 0;

    for (const scene of scenes) {
      const frame = buildScene(scene, page, x);
      x += frame.width + SCENE_GAP;
    }
  }

  const catalogue = figma.createPage();
  catalogue.name = "Components";
  buildCatalogue(catalogue);

  await figma.setCurrentPageAsync(pages["Main window"] || figma.root.children[0]);

  const missing = Object.keys(fontCache).filter(function (key) {
    return !fontCache[key] || (fontCache[key].family + "|" + fontCache[key].style) !== key && !FONT_MAP[key];
  });

  figma.closePlugin("SequenceTree mockup built: " + DATA.scenes.length + " frames" + (missing.length ? " (font substitutes for " + missing.join(", ") + ")" : ""));
}

main().catch(function (error) {
  figma.closePlugin("Mockup build failed: " + (error && error.message ? error.message : String(error)));
});

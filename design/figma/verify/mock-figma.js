const fs = require("fs");
const path = require("path");
const vm = require("vm");
const opentype = require("opentype.js");

const [codePath, outDir, fontDir] = process.argv.slice(2);

function loadFont(file) {
  const bytes = fs.readFileSync(path.join(fontDir, file));
  return opentype.parse(bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength));
}

const fonts = {
  "Geist|Regular": loadFont("Geist-Regular.ttf"),
  "Geist|SemiBold": loadFont("Geist-SemiBold.ttf"),
  "Geist Mono|Regular": loadFont("GeistMono-Regular.ttf")
};

const fontWeights = { Regular: 400, SemiBold: 600 };

let idCounter = 0;
const images = {};

function metrics(font, size) {
  const os2 = font.tables.os2;
  const useTypo = os2 && (os2.fsSelection & 128) !== 0;
  const upm = font.unitsPerEm;
  const ascender = useTypo ? os2.sTypoAscender : font.tables.hhea.ascender;
  const descender = useTypo ? os2.sTypoDescender : font.tables.hhea.descender;
  return { ascent: (ascender / upm) * size, descent: (-descender / upm) * size };
}

class MockNode {
  constructor(type) {
    this.type = type;
    this.id = String(++idCounter);
    this.children = [];
    this.parent = null;
    this.x = 0;
    this.y = 0;
    this.width = 100;
    this.height = 100;
    this.name = type;
    this.fills = [];
    this.strokes = [];
    this.strokeWeight = 1;
    this.strokeAlign = "INSIDE";
    this.effects = [];
    this.opacity = 1;
    this.visible = true;
    this.locked = false;
    this.isMask = false;
    this.clipsContent = false;
    this.cornerRadius = 0;
  }

  appendChild(child) {
    if (child.parent) {
      child.parent.children.splice(child.parent.children.indexOf(child), 1);
    }
    child.parent = this;
    this.children.push(child);
  }

  remove() {
    if (this.parent) {
      this.parent.children.splice(this.parent.children.indexOf(this), 1);
      this.parent = null;
    }
  }

  resize(width, height) {
    this.width = width;
    this.height = height;
  }

  rescale(scale) {
    this.width *= scale;
    this.height *= scale;
  }

  clone() {
    const copy = Object.assign(Object.create(Object.getPrototypeOf(this)), this);
    copy.id = String(++idCounter);
    copy.parent = null;
    copy.children = [];
    for (const child of this.children) {
      copy.appendChild(child.clone());
    }
    return copy;
  }

  setPluginData() {}
}

class MockText extends MockNode {
  constructor() {
    super("TEXT");
    this.fontName = { family: "Geist", style: "Regular" };
    this.fontSize = 12;
    this.lineHeight = { unit: "AUTO" };
    this.letterSpacing = { unit: "PIXELS", value: 0 };
    this.rangeFonts = [];
    this.rangeSizes = [];
    this.rangeFills = [];
    this._characters = "";
  }

  set characters(value) {
    this._characters = value;
    this.rangeFonts = Array.from(value).map(() => this.fontName);
    this.rangeSizes = Array.from(value).map(() => this.fontSize);
    this.rangeFills = Array.from(value).map(() => null);
    this.rangeSpacing = Array.from(value).map(() => null);
  }

  setRangeLetterSpacing(start, end, spacing) {
    for (let index = start; index < end; ++index) this.rangeSpacing[index] = spacing.value;
  }

  get characters() {
    return this._characters;
  }

  setRangeFontName(start, end, font) {
    for (let index = start; index < end; ++index) this.rangeFonts[index] = font;
  }

  setRangeFontSize(start, end, size) {
    for (let index = start; index < end; ++index) this.rangeSizes[index] = size;
  }

  setRangeFills(start, end, fills) {
    for (let index = start; index < end; ++index) this.rangeFills[index] = fills;
  }

  runs() {
    const characters = Array.from(this._characters);
    const runs = [];
    characters.forEach((character, index) => {
      const font = this.rangeFonts[index] || this.fontName;
      const size = this.rangeSizes[index] || this.fontSize;
      const fills = this.rangeFills[index] || this.fills;
      const spacing = this.rangeSpacing && this.rangeSpacing[index] !== null && this.rangeSpacing[index] !== undefined ? this.rangeSpacing[index] : null;
      const last = runs[runs.length - 1];
      const key = font.family + "|" + font.style;
      if (last && last.key === key && last.size === size && last.fills === fills && last.spacing === spacing) {
        last.text += character;
      } else {
        runs.push({ key, font, size, fills, spacing, text: character });
      }
    });
    return runs;
  }

  get width() {
    let total = 0;
    for (const run of this.runs()) {
      const font = fonts[run.key] || fonts["Geist|Regular"];
      const base = this.letterSpacing && this.letterSpacing.unit === "PIXELS" ? this.letterSpacing.value : 0;
      total += font.getAdvanceWidth(run.text, run.size, { kerning: true }) + (run.spacing !== null ? run.spacing : base) * Array.from(run.text).length;
    }
    return total;
  }

  set width(value) {}
}

function makeNode(type) {
  return new MockNode(type);
}

const rootPages = [makeNode("PAGE")];
const document = makeNode("DOCUMENT");
document.children = rootPages;
rootPages[0].parent = document;

const figma = {
  root: document,
  currentPage: rootPages[0],
  notify() {},
  closePlugin(message) {
    console.log("closePlugin:", message);
    figma.closed = true;
  },
  createFrame: () => makeNode("FRAME"),
  createRectangle: () => makeNode("RECTANGLE"),
  createEllipse: () => makeNode("ELLIPSE"),
  createText: () => new MockText(),
  createPage() {
    const page = makeNode("PAGE");
    document.appendChild(page);
    return page;
  },
  async setCurrentPageAsync(page) {
    figma.currentPage = page;
  },
  createNodeFromSvg(markup) {
    const frame = makeNode("FRAME");
    const inner = markup.replace(/^<svg[^>]*>/, "").replace(/<\/svg>$/, "");
    const shape = makeNode("SVGSHAPE");
    const suffix = "u" + ++idCounter;
    shape.markup = inner.replace(/id="g"/g, 'id="' + suffix + '"').replace(/url\(#g\)/g, "url(#" + suffix + ")");
    frame.appendChild(shape);
    return frame;
  },
  group(nodes, parent) {
    const group = makeNode("GROUP");
    parent.appendChild(group);
    for (const node of nodes) group.appendChild(node);
    return group;
  },
  createImage(bytes) {
    const hash = "h" + ++idCounter;
    images[hash] = Buffer.from(bytes).toString("base64");
    return { hash };
  },
  base64Decode: (text) => new Uint8Array(Buffer.from(text, "base64")),
  async loadFontAsync(font) {
    if (!fonts[font.family + "|" + font.style] && font.family !== "Inter") {
      throw new Error("font not available: " + font.family + " " + font.style);
    }
  },
  variables: {
    createVariableCollection(name) {
      return {
        name,
        modes: [{ modeId: "m0", name: "Mode 1" }],
        addMode(modeName) { this.modes.push({ modeId: "m" + this.modes.length, name: modeName }); return "m" + (this.modes.length - 1); },
        renameMode() {}
      };
    },
    createVariable(name, collection, type) {
      return { id: "var:" + name, name, type, values: {}, setValueForMode(mode, value) { this.values[mode] = value; } };
    },
    setBoundVariableForPaint(paint, field, variable) {
      return Object.assign({}, paint, { boundVariables: { color: { type: "VARIABLE_ALIAS", id: variable.id } } });
    }
  }
};

function paintAttributes(paints, kind) {
  const visible = (paints || []).filter((paint) => paint.visible !== false);
  if (visible.length === 0) return kind + '="none"';
  const paint = visible[0];
  if (paint.type === "SOLID") {
    const c = paint.color;
    const hex = "#" + [c.r, c.g, c.b].map((v) => Math.round(v * 255).toString(16).padStart(2, "0")).join("");
    return kind + '="' + hex + '" ' + kind + '-opacity="' + (paint.opacity === undefined ? 1 : paint.opacity) + '"';
  }
  return kind + '="#ff00ff"';
}

function imageFill(node) {
  const paint = (node.fills || []).find((fill) => fill.type === "IMAGE");
  if (!paint) return "";
  return '<image x="0" y="0" width="' + node.width + '" height="' + node.height + '" preserveAspectRatio="none" href="data:image/png;base64,' + images[paint.imageHash] + '"/>';
}

function strokeAttributes(node) {
  if (!node.strokes || node.strokes.length === 0) return "";
  return " " + paintAttributes(node.strokes, "stroke") + ' stroke-width="' + node.strokeWeight + '"';
}

function textSvg(node) {
  const runs = node.runs();
  if (runs.length === 0) return "";
  const first = runs[0];
  const firstFont = fonts[first.key] || fonts["Geist|Regular"];
  const lineHeight = node.lineHeight.unit === "PIXELS" ? node.lineHeight.value : null;
  const m = metrics(firstFont, first.size);
  const line = lineHeight === null ? m.ascent + m.descent : lineHeight;
  const baseline = node.y + (line - (m.ascent + m.descent)) / 2 + m.ascent;
  const spacing = node.letterSpacing && node.letterSpacing.unit === "PIXELS" ? node.letterSpacing.value : 0;
  const tspans = runs.map((run) => {
    const family = run.font.family === "Geist Mono" ? "'Geist Mono', 'Hiragino Sans'" : "'Geist', 'Hiragino Sans'";
    const weight = fontWeights[run.font.style] || 400;
    const text = run.text.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
    const runSpacing = run.spacing !== null ? ' letter-spacing="' + run.spacing + '"' : "";
    return '<tspan' + runSpacing + ' font-family="' + family + '" font-weight="' + weight + '" font-size="' + run.size + '" ' + paintAttributes(run.fills, "fill") + ">" + text + "</tspan>";
  }).join("");
  if (node.relativeTransform) {
    const t = node.relativeTransform;
    return '<g transform="matrix(' + [t[0][0], t[1][0], t[0][1], t[1][1], t[0][2], t[1][2]].join(" ") + ')"><text x="0" y="' + (baseline - node.y) + '" letter-spacing="' + spacing + '" xml:space="preserve" style="font-kerning:normal">' + tspans + "</text></g>";
  }
  return '<text x="' + node.x + '" y="' + baseline + '" letter-spacing="' + spacing + '" xml:space="preserve" style="font-kerning:normal">' + tspans + "</text>";
}

function nodeSvg(node) {
  if (!node.visible) return "";
  const opacity = node.opacity !== 1 ? ' opacity="' + node.opacity + '"' : "";
  switch (node.type) {
    case "FRAME": {
      let clip = "";
      let clipAttribute = "";
      if (node.clipsContent) {
        const id = "c" + ++idCounter;
        clip = '<clipPath id="' + id + '"><rect width="' + node.width + '" height="' + node.height + '"/></clipPath>';
        clipAttribute = ' clip-path="url(#' + id + ')"';
      }
      const fill = node.fills && node.fills.length ? '<rect width="' + node.width + '" height="' + node.height + '" ' + paintAttributes(node.fills, "fill") + "/>" : "";
      return '<g transform="translate(' + node.x + " " + node.y + ')"' + opacity + ">" + clip + "<g" + clipAttribute + ">" + fill + node.children.map(nodeSvg).join("") + "</g></g>";
    }
    case "GROUP": {
      const masks = node.children.filter((child) => child.isMask);
      const rest = node.children.filter((child) => !child.isMask);
      if (masks.length === 0) return "<g" + opacity + ">" + rest.map(nodeSvg).join("") + "</g>";
      const id = "m" + ++idCounter;
      const maskContent = masks.map(nodeSvg).join("").replace(/fill="#[0-9a-f]{6}"/g, 'fill="#ffffff"');
      return '<mask id="' + id + '" maskUnits="userSpaceOnUse" x="-10000" y="-10000" width="20000" height="20000">' + maskContent + '</mask><g mask="url(#' + id + ')"' + opacity + ">" + rest.map(nodeSvg).join("") + "</g>";
    }
    case "RECTANGLE": {
      const image = imageFill(node);
      if (image) return '<g transform="translate(' + node.x + " " + node.y + ')"' + opacity + ">" + image + "</g>";
      return '<rect x="' + node.x + '" y="' + node.y + '" width="' + node.width + '" height="' + node.height + '" rx="' + (node.cornerRadius || 0) + '" ' + paintAttributes(node.fills, "fill") + strokeAttributes(node) + opacity + "/>";
    }
    case "ELLIPSE":
      return '<ellipse cx="' + (node.x + node.width / 2) + '" cy="' + (node.y + node.height / 2) + '" rx="' + node.width / 2 + '" ry="' + node.height / 2 + '" ' + paintAttributes(node.fills, "fill") + strokeAttributes(node) + opacity + "/>";
    case "SVGSHAPE":
      return '<g transform="translate(' + node.x + " " + node.y + ')"' + opacity + ">" + node.markup + "</g>";
    case "TEXT":
      return "<g" + opacity + ">" + textSvg(node) + "</g>";
    default:
      return node.children.map(nodeSvg).join("");
  }
}

function fontFaces() {
  const face = (family, file, weight) => "@font-face{font-family:'" + family + "';src:url('" + path.join(fontDir, file) + "');font-weight:" + weight + ";}";
  return face("Geist", "Geist-Regular.ttf", 400) + face("Geist", "Geist-SemiBold.ttf", 600) + face("Geist Mono", "GeistMono-Regular.ttf", 400);
}

function countNodes(node, counts) {
  counts[node.type] = (counts[node.type] || 0) + 1;
  node.children.forEach((child) => countNodes(child, counts));
  return counts;
}

(async () => {
  const code = fs.readFileSync(codePath, "utf8");
  const context = vm.createContext({ figma, console, Math, Object, Array, String, Number, JSON, parseInt, Error, Promise });
  vm.runInContext(code, context);

  while (!figma.closed) {
    await new Promise((resolve) => setTimeout(resolve, 50));
  }

  fs.mkdirSync(outDir, { recursive: true });
  const manifest = [];

  for (const page of document.children) {
    page.children.filter((node) => node.type === "FRAME").forEach((frame, index) => {
      const file = (page.name + "-" + index).replace(/[^A-Za-z0-9-]+/g, "_") + ".html";
      const svg = '<svg xmlns="http://www.w3.org/2000/svg" width="' + frame.width + '" height="' + frame.height + '">' + nodeSvg(Object.assign(Object.create(Object.getPrototypeOf(frame)), frame, { x: 0, y: 0 })) + "</svg>";
      fs.writeFileSync(path.join(outDir, file), "<!doctype html><html><head><style>" + fontFaces() + "html,body{margin:0;background:transparent}</style></head><body>" + svg + "</body></html>");
      manifest.push({ page: page.name, name: frame.name, file, width: frame.width, height: frame.height, counts: countNodes(frame, {}) });
    });
  }

  fs.writeFileSync(path.join(outDir, "manifest.json"), JSON.stringify(manifest, null, 1));
  console.log("pages:", document.children.map((page) => page.name + " (" + page.children.length + ")").join(", "));
})();

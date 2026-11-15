import base64
import io
import json
import os
import subprocess
import sys

from PIL import Image, ImageChops, ImageStat

here = os.path.dirname(os.path.abspath(__file__))
figma = os.path.dirname(here)
repo = os.path.dirname(os.path.dirname(figma))
scene_path = os.path.join(repo, "cmake-build-debug", "FigmaScene.json")
fonts = os.path.join(repo, "ArsNovaUI", "Resources", "Fonts")
out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(repo, "cmake-build-debug", "FigmaVerify")

scene = json.load(open(scene_path))
mock_dir = os.path.join(out, "mock")
os.makedirs(mock_dir, exist_ok=True)

result = subprocess.run(["node", os.path.join(here, "mock-figma.js"), os.path.join(figma, "plugin", "code.js"), mock_dir, fonts],
                        capture_output=True, text=True)
print(result.stdout.strip() or result.stderr.strip())

manifest = {(entry["page"], entry["name"]): entry for entry in json.load(open(os.path.join(mock_dir, "manifest.json")))}
worst = 0.0

for index, frame in enumerate(scene["scenes"]):
    entry = manifest.get((frame["group"], frame["name"]))

    if entry is None:
        print(f"{index:02d} {frame['name']}: MISSING from the plugin output")
        worst = 100.0
        continue

    rendered = os.path.join(out, f"{index:02d}_plugin.png")
    subprocess.run([os.path.join(here, "render.sh"), os.path.join(mock_dir, entry["file"]), rendered, str(frame["width"]), str(frame["height"])])

    reference = Image.open(io.BytesIO(base64.b64decode(scene["images"][frame["reference"]]))).convert("RGBA")
    plugin = Image.open(rendered).convert("RGBA").resize(reference.size)
    backdrop = Image.new("RGBA", reference.size, (40, 40, 40, 255))
    app = Image.alpha_composite(backdrop, reference).convert("RGB")
    built = Image.alpha_composite(backdrop, plugin).convert("RGB")
    difference = ImageChops.difference(app, built).convert("L")
    histogram = difference.histogram()
    changed = 100.0 * sum(histogram[25:]) / sum(histogram)
    worst = max(worst, changed)

    print(f"{index:02d} {frame['name']:<36} mean {ImageStat.Stat(difference).mean[0]:5.2f}   pixels off by >24/255: {changed:5.2f}%")

    side = Image.new("RGB", (app.width, app.height * 2 + 4), (255, 0, 0))
    side.paste(app, (0, 0))
    side.paste(built, (0, app.height + 4))
    side.save(os.path.join(out, f"{index:02d}_app_above_plugin_below.png"))

print(f"worst frame: {worst:.2f}% of pixels off; comparisons in {out}")

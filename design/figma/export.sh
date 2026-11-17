#!/usr/bin/env bash
set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
repo="$(cd "$here/../.." && pwd)"
build="$repo/cmake-build-debug"
width="${1:-1580}"
height="${2:-805}"
exporter="$build/FigmaExporter"
scene="$build/FigmaScene.json"

cmake --build "$build" --target SequenceTree_Standalone

flags=$(python3 - "$build" "$repo" <<'PY'
import json, shlex, sys
build, repo = sys.argv[1], sys.argv[2]
for entry in json.load(open(f"{build}/compile_commands.json")):
    if entry["file"].endswith("Source/Editor/PluginEditor.cpp"):
        args = shlex.split(entry["command"])[1:]
        kept, skip = [], False
        for arg in args:
            if skip:
                skip = False
                continue
            if arg in ("-o", "-c"):
                skip = True
                continue
            if arg.endswith(".cpp"):
                continue
            kept.append(arg)
        kept += [f"-I{repo}/Source", "-Wno-keyword-macro", "-Wno-unused-function", "-Wno-unused-variable"]
        print(" ".join(shlex.quote(arg) for arg in kept))
        break
PY
)

eval /usr/bin/c++ $flags -c "$here/exporter/FigmaExporter.cpp" -o "$build/FigmaExporter.o"

/usr/bin/c++ -arch arm64 -Wl,-search_paths_first "$build/FigmaExporter.o" -o "$exporter" \
    "$build/SequenceTree_artefacts/Debug/libSequenceTree_SharedCode.a" \
    "$build/ArsNovaUI/libArsNovaUIFonts.a" "$build/ArsNovaUI/libArsNovaUIIcons.a" \
    -framework CoreAudioKit -framework DiscRecording -framework Accelerate -framework CoreAudio -framework CoreMIDI \
    -framework AudioToolbox -framework Foundation -framework IOKit -framework Security -framework Cocoa -framework QuartzCore \
    -weak_framework Metal -weak_framework MetalKit -framework WebKit

"$exporter" "$scene" "$width" "$height"

python3 - "$here/plugin/code.template.js" "$scene" "$here/plugin/code.js" <<'PY'
import sys
template, scene, out = sys.argv[1:]
open(out, "w").write(open(template).read().replace("__SCENE_DATA__", open(scene).read(), 1))
PY

echo "Figma plugin ready: $here/plugin/manifest.json"

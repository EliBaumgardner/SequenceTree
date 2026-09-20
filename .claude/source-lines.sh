#!/usr/bin/env bash
set -uo pipefail

root="${CLAUDE_PROJECT_DIR:-$(git rev-parse --show-toplevel 2>/dev/null)}"
dir="$root/Source"

[ -d "$dir" ] || exit 0

files=$(find "$dir" -type f \( -name '*.cpp' -o -name '*.h' \) | wc -l | tr -d ' ')
lines=$(find "$dir" -type f \( -name '*.cpp' -o -name '*.h' \) -exec cat {} + | wc -l | tr -d ' ')

printf '{"systemMessage":"Source/: %s lines across %s files","suppressOutput":true}\n' "$lines" "$files"

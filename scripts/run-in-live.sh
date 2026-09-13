#!/usr/bin/env bash
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$REPO/cmake-build-debug}"
CONFIG="${CONFIG:-Debug}"
TARGET="${TARGET:-SequenceTree_VST3}"
LIVE_APP="${LIVE_APP:-/Applications/Ableton Live 12 Suite.app}"
LIVE_SET="${LIVE_SET:-$HOME/Documents/Ableton Projects/SequenceTree Dev/SequenceTree Dev Project/SequenceTree Dev.als}"

LIVE_NAME="$(basename "$LIVE_APP" .app)"

if pgrep -x Live >/dev/null; then
    echo "quitting $LIVE_NAME"
    osascript -e "tell application \"$LIVE_NAME\" to quit saving no" >/dev/null 2>&1 || true
    for _ in $(seq 1 40); do
        pgrep -x Live >/dev/null || break
        sleep 0.25
    done
    if pgrep -x Live >/dev/null; then
        echo "Live did not quit (unsaved-changes dialog?) - aborting so the build does not clobber a loaded binary" >&2
        exit 1
    fi
fi

cmake --build "$BUILD_DIR" --config "$CONFIG" --target "$TARGET"

if [ -f "$LIVE_SET" ]; then
    open -a "$LIVE_APP" "$LIVE_SET"
else
    echo "no set at '$LIVE_SET' - opening Live with an empty set"
    open -a "$LIVE_APP"
fi

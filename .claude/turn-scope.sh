#!/usr/bin/env bash
set -uo pipefail

root="${CLAUDE_PROJECT_DIR:-$(git rev-parse --show-toplevel 2>/dev/null || pwd)}"
cd "$root" 2>/dev/null || exit 0

mode="${1:-}"
session="${2:-unknown}"
session="${session//[^A-Za-z0-9_-]/_}"

snap="${TMPDIR:-/tmp}/claude-turnsnap-${session}"
stamp="$snap/.captured"

snapshot() {
    rm -rf "$snap"
    mkdir -p "$snap/Source" || return 0
    if [ -d Source ]; then
        find Source -type f \( -name '*.cpp' -o -name '*.h' \) -print0 \
            | while IFS= read -r -d '' f; do
                mkdir -p "$snap/$(dirname "$f")"
                cp "$f" "$snap/$f"
            done
    fi
    date +%s > "$stamp"
}

raw_diff() {
    if [ ! -f "$stamp" ]; then
        git diff HEAD -- 'Source/*.cpp' 'Source/*.h' 2>/dev/null
        git ls-files --others --exclude-standard -- 'Source/*.cpp' 'Source/*.h' 2>/dev/null \
            | sed 's/^/new untracked file: /'
        return
    fi
    { find Source -type f \( -name '*.cpp' -o -name '*.h' \) 2>/dev/null
      if [ -d "$snap/Source" ]; then
          find "$snap/Source" -type f \( -name '*.cpp' -o -name '*.h' \) 2>/dev/null \
              | sed "s#^$snap/##"
      fi
    } | sort -u | while IFS= read -r f; do
        [ -n "$f" ] || continue
        old="$snap/$f"
        new="$f"
        [ -f "$old" ] || old=/dev/null
        [ -f "$new" ] || new=/dev/null
        [ "$old" = /dev/null ] && [ "$new" = /dev/null ] && continue
        git --no-pager diff --no-index --no-color --unified="${unified:-3}" \
            --src-prefix="a/" --dst-prefix="b/" -- "$old" "$new" 2>/dev/null \
            | awk -v F="$f" '
                /^diff --git / { print "diff --git a/" F " b/" F; next }
                /^--- / { if ($2 == "/dev/null") { print } else { print "--- a/" F } ; next }
                /^\+\+\+ / { if ($2 == "/dev/null") { print } else { print "+++ b/" F } ; next }
                { print }
            '
    done
}

case "$mode" in
    --snapshot)
        snapshot
        ;;
    --captured)
        if [ -f "$stamp" ]; then
            echo yes
        else
            echo no
        fi
        ;;
    --diff)
        raw_diff
        ;;
    --files)
        raw_diff | awk '
            /^\+\+\+ b\// { f = substr($0, 7); if (f != "/dev/null") { print f } }
            /^new untracked file: / { print substr($0, 20) }
        ' | sort -u
        ;;
    --added)
        unified=0 raw_diff | awk '
            /^\+\+\+ b\// { F = substr($0, 7); next }
            /^@@/ {
                if (F != "" && F != "/dev/null" && match($0, /\+[0-9]+(,[0-9]+)?/)) {
                    spec = substr($0, RSTART + 1, RLENGTH - 1)
                    n = split(spec, a, ",")
                    start = a[1] + 0
                    count = 1
                    if (n > 1) { count = a[2] + 0 }
                    for (i = 0; i < count; i++) { print F ":" (start + i) }
                }
            }
        '
        ;;
    *)
        echo "usage: turn-scope.sh --snapshot|--captured|--diff|--files|--added <session>"
        ;;
esac

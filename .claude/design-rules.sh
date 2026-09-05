#!/usr/bin/env bash
set -uo pipefail

root="${CLAUDE_PROJECT_DIR:-$(git rev-parse --show-toplevel 2>/dev/null || pwd)}"
cd "$root" 2>/dev/null || exit 0

if [ "${1:-}" = "--all" ]; then
    files=$(find Source -type f \( -name '*.cpp' -o -name '*.h' \) | sort)
    scope="all Source files"
else
    files=$( { git diff --name-only HEAD -- 'Source/*.cpp' 'Source/*.h' 2>/dev/null
               git ls-files --others --exclude-standard -- 'Source/*.cpp' 'Source/*.h' 2>/dev/null; } \
             | sort -u )
    scope="files changed vs HEAD"
fi

if [ -z "$files" ]; then
    echo "design-rules: no changed Source files to check"
    exit 0
fi

violations=0

emit() {
    local rule="$1"
    local hits="$2"
    if [ -n "$hits" ]; then
        violations=$((violations + 1))
        printf '\n[VIOLATION] %s\n%s\n' "$rule" "$hits"
    fi
}

inline_hits=$(echo "$files" | xargs grep -HnE '(^|[[:space:]])inline[[:space:]]' 2>/dev/null \
    | grep -F '(' | grep -v 'constexpr')

ternary_hits=$(echo "$files" | xargs awk '
    { line = $0
      gsub(/::/, "@@", line)
      gsub(/"[^"]*"/, "@@", line)
      gsub(/\/\/.*$/, "", line)
      if (line ~ /\?[^:]*:/) printf "%s:%d:%s\n", FILENAME, FNR, $0 }
' 2>/dev/null)

comment_hits=$(echo "$files" | xargs awk '
    FNR == 1 { banner = 1 }
    banner && /^\s*(\/\*|\*|\*\/|\s*$)/ { if (/\*\//) banner = 0; next }
    { banner = 0 }
    /#endif[[:space:]]*\/\// { next }
    /\/\/[[:space:]]*Created by/ { next }
    /\/\/=+/ { next }
    /(^|[^:"\/])\/\/|\/\*/ { printf "%s:%d:%s\n", FILENAME, FNR, $0 }
' 2>/dev/null)

brace_hits=$(echo "$files" | xargs awk '
    function balanced(s,   i, c, depth) {
        depth = 0
        for (i = 1; i <= length(s); i++) {
            c = substr(s, i, 1)
            if (c == "(") depth++
            if (c == ")") depth--
        }
        return depth == 0
    }
    prev != "" {
        if ($0 !~ /^[[:space:]]*\{/) printf "%s:%d:%s\n", FILENAME, prevline, prev
        prev = ""
    }
    /^[[:space:]]*(if|for|while)[[:space:]]*\(.*\)[[:space:]]*$/ {
        if (balanced($0)) { prev = $0; prevline = FNR; next }
    }
    /^[[:space:]]*else[[:space:]]*$/ { prev = $0; prevline = FNR; next }
    { prev = "" }
' 2>/dev/null)

echo "design-rules: checking $scope"
echo "$files" | sed 's/^/  /'

emit "Never use inline functions"            "$inline_hits"
emit "Never use ternary operators"           "$ternary_hits"
emit "This project uses no code comments"    "$comment_hits"
emit "Always use {} for blocks"              "$brace_hits"

if [ "$violations" -eq 0 ]; then
    echo
    echo "design-rules: clean"
    exit 0
fi

echo
echo "design-rules: $violations rule(s) violated"
exit 1

#!/usr/bin/env bash
set -uo pipefail

root="${CLAUDE_PROJECT_DIR:-$(git rev-parse --show-toplevel 2>/dev/null || pwd)}"
cd "$root" 2>/dev/null || exit 0

mode="diff"
target=""

case "${1:-}" in
    --all)
        mode="all"
        ;;
    --file)
        mode="file"
        target="${2:-}"
        ;;
    --hook)
        mode="${2:-}"
        ;;
esac

payload=""
if [ "$mode" = "post-tool" ] || [ "$mode" = "stop" ]; then
    payload=$(cat)
fi

if [ "$mode" = "stop" ]; then
    active=$(printf '%s' "$payload" | jq -r '.stop_hook_active // false' 2>/dev/null)
    if [ "$active" = "true" ]; then
        exit 0
    fi
fi

if [ "$mode" = "post-tool" ]; then
    target=$(printf '%s' "$payload" | jq -r '.tool_input.file_path // ""' 2>/dev/null)
    if [ -z "$target" ]; then
        exit 0
    fi
    target="${target#"$root"/}"
    case "$target" in
        Source/*.cpp|Source/*.h) ;;
        *) exit 0 ;;
    esac
fi

case "$mode" in
    all)
        files=$(find Source -type f \( -name '*.cpp' -o -name '*.h' \) | sort)
        scope="all Source files"
        ;;
    file|post-tool)
        if [ ! -f "$target" ]; then
            exit 0
        fi
        files="$target"
        scope="$target"
        ;;
    *)
        files=$( { git diff --name-only HEAD -- 'Source/*.cpp' 'Source/*.h' 2>/dev/null
                   git ls-files --others --exclude-standard -- 'Source/*.cpp' 'Source/*.h' 2>/dev/null; } \
                 | sort -u )
        scope="files changed vs HEAD"
        ;;
esac

if [ -z "$files" ]; then
    if [ "$mode" = "post-tool" ] || [ "$mode" = "stop" ]; then
        exit 0
    fi
    echo "design-rules: no changed Source files to check"
    exit 0
fi

violations=0
report=""

emit() {
    local rule="$1"
    local hits="$2"
    if [ -n "$hits" ]; then
        violations=$((violations + 1))
        report="${report}"$'\n'"[VIOLATION] ${rule}"$'\n'"${hits}"$'\n'
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
    banner {
        if ($0 ~ /^[[:space:]]*$/ || $0 ~ /^[[:space:]]*(\/\/|\/\*|\*)/) { next }
        banner = 0
    }
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

emit "Never use inline functions"            "$inline_hits"
emit "Never use ternary operators"           "$ternary_hits"
emit "This project uses no code comments"    "$comment_hits"
emit "Always use {} for blocks"              "$brace_hits"

case "$mode" in
    post-tool)
        if [ "$violations" -eq 0 ]; then
            exit 0
        fi
        context="Design-rule violations in ${target} (CLAUDE.md Key Design Rules)."$'\n'
        context="${context}Fix the ones this edit introduced now, in this turn, before moving on."$'\n'
        context="${context}If a hit is pre-existing code you only touched, leave it and say so."$'\n'
        context="${context}${report}"
        jq -n --arg ctx "$context" \
          '{hookSpecificOutput: {hookEventName: "PostToolUse", additionalContext: $ctx}}'
        exit 0
        ;;
    stop)
        if [ "$violations" -eq 0 ]; then
            exit 0
        fi
        reason="Design-rule violations remain in files changed vs HEAD (CLAUDE.md Key Design Rules)."$'\n'
        reason="${reason}Fix the ones you introduced this session."$'\n'
        reason="${reason}For any hit that is pre-existing code you only touched, leave it and report it to the user."$'\n'
        reason="${reason}${report}"
        jq -n --arg r "$reason" '{decision: "block", reason: $r}'
        exit 0
        ;;
    *)
        echo "design-rules: checking $scope"
        echo "$files" | sed 's/^/  /'
        printf '%s' "$report"
        if [ "$violations" -eq 0 ]; then
            echo
            echo "design-rules: clean"
            exit 0
        fi
        echo
        echo "design-rules: $violations rule(s) violated"
        exit 1
        ;;
esac

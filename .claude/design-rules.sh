#!/usr/bin/env bash
set -uo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
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

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

addedmap="$work/added"
: > "$addedmap"

git diff -U0 -M HEAD -- 'Source/*.cpp' 'Source/*.h' 2>/dev/null | awk '
    /^\+\+\+ b\// { F = substr($0, 7); next }
    /^@@/ {
        if (F != "" && match($0, /\+[0-9]+(,[0-9]+)?/)) {
            spec = substr($0, RSTART + 1, RLENGTH - 1)
            n = split(spec, a, ",")
            start = a[1] + 0
            count = 1
            if (n > 1) { count = a[2] + 0 }
            for (i = 0; i < count; i++) { print F ":" (start + i) }
        }
    }
' >> "$addedmap"

for f in $files; do
    if ! git ls-files --error-unmatch "$f" >/dev/null 2>&1; then
        awk -v F="$f" '{ print F ":" FNR }' "$f" >> "$addedmap"
    fi
done

scope_added=1
if [ "$mode" = "all" ]; then
    scope_added=0
fi

only_added() {
    if [ "$scope_added" -eq 0 ]; then
        cat
        return
    fi
    awk -F: -v MAP="$addedmap" '
        BEGIN { while ((getline l < MAP) > 0) { keep[l] = 1 } }
        NF >= 2 { key = $1 ":" $2; if (key in keep) { print } }
    '
}

funcs="$work/funcs"
echo "$files" | tr ' ' '\n' | grep -v '^$' \
    | xargs awk -v MINBLOCK=999999 -f "$here/cxx-scan.awk" 2>/dev/null \
    | grep '^F' > "$funcs"

members="$work/members"
echo "$files" | tr ' ' '\n' | grep -E '\.h$' | xargs awk '
    FNR == 1 { access = ""; curclass = ""; depth = 0 }
    {
        line = $0
        sub(/\/\/.*$/, "", line)

        if (line ~ /^[[:space:]]*(class|struct)[[:space:]]+[A-Za-z_]/ && line !~ /;[[:space:]]*$/) {
            c = line
            sub(/^[[:space:]]*(class|struct)[[:space:]]+/, "", c)
            if (match(c, /^[A-Za-z_][A-Za-z0-9_]*/)) { curclass = substr(c, RSTART, RLENGTH) }
            access = "private"
            if (line ~ /^[[:space:]]*struct/) { access = "public" }
            next
        }
        if (line ~ /^[[:space:]]*public[[:space:]]*:/)    { access = "public";    next }
        if (line ~ /^[[:space:]]*protected[[:space:]]*:/) { access = "protected"; next }
        if (line ~ /^[[:space:]]*private[[:space:]]*:/)   { access = "private";   next }

        if (curclass == "" || access == "" || access == "public") { next }
        if (line !~ /;[[:space:]]*$/ || line ~ /\(/) { next }
        if (line ~ /^[[:space:]]*(using|typedef|friend|template|#|\})/) { next }

        n = line
        sub(/[[:space:]]*=.*$/, "", n)
        sub(/;[[:space:]]*$/, "", n)
        sub(/\[[^]]*\][[:space:]]*$/, "", n)
        if (n !~ /[[:space:]]/) { next }
        if (match(n, /[A-Za-z_][A-Za-z0-9_]*[[:space:]]*$/)) {
            nm = substr(n, RSTART, RLENGTH)
            gsub(/[[:space:]]/, "", nm)
            print "M\t" curclass "\t" nm "\t" FILENAME "\t" FNR "\t" access
        }
    }
' 2>/dev/null > "$members"

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
    | grep -F '(' | grep -v 'constexpr' | only_added)

ternary_hits=$(echo "$files" | xargs awk '
    { line = $0
      gsub(/::/, "@@", line)
      gsub(/"[^"]*"/, "@@", line)
      gsub(/\/\/.*$/, "", line)
      if (line ~ /\?[^:]*:/) printf "%s:%d:%s\n", FILENAME, FNR, $0 }
' 2>/dev/null | only_added)

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
' 2>/dev/null | only_added)

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
' 2>/dev/null | only_added)

tiny_hits=$(awk -F'\t' '
    $1 == "F" && $7 == "function" && $6 == 0 && $5 <= 2 && $5 > 0 {
        printf "%s:%d:%s() body is %d statement(s)\n", $2, $3, $8, $5
    }
' "$funcs" | only_added)

accessor_hits=$(awk -F'\t' '
    FILENAME == ARGV[1] && $1 == "M" { owner[$2 "\t" $3] = $4 ":" $5; vis[$2 "\t" $3] = $6; next }
    $1 == "F" && $10 != "" && $9 != "" {
        key = $9 "\t" $10
        if (key in owner) {
            printf "%s:%d:%s() only hands out %s member %s (declared %s) - move the member to public scope\n",
                   $2, $3, $8, vis[key], $10, owner[key]
        }
    }
' "$members" "$funcs" | only_added)

audiofiles=$(echo "$files" | tr ' ' '\n' | grep -E '^Source/Audio/' || true)

alloc_hits=""
uimutate_hits=""
boundary_hits=""

if [ -n "$audiofiles" ]; then
    alloc_hits=$(echo "$audiofiles" | xargs awk -F'\t' '
        FNR == NR { if ($1 == "F") { for (l = $3; l <= $4; l++) { fn[$2 ":" l] = $8 } } ; next }
        {
            here = fn[FILENAME ":" FNR]
            if (here == "") { next }
            if (here ~ /^(prepare|prepareToPlay|reserve|setup|releaseResources)/) { next }
            line = $0
            sub(/\/\/.*$/, "", line)
            if (line ~ /(^|[^A-Za-z0-9_])(new|delete)[[:space:]]/ \
             || line ~ /make_unique|make_shared|malloc\(|calloc\(|realloc\(|free\(/ \
             || line ~ /(^|[^A-Za-z0-9_:])(std::)?(string|vector|map|set)[[:space:]]*<[^>]*>[[:space:]]+[A-Za-z_]/ \
             || line ~ /juce::String[[:space:]]+[A-Za-z_]/) {
                printf "%s:%d:%s\n", FILENAME, FNR, $0
            }
        }
    ' "$funcs" 2>/dev/null | only_added)

    uimutate_hits=$(echo "$audiofiles" | xargs awk '
        { line = $0
          sub(/\/\/.*$/, "", line)
          if (line ~ /juce::Component|NodeCanvas|->repaint\(|\.repaint\(|setBounds\(|setVisible\(|LookAndFeel/) {
              printf "%s:%d:%s\n", FILENAME, FNR, $0
          } }
    ' 2>/dev/null | only_added)

    boundary_hits=$(echo "$audiofiles" | xargs awk '
        { line = $0
          sub(/\/\/.*$/, "", line)
          if (line ~ /juce::ValueTree|GraphState|ValueTreeIdentifiers/) {
              printf "%s:%d:%s\n", FILENAME, FNR, $0
          } }
    ' 2>/dev/null | only_added)
fi

staticctx_hits=$(echo "$files" | xargs grep -HnE '^[[:space:]]*(static|extern)?[[:space:]]*[A-Za-z_][A-Za-z0-9_:<>]*[[:space:]]+[A-Za-z_][A-Za-z0-9_]*[[:space:]]*=[^=]*ApplicationContext' 2>/dev/null | only_added)

emit "Never use inline functions"                                        "$inline_hits"
emit "Never use ternary operators"                                       "$ternary_hits"
emit "This project uses no code comments"                                "$comment_hits"
emit "Always use {} for blocks"                                          "$brace_hits"
emit "Never write 1-2 line functions, wrappers, or single-operation functions" "$tiny_hits"
emit "Anything that needs a getter belongs in public scope"              "$accessor_hits"
emit "Never allocate or free on the audio thread"                        "$alloc_hits"
emit "Never touch UI components from the audio thread"                   "$uimutate_hits"
emit "Only RTData structs and RTScript cross the audio boundary"         "$boundary_hits"
emit "ApplicationContext is not valid at static init time"               "$staticctx_hits"

checked="inline, ternaries, comments, braces, tiny/wrapper functions, non-public members with accessors, audio-thread allocation, audio-thread UI access, audio boundary types, static-init ApplicationContext"
unchecked="Code fits the class's intended purpose / single area of concern
Avoid encapsulation on very small segments of code which repeat
Whether a push_back stays inside its reserved capacity (the allocation check cannot see capacity)
Per-block scratch state lives in reserved member vectors, not locals
Prefer declaring an unused variable over deleting one that represents the class's functionality
Whether a short function qualifies for the \"states a larger process\" exception - ASK, do not self-certify"

coverage="design-rules: machine-checked - ${checked}"$'\n'
coverage="${coverage}design-rules: NOT machine-checked - judge these yourself, a pass here is not a pass on them:"$'\n'
coverage="${coverage}$(printf '%s\n' "$unchecked" | sed 's/^/  /')"

case "$mode" in
    post-tool)
        if [ "$violations" -eq 0 ]; then
            exit 0
        fi
        context="Design-rule violations in ${target} (CLAUDE.md Key Design Rules)."$'\n'
        context="${context}Fix the ones this edit introduced now, in this turn, before moving on."$'\n'
        context="${context}If a hit is pre-existing code you only touched, leave it and say so."$'\n'
        context="${context}${report}"$'\n'"${coverage}"
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
        reason="${reason}${report}"$'\n'"${coverage}"
        jq -n --arg r "$reason" '{decision: "block", reason: $r}'
        exit 0
        ;;
    *)
        echo "design-rules: checking $scope"
        echo "$files" | sed 's/^/  /'
        printf '%s' "$report"
        echo
        if [ "$violations" -eq 0 ]; then
            echo "design-rules: no violations of the machine-checked rules"
        else
            echo "design-rules: $violations rule(s) violated"
        fi
        echo "$coverage"
        if [ "$violations" -eq 0 ]; then
            exit 0
        fi
        exit 1
        ;;
esac

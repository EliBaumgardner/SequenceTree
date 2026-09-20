#!/usr/bin/env bash
set -uo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="${CLAUDE_PROJECT_DIR:-$(git rev-parse --show-toplevel 2>/dev/null || pwd)}"
cd "$root" 2>/dev/null || exit 0

maxfunc=80
minsplit=30

mode="turn"
target=""
session="unknown"

case "${1:-}" in
    --all)  mode="all" ;;
    --file) mode="file"; target="${2:-}" ;;
    --turn) mode="turn"; session="${2:-unknown}" ;;
    --hook) mode="${2:-}" ;;
esac

payload=""
if [ "$mode" = "post-tool" ] || [ "$mode" = "stop" ]; then
    payload=$(cat)
    session=$(printf '%s' "$payload" | jq -r '.session_id // "unknown"' 2>/dev/null)
fi

if [ "$mode" = "stop" ]; then
    active=$(printf '%s' "$payload" | jq -r '.stop_hook_active // false' 2>/dev/null)
    if [ "$active" = "true" ]; then
        exit 0
    fi
fi

if [ "$mode" = "post-tool" ]; then
    target=$(printf '%s' "$payload" | jq -r '.tool_input.file_path // ""' 2>/dev/null)
    [ -n "$target" ] || exit 0
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
        scoped=0
        ;;
    file)
        [ -f "$target" ] || exit 0
        files="$target"
        scope="$target"
        scoped=0
        ;;
    post-tool)
        [ -f "$target" ] || exit 0
        files="$target"
        scope="$target"
        scoped=1
        ;;
    *)
        files=$("$here/turn-scope.sh" --files "$session" 2>/dev/null)
        scope="files changed this turn"
        scoped=1
        ;;
esac

files=$(printf '%s\n' $files | grep -v '^$' | while IFS= read -r f; do [ -f "$f" ] && echo "$f"; done)

if [ -z "$files" ]; then
    if [ "$mode" = "post-tool" ] || [ "$mode" = "stop" ]; then
        exit 0
    fi
    echo "readability: no Source files in scope"
    exit 0
fi

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

captured=$("$here/turn-scope.sh" --captured "$session" 2>/dev/null)

touched="$work/touched"
: > "$touched"
if [ "$scoped" -eq 1 ]; then
    "$here/turn-scope.sh" --added "$session" 2>/dev/null > "$touched"
fi

scan="$work/scan"
printf '%s\n' "$files" | xargs awk -v MINBLOCK="$minsplit" -f "$here/cxx-scan.awk" 2>/dev/null > "$scan"

findings=0
report=""

emit() {
    local rule="$1"
    local hits="$2"
    if [ -n "$hits" ]; then
        findings=$((findings + 1))
        report="${report}"$'\n'"[READABILITY] ${rule}"$'\n'"${hits}"$'\n'
    fi
}

long_hits=$(awk -F'\t' -v MAX="$maxfunc" -v MIN="$minsplit" -v SCOPED="$scoped" -v TOUCH="$touched" '
    BEGIN {
        if (SCOPED == 1) {
            while ((getline l < TOUCH) > 0) {
                split(l, p, ":")
                key = p[1]
                added[key ":" p[2]] = 1
            }
        }
    }
    $1 == "B" { bf[$2 "\t" $7] = bf[$2 "\t" $7] $3 ":" $4 ":" $5 ":" $6 " " ; next }
    $1 == "F" {
        len = $4 - $3 + 1
        if (len <= MAX) { next }
        if (SCOPED == 1) {
            hit = 0
            for (i = $3; i <= $4; i++) { if (($2 ":" i) in added) { hit = 1; break } }
            if (hit == 0) { next }
        }
        owner = $9
        if (owner != "") { owner = owner "::" }
        printf "  %s:%d  %s%s is %d lines (limit %d)\n", $2, $3, owner, $8, len, MAX
        cands = bf[$2 "\t" $8]
        if (cands == "") {
            printf "      no single block of %d+ lines to lift; the split has to be chosen by hand\n", MIN
            next
        }
        n = split(cands, c, " ")
        shown = 0
        for (i = 1; i <= n; i++) {
            if (c[i] == "") { continue }
            split(c[i], d, ":")
            printf "      extract lines %s-%s (%s-block, %s lines)\n", d[1], d[2], d[4], d[3]
            shown++
            if (shown >= 3) { break }
        }
    }
' "$scan")
emit "function longer than ${maxfunc} lines - lift a block of ${minsplit}+ lines into its own function" "$long_hits"

multi_hits=""
for f in $files; do
    case "$f" in
        *.cpp) ;;
        *) continue ;;
    esac
    hdr="${f%.cpp}.h"
    if [ ! -f "$hdr" ]; then
        hdr="${f%.cpp}"
        hdr="${hdr%%_*}.h"
    fi
    : > "$work/hdrclasses"
    if [ -f "$hdr" ]; then
        grep -oE '^[[:space:]]*(class|struct)[[:space:]]+[A-Za-z_][A-Za-z0-9_]*' "$hdr" 2>/dev/null \
            | awk '{ print $NF }' | sort -u > "$work/hdrclasses"
    fi
    extra=$(awk -F'\t' -v F="$f" -v HC="$work/hdrclasses" -v SCOPED="$scoped" -v TOUCH="$touched" '
        BEGIN {
            while ((getline l < HC) > 0) { if (l != "") { declared[l] = 1 } }
            if (SCOPED == 1) { while ((getline l < TOUCH) > 0) { added[l] = 1 } }
        }
        $1 == "F" && $2 == F && $9 != "" && $11 == "qualified" {
            if (!($9 in first)) { first[$9] = $3; ord[++n] = $9 }
        }
        END {
            base = F
            sub(/^.*\//, "", base)
            sub(/\.cpp$/, "", base)
            sub(/_.*$/, "", base)
            for (i = 1; i <= n; i++) {
                c = ord[i]
                if (c == base || (c in declared)) { continue }
                if (SCOPED == 1 && !((F ":" first[c]) in added)) { continue }
                printf "      %s (first defined at line %s)\n", c, first[c]
            }
        }
    ' "$scan")
    if [ -n "$extra" ]; then
        multi_hits="${multi_hits}  ${f} implements classes that are neither its own nor declared in its header:"$'\n'"${extra}"$'\n'
    fi
done
emit "several classes implemented in one .cpp - give each class its own translation unit" "$multi_hits"

order_hits=""
for f in $files; do
    case "$f" in
        *.cpp) ;;
        *) continue ;;
    esac
    hdr="${f%.cpp}.h"
    [ -f "$hdr" ] || continue
    cls=$(basename "$f" .cpp)
    decl=$(awk -v C="$cls" '
        BEGIN { depth = 0; inclass = 0 }
        {
            line = $0
            sub(/\/\/.*$/, "", line)
            if (inclass == 0 && line ~ ("^[[:space:]]*(class|struct)[[:space:]]+" C "[^A-Za-z0-9_]") && line !~ /;[[:space:]]*$/) {
                inclass = 1
                next
            }
            if (inclass == 0) { next }
            if (line ~ /\}[[:space:]]*;/) { inclass = 0; next }
            if (line !~ /\(/) { next }
            if (line ~ /^[[:space:]]*(\/\/|#|template|friend|using|typedef)/) { next }
            sig = line
            sub(/\(.*$/, "", sig)
            sub(/[[:space:]]+$/, "", sig)
            if (match(sig, /[A-Za-z_~][A-Za-z0-9_]*$/)) {
                nm = substr(sig, RSTART, RLENGTH)
                if (nm == C || nm == "~" C) { next }
                if (!(nm in seen)) { seen[nm] = 1; print nm }
            }
        }
    ' "$hdr")
    [ -n "$decl" ] || continue
    printf '%s\n' "$decl" > "$work/decl"
    defs=$(awk -F'\t' -v F="$f" -v C="$cls" '$1 == "F" && $2 == F && $9 == C { if (!($8 in s)) { s[$8] = 1; print $8 "\t" $3 } }' "$scan")
    [ -n "$defs" ] || continue
    inv=$(printf '%s' "$defs" | awk -F'\t' -v DECLFILE="$work/decl" -v F="$f" -v SCOPED="$scoped" -v TOUCH="$touched" '
        BEGIN {
            i = 0
            while ((getline l < DECLFILE) > 0) { if (l != "") { rank[l] = ++i } }
            if (SCOPED == 1) { while ((getline l < TOUCH) > 0) { added[l] = 1 } }
        }
        { if ($1 in rank) { r[++k] = rank[$1]; nm[k] = $1; ln[k] = $2 } }
        END {
            bad = 0
            for (i = 2; i <= k; i++) {
                if (r[i] < r[i-1]) {
                    if (SCOPED == 1 && !((F ":" ln[i]) in added)) { continue }
                    bad++
                    if (bad <= 3) { printf "      %s (line %s) is declared before %s but defined after it\n", nm[i], ln[i], nm[i-1] }
                }
            }
            if (bad > 0) { printf "COUNT %d\n", bad }
        }
    ')
    cnt=$(printf '%s' "$inv" | awk '/^COUNT/ { print $2 }')
    if [ -n "$cnt" ] && [ "$cnt" -gt 0 ]; then
        detail=$(printf '%s' "$inv" | grep -v '^COUNT')
        order_hits="${order_hits}  ${f}: ${cnt} definition(s) out of header order"$'\n'"${detail}"$'\n'
    fi
done
emit "definition order does not follow the header's declaration order" "$order_hits"

group_hits=$(printf '%s\n' "$files" | grep -E '\.h$' | xargs awk '
    function kindOf(decl,   stripped) {
        stripped = decl
        while (match(stripped, /<[^<>]*>/)) {
            stripped = substr(stripped, 1, RSTART - 1) substr(stripped, RSTART + RLENGTH)
        }
        if (stripped !~ /\(/) { return "var" }
        if (stripped ~ /=[[:space:]]*[^;]*\(/ && stripped !~ /^[[:space:]]*[A-Za-z_][A-Za-z0-9_:<>,&* ]*\(/) { return "var" }
        return "func"
    }
    FNR == 1 {
        depth = 0; sp = 0; bodydepth = -1; curclass = ""
        access = ""; last = ""; flips = 0; reported = 0; pending = ""
    }
    {
        line = $0
        sub(/\/\/.*$/, "", line)

        tmp = line; opens  = gsub(/\{/, "", tmp)
        tmp = line; closes = gsub(/\}/, "", tmp)

        if (line ~ /^[[:space:]]*(class|struct)[[:space:]]+[A-Za-z_]/ && line !~ /;[[:space:]]*$/) {
            sp++
            stkcls[sp] = curclass; stkacc[sp] = access;  stklast[sp] = last
            stkflips[sp] = flips;  stkrep[sp] = reported
            stkdepth[sp] = depth;  stkbody[sp] = bodydepth

            c = line
            sub(/^[[:space:]]*(class|struct)[[:space:]]+/, "", c)
            if (match(c, /^[A-Za-z_][A-Za-z0-9_]*/)) { curclass = substr(c, RSTART, RLENGTH) }
            access = "private"
            if (line ~ /^[[:space:]]*struct/) { access = "public" }
            last = ""; flips = 0; reported = 0; pending = ""
            bodydepth = depth + 1
            depth += opens - closes
            next
        }

        if (curclass != "" && depth == bodydepth) {
            if (line ~ /^[[:space:]]*(public|protected|private)[[:space:]]*:/) {
                section = line
                sub(/^[[:space:]]*/, "", section)
                sub(/[[:space:]]*:.*$/, "", section)
                if (section != access) { access = section; last = ""; flips = 0 }
                pending = ""
            }
            else if (line ~ /^[[:space:]]*(using|typedef|friend|template|enum|#|\})/) {
                pending = ""
            }
            else if (line !~ /^[[:space:]]*$/) {
                pending = pending " " line

                if (opens != closes) {
                    pending = ""
                }
                else if (line ~ /;[[:space:]]*$/) {
                    kind = kindOf(pending)
                    if (last != "" && kind != last) {
                        flips++
                        if (flips == 2 && reported == 0) {
                            reported = 1
                            printf "  %s:%d  %s interleaves member variables and member function declarations\n", FILENAME, FNR, curclass
                        }
                    }
                    last = kind
                    pending = ""
                }
                else if (line ~ /\}[[:space:]]*$/) {
                    pending = ""
                }
            }
        }

        depth += opens - closes

        while (sp > 0 && depth <= stkdepth[sp]) {
            curclass = stkcls[sp]; access = stkacc[sp]; last = stklast[sp]
            flips = stkflips[sp]; reported = stkrep[sp]; bodydepth = stkbody[sp]
            pending = ""
            sp--
        }
    }
' 2>/dev/null)
emit "member variables and member functions interleaved - group declarations by kind" "$group_hits"

if [ "$mode" = "stop" ] || [ "$mode" = "post-tool" ]; then
    if [ "$findings" -eq 0 ]; then
        exit 0
    fi
    if [ "$scoped" -eq 1 ] && [ "$captured" != "yes" ]; then
        if [ "$mode" = "post-tool" ]; then
            exit 0
        fi
        reason="readability: no turn snapshot for this session, so these ${findings} finding(s) cover the whole uncommitted working tree, not what you wrote this turn."$'\n'
        reason="${reason}Not blocking on them. Report them to the user as pre-existing and leave them alone unless you actually touched the code."$'\n'"${report}"
        jq -n --arg r "$reason" '{systemMessage: $r}'
        exit 0
    fi
    reason="readability: ${findings} issue(s) in ${scope}."$'\n'"${report}"
    reason="${reason}"$'\n'"Fix these in the code you just wrote. If a finding is about pre-existing code you did not touch, say so and leave it alone."$'\n'
    if [ "$mode" = "stop" ]; then
        jq -n --arg r "$reason" '{decision: "block", reason: $r}'
    else
        jq -n --arg r "$reason" '{hookSpecificOutput: {hookEventName: "PostToolUse", additionalContext: $r}}'
    fi
    exit 0
fi

echo "readability: checking ${scope}"
printf '%s\n' "$files" | sed 's/^/  /'
if [ "$findings" -eq 0 ]; then
    echo
    echo "readability: no issues in the machine-checked set"
else
    printf '%s\n' "$report"
fi
echo "readability: machine-checked - function length (>${maxfunc} lines) with ${minsplit}+ line extraction candidates, multiple classes per .cpp, definition order vs header, variable/function declaration grouping"
echo "readability: NOT machine-checked - judge these yourself:"
echo "  Whether names carry the intent that comments are banned from carrying"
echo "  Whether a long function is long because it is doing two jobs, or because one job is genuinely long"
echo "  Whether an extraction candidate is a real concept or an arbitrary slice"

#!/usr/bin/env bash
set -uo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="${CLAUDE_PROJECT_DIR:-$(git rev-parse --show-toplevel 2>/dev/null || pwd)}"
cd "$root" 2>/dev/null || exit 0

steps="api-surface structure data-flow design-rules readability behavior-delta|code-behavior broader-api broader-conformance recursion"
frontsteps="api-surface structure data-flow"

minlen=40
maxdenials=3
judgemodel="haiku"
judgewait=120

mode=""
case "${1:-}" in
    --hook)   mode="${2:-}" ;;
    --check)  mode="check" ;;
    --judge)  mode="judge" ;;
    --format) mode="format" ;;
esac

attestation_contract() {
    printf '%s\n' "SYSTEMATIC PASS"
    printf '%s\n' "$steps" | tr ' ' '\n' | sed -e 's/|/ (or /' -e 's/$/: .../' \
        | sed 's/ (or \([a-z-]*\): \.\.\./: ... [name it \1 instead when the turn changes no code]/'
    printf '%s\n' "readability: answer it for code you wrote and for code you were asked to analyse."
}

grade() {
    awk -v steps="${1:-$steps}" -v minlen="$minlen" '
        BEGIN {
            n = split(steps, s, " ")
            for (i = 1; i <= n; i++) {
                m = split(s[i], alt, "|")
                for (j = 1; j <= m; j++) { want[alt[j]] = 1; slot[alt[j]] = s[i] }
            }
        }
        {
            line = $0
            sub(/^[[:space:]]*[-+*][[:space:]]+/, "", line)
            gsub(/`/, "", line)
            gsub(/\*/, "", line)
            gsub(/_/, "", line)
            sub(/^[[:space:]]+/, "", line)
            if (match(line, /^[a-z-]+:/)) {
                k = substr(line, 1, RLENGTH - 1)
                if (k in want) {
                    v = substr(line, RLENGTH + 1)
                    sub(/^[[:space:]]+/, "", v)
                    sub(/[[:space:]]+$/, "", v)
                    key = slot[k]
                    seen[key] = 1
                    val[key] = val[key] " " v
                    last = key
                    next
                }
            }
            if (last != "" && line !~ /^[[:space:]]*$/ && line !~ /^#/) { val[last] = val[last] " " line }
            else { last = "" }
        }
        END {
            for (i = 1; i <= n; i++) {
                k = s[i]
                label = k
                gsub(/\|/, " or ", label)
                if (!(k in seen)) { print "MISSING\t" label; continue }
                v = val[k]
                sub(/^[[:space:]]+/, "", v)
                if (length(v) < minlen) { print "THIN\t" label "\t" length(v); continue }
                print "OK\t" label
            }
        }
    '
}

turn_text() {
    jq -rs '
        ([ range(0; length) as $i
           | select(.[$i].type == "assistant" or (.[$i].type == "user"
               and ((.[$i].message.content | type) == "array")
               and (any(.[$i].message.content[]?; .type == "text"))))
           | select(.[$i].type == "user") | $i ] | last // -1) as $b
        | .[$b + 1:]
        | map(select(.type == "assistant"))
        | map(.message.content[]? | select(.type == "text") | .text)
        | join("\n")
    ' "$1" 2>/dev/null
}

turn_files_touched() {
    jq -rs '
        ([ range(0; length) as $i
           | select(.[$i].type == "assistant" or (.[$i].type == "user"
               and ((.[$i].message.content | type) == "array")
               and (any(.[$i].message.content[]?; .type == "text"))))
           | select(.[$i].type == "user") | $i ] | last // -1) as $b
        | .[$b + 1:]
        | map(select(.type == "assistant"))
        | map(.message.content[]? | select(.type == "tool_use"))
        | map(
            if (.name == "Read" or .name == "Grep" or .name == "Edit" or .name == "Write")
            then ((.input.file_path // .input.path // "") | tostring)
            else ((.input.command // "") | tostring) end)
        | join("\n")
    ' "$1" 2>/dev/null \
      | grep -oE 'Source/[A-Za-z0-9_/.-]+\.(cpp|h)' \
      | sort -u \
      | while IFS= read -r f; do
            if [ -f "$f" ]; then
                echo "$f"
            fi
        done
}

analysis_anchor() {
    local list
    list=$(turn_files_touched "$1")
    if [ -z "$list" ]; then
        return
    fi
    printf '%s\n' "$list" \
      | xargs awk -v MINBLOCK=999999 -f "$here/cxx-scan.awk" 2>/dev/null \
      | awk -F'\t' '$1 == "F" {
            owner = $9
            if (owner != "") { owner = owner "::" }
            printf "%s  %s%s  lines %s-%s\n", $2, owner, $8, $3, $4
        }' \
      | head -250
}

scoped_diff() {
    local full truncated
    full=$("$here/turn-scope.sh" --diff "${gitsession:-unknown}" 2>/dev/null)
    truncated=$(printf '%s' "$full" | head -1200)
    if [ "$(printf '%s' "$full" | wc -l)" -gt 1200 ]; then
        printf '%s\n' "$truncated"
        printf '%s\n' "[diff truncated at 1200 lines]"
        return
    fi
    printf '%s' "$truncated"
}

pre_edit_gate() {
    local payload session transcript target satisfied denials count text result missing thin reason

    payload=$(cat)

    session=$(printf '%s' "$payload" | jq -r '.session_id // "unknown"' 2>/dev/null)
    transcript=$(printf '%s' "$payload" | jq -r '.transcript_path // ""' 2>/dev/null)
    target=$(printf '%s' "$payload" | jq -r '.tool_input.file_path // ""' 2>/dev/null)
    target="${target#"$root"/}"

    case "$target" in
        Source/*.cpp|Source/*.h) ;;
        *) return 0 ;;
    esac

    if [ ! -f "${TMPDIR:-/tmp}/claude-systematic-deep-${session//[^A-Za-z0-9_-]/_}" ]; then
        return 0
    fi

    satisfied="${TMPDIR:-/tmp}/claude-systematic-front-${session//[^A-Za-z0-9_-]/_}"
    if [ -f "$satisfied" ]; then
        return 0
    fi

    if [ ! -f "$transcript" ]; then
        return 0
    fi

    text=$(turn_text "$transcript")
    result=$(printf '%s' "$text" | grade "$frontsteps")

    missing=$(printf '%s' "$result" | awk -F'\t' '$1 == "MISSING" { print "  " $2 " - absent" }')
    thin=$(printf '%s' "$result" | awk -F'\t' '$1 == "THIN" { print "  " $2 " - only " $3 " characters" }')

    if [ -z "$missing" ] && [ -z "$thin" ]; then
        : > "$satisfied"
        printf '{"systemMessage":"systematic-check: front-half passes stated, Source edits unblocked for this turn","suppressOutput":true}\n'
        return 0
    fi

    denials="${TMPDIR:-/tmp}/claude-systematic-frontdenials-${session//[^A-Za-z0-9_-]/_}"
    count=$(cat "$denials" 2>/dev/null)
    count=$(( ${count:-0} + 1 ))
    printf '%s' "$count" > "$denials"

    if [ "$count" -gt "$maxdenials" ]; then
        printf '{"systemMessage":"systematic-check: front-half passes still missing after %s denials, letting the edit through","suppressOutput":true}\n' "$maxdenials"
        return 0
    fi

    reason="Deep mode: the front-half passes must be stated before you edit Source (CLAUDE.md, How to Systematically Solve Problems)."$'\n'
    reason="${reason}This edit to ${target} was not applied. Answer these in your reply text first, against the code you are about to change, then repeat the edit:"$'\n\n'
    if [ -n "$missing" ]; then
        reason="${reason}Missing:"$'\n'"${missing}"$'\n'
    fi
    if [ -n "$thin" ]; then
        reason="${reason}Too thin to be an answer (need ${minlen}+ characters):"$'\n'"${thin}"$'\n'
    fi
    reason="${reason}"$'\n'"Required shape:"$'\n'"$(printf '%s\n' "$frontsteps" | tr ' ' '\n' | sed 's/$/: .../')"$'\n\n'
    reason="${reason}Stating them after the edit does not count. The remaining five passes still belong at the end of the turn."$'\n'

    jq -n --arg r "$reason" \
      '{hookSpecificOutput: {hookEventName: "PreToolUse", permissionDecision: "deny", permissionDecisionReason: $r}}'
}

run_judge() {
    local block="$1"
    local diff="$2"
    local kind="${3:-diff}"
    local work out prompt pid waited raw subject header

    command -v claude >/dev/null 2>&1 || return 2

    work=$(mktemp -d) || return 2
    out="$work/out"

    subject="diff"
    header="DIFF"
    if [ "$kind" = "inventory" ]; then
        subject="set of functions"
        header="FUNCTIONS THE ENGINEER INSPECTED THIS TURN"
    fi

    prompt="A ${subject} follows, then labelled analysis passes an engineer claims to have performed on it.

You are NOT reviewing the code and NOT checking whether the passes are technically correct. Your only question, per pass, is: was this written about THIS ${subject}, or is it boilerplate that would read identically for anything?

Fail a pass only if one of these is true:
- it names no identifier, file, call site or behaviour that appears in the ${subject}, and describes nothing specific to it
- it plainly describes something other than what the ${subject} shows

Never fail a pass because you disagree with a claim it makes, because you cannot verify a claim about code outside the ${subject}, or because it is short. When in doubt, pass.

=== ${header} ===
${diff}

=== CLAIMED PASSES ===
${block}

Output one JSON object and stop. No explanation, no commentary before or after it.
{\"verdict\":\"pass\"}
or
{\"verdict\":\"fail\",\"failures\":[{\"step\":\"data-flow\",\"why\":\"one short sentence\"}]}"

    (
        cd "$work" || exit 1
        printf '%s' "$prompt" | env -u CLAUDE_PROJECT_DIR claude -p --model "$judgemodel" \
            --system-prompt "You are a terse classifier. You reply with one JSON object and no other text." \
            --disallowedTools "Bash,Edit,Write,Read,Glob,Grep,Task,WebFetch,WebSearch,TodoWrite" \
            > "$out" 2>/dev/null
    ) &
    pid=$!
    waited=0
    while kill -0 "$pid" 2>/dev/null && [ "$waited" -lt "$judgewait" ]; do
        sleep 1
        waited=$((waited + 1))
    done
    if kill -0 "$pid" 2>/dev/null; then
        kill -9 "$pid" 2>/dev/null
        wait "$pid" 2>/dev/null
        rm -rf "$work"
        return 2
    fi
    wait "$pid" 2>/dev/null

    raw=$(tr -d '\000' < "$out" | awk '
        started == 0 {
            i = index($0, "{")
            if (i == 0) { next }
            started = 1
            $0 = substr($0, i)
        }
        {
            for (c = 1; c <= length($0); c++) {
                ch = substr($0, c, 1)
                if (ch == "\"" && prev != "\\") { instr = 1 - instr }
                if (instr == 0 && ch == "{") { depth++ }
                if (instr == 0 && ch == "}") { depth-- }
                out = out ch
                prev = ch
                if (started == 1 && depth == 0) { print out; exit }
            }
            out = out " "
        }
    ')
    verdict=$(printf '%s' "$raw" | jq -r '.verdict // empty' 2>/dev/null)
    judgefailures=$(printf '%s' "$raw" | jq -r '.failures[]? | "  " + .step + ": " + .why' 2>/dev/null)
    rm -rf "$work"

    if [ -z "$verdict" ]; then
        return 2
    fi
    if [ "$verdict" = "pass" ]; then
        return 0
    fi
    return 1
}

case "$mode" in
    format)
        attestation_contract
        exit 0
        ;;
    check)
        grade < "${2:-/dev/stdin}"
        exit 0
        ;;
    judge)
        gitsession="${4:-unknown}"
        block=$(cat "${2:-/dev/stdin}")
        if [ -n "${3:-}" ]; then
            usediff=$(cat "$3")
        else
            usediff=$(scoped_diff)
        fi
        run_judge "$block" "$usediff"
        rc=$?
        echo "judge rc=$rc verdict=${verdict:-none}"
        printf '%s\n' "${judgefailures:-}"
        exit 0
        ;;
    pre-edit)
        pre_edit_gate
        exit 0
        ;;
    stop) ;;
    *)
        echo "usage: systematic-check.sh --hook stop | --hook pre-edit | --check [file] | --judge [file] | --format"
        exit 0
        ;;
esac

payload=$(cat)

active=$(printf '%s' "$payload" | jq -r '.stop_hook_active // false' 2>/dev/null)
if [ "$active" = "true" ]; then
    exit 0
fi

session=$(printf '%s' "$payload" | jq -r '.session_id // "unknown"' 2>/dev/null)
transcript=$(printf '%s' "$payload" | jq -r '.transcript_path // ""' 2>/dev/null)
gitsession="$session"

marker="${TMPDIR:-/tmp}/claude-systematic-deep-${session//[^A-Za-z0-9_-]/_}"
if [ ! -f "$marker" ]; then
    exit 0
fi

if [ ! -f "$transcript" ]; then
    exit 0
fi

text=$(turn_text "$transcript")
if [ -z "$text" ]; then
    exit 0
fi

result=$(printf '%s' "$text" | grade)

missing=$(printf '%s' "$result" | awk -F'\t' '$1 == "MISSING" { print "  " $2 " - absent" }')
thin=$(printf '%s' "$result" | awk -F'\t' '$1 == "THIN" { print "  " $2 " - only " $3 " characters" }')

if [ -n "$missing" ] || [ -n "$thin" ]; then
    reason="Deep mode: the systematic pass block is incomplete (CLAUDE.md, How to Systematically Solve Problems)."$'\n'
    reason="${reason}End your reply with a SYSTEMATIC PASS block, one labelled line per step, each answered against the code you actually touched."$'\n\n'
    if [ -n "$missing" ]; then
        reason="${reason}Missing:"$'\n'"${missing}"$'\n'
    fi
    if [ -n "$thin" ]; then
        reason="${reason}Too thin to be an answer (need ${minlen}+ characters):"$'\n'"${thin}"$'\n'
    fi
    reason="${reason}"$'\n'"Required shape:"$'\n'"$(attestation_contract)"$'\n'
    jq -n --arg r "$reason" '{decision: "block", reason: $r}'
    exit 0
fi

if [ -x "$here/design-rules.sh" ]; then
    if ! "$here/design-rules.sh" >/dev/null 2>&1; then
        printf '{"systemMessage":"systematic-check: pass block complete; judge skipped, design-rule gate is already blocking","suppressOutput":true}\n'
        exit 0
    fi
fi

diff=$(scoped_diff)
anchorkind="diff"
if [ -z "$diff" ]; then
    diff=$(analysis_anchor "$transcript")
    anchorkind="inventory"
fi
if [ -z "$diff" ]; then
    printf '{"systemMessage":"systematic-check: pass block complete, no Source code in scope to judge","suppressOutput":true}\n'
    exit 0
fi

block=$(printf '%s' "$text" | awk '/^[[:space:]]*#*[[:space:]]*SYSTEMATIC PASS/, 0')
if [ -z "$block" ]; then
    block="$text"
fi

verdict=""
judgefailures=""
run_judge "$block" "$diff" "$anchorkind"
rc=$?

if [ "$rc" -eq 2 ]; then
    printf '{"systemMessage":"systematic-check: pass block complete; judge unavailable, not enforced","suppressOutput":true}\n'
    exit 0
fi

if [ "$rc" -eq 1 ]; then
    reason="Deep mode: the systematic pass block does not hold up against the diff."$'\n'
    reason="${reason}A second model read your passes alongside the actual Source changes and found these generic or unsupported:"$'\n\n'
    reason="${reason}${judgefailures}"$'\n\n'
    reason="${reason}Redo those passes against the code you changed - name the real functions, call sites and behaviours - then restate the block."$'\n'
    jq -n --arg r "$reason" '{decision: "block", reason: $r}'
    exit 0
fi

printf '{"systemMessage":"systematic-check: all eight passes present and judged against the diff","suppressOutput":true}\n'

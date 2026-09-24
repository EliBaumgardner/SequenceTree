#!/usr/bin/env bash
set -uo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="${CLAUDE_PROJECT_DIR:-$(git rev-parse --show-toplevel 2>/dev/null || pwd)}"
cd "$root" 2>/dev/null || exit 0

payload=$(cat)

target=$(printf '%s' "$payload" | jq -r '.tool_input.file_path // ""' 2>/dev/null)
[ -n "$target" ] || exit 0
target="${target#"$root"/}"
case "$target" in
    Source/*.cpp|Source/*.h) ;;
    *) exit 0 ;;
esac
[ -f "$target" ] || exit 0

old=$(printf '%s' "$payload" | jq -r '.tool_input.old_string // ""' 2>/dev/null)
new=$(printf '%s' "$payload" | jq -r '.tool_input.new_string // ""' 2>/dev/null)
[ -n "$old$new" ] || exit 0

deny() {
    jq -n --arg r "$1" \
      '{hookSpecificOutput: {hookEventName: "PreToolUse", permissionDecision: "deny", permissionDecisionReason: $r}}'
    exit 0
}

defsig='^[[:space:]]*[A-Za-z_][A-Za-z0-9_:<>,&\* ]*[[:space:]]+([A-Za-z_][A-Za-z0-9_]*)::([A-Za-z_~][A-Za-z0-9_]*)[[:space:]]*\('

added_def=$(printf '%s' "$new" | grep -nE "$defsig" | head -1)
removed_def=$(printf '%s' "$old" | grep -nE "$defsig" | head -1)

body_moves() {
    local text="$1"
    local hay="$2"
    local run=0 best=0
    while IFS= read -r line; do
        local trimmed
        trimmed=$(printf '%s' "$line" | sed 's/^[[:space:]]*//; s/[[:space:]]*$//')
        case "$trimmed" in
            ""|"{"|"}"|"//"*) continue ;;
        esac
        if printf '%s' "$hay" | grep -qF -- "$trimmed"; then
            run=$((run + 1))
            if [ "$run" -gt "$best" ]; then best=$run; fi
        else
            run=0
        fi
    done <<< "$text"
    echo "$best"
}

if [ -n "$added_def" ] && [ -z "$removed_def" ]; then
    current=$(cat "$target")
    moved=$(body_moves "$new" "$current")
    if [ "$moved" -ge 5 ]; then
        deny "This edit adds a function to ${target} whose body already exists in the file - that is an extraction, not new code (${moved} consecutive lines matched).

Use the tool that performs it:
  refactor.encap ${target}:<start>-<end> <name>

It computes the parameters and return value from the data flow, inherits the enclosing function's const/noexcept, places the declaration in the matching access section, builds SequenceTree_Standalone, and restores every file if the build fails. It refuses rather than guessing when a return crosses the boundary, more than one value stays live, or a type is auto.

If this really is new code that happens to resemble nearby lines, say so and re-issue the edit; the gate only inspects one call at a time."
    fi
fi

if [ -n "$removed_def" ]; then
    name=$(printf '%s' "$old" | sed -nE "s/$defsig.*/\\2/p" | head -1)
    cls=$(printf '%s' "$old" | sed -nE "s/$defsig.*/\\1/p" | head -1)
    if [ -n "$name" ] && ! printf '%s' "$new" | grep -qE "[^A-Za-z0-9_]${name}[[:space:]]*\(" ; then
        callers=$(grep -rnE "\b${name}[[:space:]]*\(" Source --include='*.cpp' 2>/dev/null \
                  | while IFS= read -r hit; do
                        text=$(printf '%s' "$hit" | cut -d: -f3- | sed 's/^[[:space:]]*//; s/[[:space:]]*$//')
                        if [ -n "$text" ] && ! printf '%s' "$old" | grep -qF -- "$text"; then
                            printf '%s\n' "$hit" | cut -d: -f1,2
                        fi
                    done | head -3)
        if [ -n "$callers" ]; then
            deny "This edit removes the definition of ${cls}::${name} from ${target} while call sites remain (${callers//$'\n'/, }) - that is a decapsulation in progress.

Use the tool that performs it:
  refactor.decap ${cls}::${name}

It rewrites every call site, substitutes arguments for parameters, prefixes the receiver onto member access for cross-class sites, removes the declaration, builds SequenceTree_Standalone, and restores every file if the build fails. It refuses rather than guessing when the function is virtual, its address is taken, its name is ambiguous, an argument would be evaluated more than once, or the body would reach a non-public member from another class - that last one names the member you would have to make public.

If you meant to delete the function outright, remove its call sites first."
        fi
    fi
fi

exit 0

#!/usr/bin/env bash
set -uo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="${CLAUDE_PROJECT_DIR:-$(git rev-parse --show-toplevel 2>/dev/null || pwd)}"
cd "$root" 2>/dev/null || exit 0

payload=$(cat)
prompt=$(printf '%s' "$payload" | jq -r '.prompt // ""' 2>/dev/null)
session=$(printf '%s' "$payload" | jq -r '.session_id // "unknown"' 2>/dev/null)

"$here/turn-scope.sh" --snapshot "$session" 2>/dev/null

deep=0
if printf '%s' "$prompt" | grep -qiE -- '-deep([^[:alnum:]_-]|$)'; then
    deep=1
fi

marker="${TMPDIR:-/tmp}/claude-md-hook-${session//[^A-Za-z0-9_-]/_}"
deepmarker="${TMPDIR:-/tmp}/claude-systematic-deep-${session//[^A-Za-z0-9_-]/_}"

if [ "$deep" -eq 1 ]; then
    : > "$deepmarker"
else
    rm -f "$deepmarker"
fi

rm -f "${TMPDIR:-/tmp}/claude-systematic-front-${session//[^A-Za-z0-9_-]/_}" \
      "${TMPDIR:-/tmp}/claude-systematic-frontdenials-${session//[^A-Za-z0-9_-]/_}"
first=0
if [ ! -f "$marker" ]; then
    first=1
    : > "$marker"
fi

check=$("$here/design-rules.sh" 2>&1)

clean=0
if printf '%s' "$check" | grep -qE 'design-rules: clean|no changed Source files'; then
    clean=1
fi

if [ "$deep" -eq 0 ] && [ "$first" -eq 0 ] && [ "$clean" -eq 1 ]; then
    exit 0
fi

context=""

if [ "$deep" -eq 1 ] || [ "$first" -eq 1 ]; then
    rules=$(awk '/^### How to Systematically Solve Problems/, 0' CLAUDE.md 2>/dev/null)
    if [ -n "$rules" ]; then
        context="${rules}"$'\n\n---\n\n'
    fi
fi

context="${context}Design-rule check on files changed vs HEAD:"$'\n\n'"${check}"$'\n\n'

context="${context}Readability gate (runs on the code you add this turn, not on the working tree):"$'\n'
context="${context}  no function over 80 lines - when one is, lift out a block of 30+ lines as its own function"$'\n'
context="${context}  one class per .cpp - do not implement a second class there unless its header declares it"$'\n'
context="${context}  define members in the order the header declares them, and group member variables apart from member functions"$'\n'
context="${context}Run .claude/readability.sh --file <path> on any file you are asked to analyse, and report what it finds alongside your own read of the naming."$'\n\n'

if [ "$deep" -eq 1 ]; then
    context="${context}Deep mode requested. Before answering, state which of the Key Design Rules apply to the code in question and how you verified each, and work through the General Principles pass in full. Run .claude/design-rules.sh --all to check the whole tree."$'\n'
    context="${context}Then audit every function you edited for unintended behavior change. For each class of input the old code handled, state what it did before, what it does now, and whether the request asked for that difference. Revert every difference the request did not ask for, even one you believe is an improvement, and report it separately as a pre-existing bug for the user to decide on. Keep an unrequested change only when the requested fix does not work without it, and say why."$'\n'
    context="${context}"$'\n'"Front-load the investigation. Before your first Edit or Write under Source, state api-surface, structure and data-flow as labelled lines in your reply, answered against the code you are about to change. A PreToolUse hook denies that edit until all three are there; stating them afterwards does not count."$'\n\n'
    context="${context}Then end your reply with the full SYSTEMATIC PASS block - one labelled line per step, each answered against the code you actually touched, naming the real functions, files and call sites. A Stop hook checks every line is present, then has a second model read them against your diff; generic lines are rejected and you will be asked to redo them."$'\n\n'
    context="${context}$("$here/systematic-check.sh" --format)"$'\n'
else
    context="${context}Analysis counts as work: when the user asks you to analyse, review or explain code rather than change it, the systematic principles apply to that answer too. Name the real functions and call sites, and answer readability for the code you read."$'\n\n'
context="${context}Light mode: answer directly. No rule recitation, no multi-pass audit, no broader-API sweep unless the task needs it. The Key Design Rules still bind any code you write. The user requests the full verification protocol by putting -deep in their prompt."$'\n'
fi

jq -n --arg ctx "$context" \
  '{hookSpecificOutput: {hookEventName: "UserPromptSubmit", additionalContext: $ctx}}'

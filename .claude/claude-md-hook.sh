#!/usr/bin/env bash
set -uo pipefail

root="${CLAUDE_PROJECT_DIR:-$(git rev-parse --show-toplevel 2>/dev/null || pwd)}"
cd "$root" 2>/dev/null || exit 0

payload=$(cat)
prompt=$(printf '%s' "$payload" | jq -r '.prompt // ""' 2>/dev/null)
session=$(printf '%s' "$payload" | jq -r '.session_id // "unknown"' 2>/dev/null)

deep=0
if printf '%s' "$prompt" | grep -qiE -- '-deep([^[:alnum:]_-]|$)'; then
    deep=1
fi

marker="${TMPDIR:-/tmp}/claude-md-hook-${session//[^A-Za-z0-9_-]/_}"
first=0
if [ ! -f "$marker" ]; then
    first=1
    : > "$marker"
fi

check=$(./.claude/design-rules.sh 2>&1)

clean=0
if printf '%s' "$check" | grep -qE 'design-rules: clean|no changed Source files'; then
    clean=1
fi

if [ "$deep" -eq 0 ] && [ "$first" -eq 0 ] && [ "$clean" -eq 1 ]; then
    exit 0
fi

context=""

if [ "$deep" -eq 1 ] || [ "$first" -eq 1 ]; then
    rules=$(awk '/^### General Principles for Answering Questions/, 0' CLAUDE.md 2>/dev/null)
    if [ -n "$rules" ]; then
        context="${rules}"$'\n\n---\n\n'
    fi
fi

context="${context}Design-rule check on files changed vs HEAD:"$'\n\n'"${check}"$'\n\n'

if [ "$deep" -eq 1 ]; then
    context="${context}Deep mode requested. Before answering, state which of the Key Design Rules apply to the code in question and how you verified each, and work through the General Principles pass in full. Run .claude/design-rules.sh --all to check the whole tree."$'\n'
else
    context="${context}Light mode: answer directly. No rule recitation, no multi-pass audit, no broader-API sweep unless the task needs it. The Key Design Rules still bind any code you write. The user requests the full verification protocol by putting -deep in their prompt."$'\n'
fi

jq -n --arg ctx "$context" \
  '{hookSpecificOutput: {hookEventName: "UserPromptSubmit", additionalContext: $ctx}}'

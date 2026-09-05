#!/usr/bin/env bash
set -uo pipefail

root="${CLAUDE_PROJECT_DIR:-$(git rev-parse --show-toplevel 2>/dev/null || pwd)}"
cd "$root" 2>/dev/null || exit 0

rules=$(awk '/^### General Principles for Answering Questions/, 0' CLAUDE.md 2>/dev/null)
check=$(./.claude/design-rules.sh 2>&1)

context=$(printf 'Re-read of CLAUDE.md rules (injected every prompt).\n\n%s\n\n---\n\nDesign-rule check on files changed vs HEAD:\n\n%s\n\nBefore answering any question about this code, state which of the Key Design Rules apply to the code in question and how you verified each. Run .claude/design-rules.sh --all to check the whole tree.\n' "$rules" "$check")

jq -n --arg ctx "$context" \
  '{hookSpecificOutput: {hookEventName: "UserPromptSubmit", additionalContext: $ctx}}'

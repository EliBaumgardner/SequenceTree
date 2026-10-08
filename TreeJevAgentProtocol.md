# TreeJev Agent Protocol

## Policy Tree ('sequence_tree_turn')
The tree lives in `.treejev/trees/sequence_tree_turn.json`.
- **Claude Code**: a UserPromptSubmit hook runs `treejev hook claude sequence_tree_turn` on every prompt and adds its result to the turn's context as `[TreeJev: sequence_tree_turn | Final Action: <label>]` with the action's directive. That action is the decision for the turn: follow its directive, and do not run the tree again yourself unless asked. The decision is also recorded for other hooks in `$TMPDIR/treejev/<session_id>.json`.
- Always adhere strictly to the terminal action directive.

## Signifier
Whenever a TreeJev tree has been evaluated for the turn, include the compact signifier in the response:
`[TreeJev: <tree_id> | Final Action: <final_action_label>]`
Never include emojis in any TreeJev notice or signifier.

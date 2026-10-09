# AI Integration and Workflow

I designed SequenceTree and wrote its core by hand: the graph model, the audio engine, the scripting language and the interface. AI coding agents came in later, as a tool for audits, large refactors and repetitive edits, always working inside an architecture and design rules I had already set.

## How agent work is controlled

Agent work moves through four stages, each a folder of plain markdown in `.claude/context/`:

1. **Analysis.** An agent audits the code or researches a topic and writes a report.
2. **Review.** A second agent checks every claim against the source and keeps only what holds up.
3. **Proposal.** What survives becomes a step-by-step plan, with open decisions left to me.
4. **Implementation.** Only a plan I have approved gets built.

Before an agent can finish, automatic checks test its changes against my design rules and run the test suite. A failed check sends the agent back to fix the problem.

## Tools

- **[research-suite](https://github.com/EliBaumgardner/research-suite)**: a Claude Code plugin that runs the pipeline, the checks and a set of C++ refactoring commands.
- **TreeJev**: decides what kind of work each request is and which checks it owes. Its decision trees are in `treejev/`.
- **`.claude/CLAUDE.md`**: the project briefing every agent works from.

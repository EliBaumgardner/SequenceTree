# Ornaments

> Status: Confirmed
> Written 2026-10-09 at 31bccc7. Deployment answers added 2026-10-09.

## In the Owner's Words

> It will be an ornament creator. The user can design ornaments and then apply them to a node in a tree. For instance, a user could design a simple "trill", an ornament which moves up a semitone from the original, host pitch, and back down again to it. I want to have an interface where users can design both the visual appearance of an ornament, (which is used to distinguish ornaments, just like in normal music notation), and also what the ornament does. Then, the user can select which active ornament they want to apply with a button, and click on nodes to add the ornament while the ornament button is toggled. Essentially, we are two processes: The creation of ornaments, via a visual editor the user can use, and the deployment: the selection, and then application of the ornament to a node. I want to first focus on the first aspect: creation.

> The user will use a mini graph. The user can add nodes to the mini graph, of two general kinds: regular nodes and the "host node". There should be a little button pane, like there is in the titleBar. Since the host node can be any pitch, instead of holding an actual pitch value it will hold a semitone offset, and a duration. The user at some point switches to the host node, and connects that node, and then the ornament can be published and used. An ornament without a host node added to the graph cannot, because we do not know where the ornament is occurring in relation to the host node (are these nodes added before the host? after? in the midst?). Also, the duration values of the nodes added in the ornament builder should not be fixed values either, they should be proportionate to the total duration of the node to which the ornament is added. [...] This mirrors the way an actual ornament works.

> The ornament button and the builder will spring from the same button. If a user clicks on the button it will open the builder menu, and if they simply drag from the ornament button on to the canvas, they can apply the ornament. The button will also show the currently active ornament. The build menu will allow users to select different ornaments to have on the ornament button, which when selected, will allow the user to drag the ornament from the ornament button on to the canvas and onto a node. The ornament will always be on the top of the node area. In the side of build panel, where the user actually assembles the ornament, there can be a small button they press to enter the actual visual editor that they freedraw and save as the icon representing the node. It then appears in a sidepanel across from the building panel with all other ornaments. The ornaments on that panel, when selected, open up the ornament on the left of the screen for altering. The ornaments will also have a small name tag under them in the ornament selection menu to the right. [...] Deleting an ornament should ask the user. [...] The user can just share their ornament library with another user.

> The overall ornament system goes in its own window that opens with the button. The window is subdivided into a panel for actually building the ornament, and a panel for selecting the active ornament for alterations or application in the canvas. [Dropping onto an ornamented node] will replace the ornament. [While it plays] the node just lights up. On chord nodes it works the same, an ornament on the first note of a chord, the host, only appears there, just like an actual ornament. You need to individually apply them to each instance. It cannot be used on modulators. [After a confirmed delete, nodes] go back to plain. [A project opened without the ornament:] the node plays plainly and shows a missing ornament mark.

> The ornament button should be in the bottom bar. Right click the node should bring up a context menu with the option to delete the ornament, or shift right click. [A note retimed while its figure plays:] yes it should stretch.

## Summary

An ornament is a short, reusable figure (a trill, a mordent, a grace note, a turn) that decorates one node's note without the user having to build it into the tree. The user designs each ornament once in the ornament builder, which holds a small one-path graph of ornament nodes around a single host node, plus a freehand-drawn symbol that tells ornaments apart on the canvas, as notation does. One ornament button does both jobs: clicking it opens the builder, and dragging from it onto a node applies the ornament it shows. When an ornament is applied to a node, it replaces that node's single note with the ornament's figure. Every pitch is relative to the node's own pitch, and every duration is a share of the node's own duration.

## Parts

- **Creation.** Designing an ornament in the builder: its figure (the mini graph) and its symbol (the freehand drawing), then publishing it once it has a connected host node.
- **Deployment.** Choosing the active ornament in the builder, dragging it from the ornament button onto a node, and how an ornamented node then plays and looks.

Creation is built first.

## How the User Works With It

### Building an ornament

Building a lower-neighbour figure before the main note:

1. The user clicks the ornament button, and the ornament window opens.
2. The window has two panels. On the left is the build panel, where an ornament is assembled. On the right is the selection panel: every ornament in the user's library, each shown as its symbol with a small name tag under it. The user creates a new ornament and names it "Lower grace", and it opens in the build panel.
3. A small button pane along the build panel's top, like the one in the title bar, offers the node kinds: a regular ornament node and the host node.
4. With "regular node" chosen, the user places a node and sets its offset to −1 semitone and its velocity to 70%, so it is softer than the main note.
5. The user switches to "host node" and places the host to the right, connecting the first node to it. The host's offset stays 0, so it plays the ornamented node's own pitch.
6. The user drags the two nodes, and the arrow lengths set how the note's time is split between them. The first node is drawn with half the host's length, so it takes a third of the note and the host takes two thirds.
7. The user presses a small button at the side of the build panel, which switches the build panel to the symbol editor. They sketch the ornament's symbol freehand and save it.
8. The "Publish" button, greyed out until a host node is connected, is now enabled. The user publishes the ornament, and "Lower grace" appears in the selection panel with its symbol and name tag.
9. Later the user selects "Lower grace" in the selection panel, which opens it in the build panel, and changes the first node's offset to +1. Every node that already carries this ornament picks up the change.

Applied to a node playing C4 for 1000: at velocity 100, the first node plays B3 for 333 at velocity 70, then the host plays C4 for 666.

### Applying an ornament

1. In the builder's selection panel, the user selects "Lower grace". The ornament button now shows its symbol: it is the active ornament.
2. The user closes the builder, presses on the ornament button and drags onto a node on the canvas.
3. On release over the node, the ornament is applied. Its symbol appears at the top of the node's area, and the node now plays the figure instead of its single note. While the figure plays, the node lights up as any node does; its inner notes are not shown.
4. Dropping a different ornament onto the same node replaces the first one.
5. To take the ornament off, the user right-clicks the node and chooses "Delete ornament" from its context menu, or Shift+Right-Clicks the ornament's symbol above the node.
6. A plain click on the ornament button opens the ornament window again; a drag from it always applies the active ornament.

On a chord, the user drops the ornament onto the chord's first node, its host. Only that note is ornamented. To ornament other notes of the chord, the user drops it onto each of them in turn. Modulators do not accept ornaments.

### Deleting an ornament

The user deletes an ornament from the selection panel, and the window asks them to confirm before it goes. Every node that carried it goes back to playing its plain note.

### Opening a project whose ornament is missing

A project uses "Lower grace", and is opened where the library has no ornament of that name. The node plays its plain note and shows a missing-ornament mark where the symbol would be.

## Behaviour

- **One path, no branching.** An ornament's graph is a single chain, played straight through, the same way every time.
- **Exactly one host node.** The host button is unavailable once the ornament has a host. The host marks where the node's own note falls inside the figure, so ornament nodes can come before it, after it, or both.
- **Pitch is an offset from the host's actual pitch.** Every ornament node, the host included, holds a semitone offset rather than a pitch. On a node playing C4, an ornament node at −1 plays B3. The host's offset is normally 0 (it plays the node's own note) but can be changed.
- **Velocity is a percentage of the ornamented node's velocity.** Each ornament node carries one, so a grace note at 70% on a note at velocity 100 plays at 70, and stays proportionally softer when the note is quieter.
- **Duration is a proportion, set by arrow length.** As on the main canvas, an ornament node's duration comes from the length of its arrow, so dragging nodes reshapes the figure's rhythm. The last node in the chain, often the host, gets its length from a dangling arrow, as a node at the end of a path does on the main canvas. When the ornament plays, each node gets *its duration ÷ the sum of the ornament's durations* of the ornamented node's actual duration. Designed as 500 then 1000 and applied to a note of 1000, that gives 333 and 666. The figure always fills exactly the ornamented note: never longer, never shorter.
- **Publishing.** An ornament can be published only once its host node is connected into the chain. Until then the builder can save it but nothing can use it.
- **Editing a published ornament** updates every node that carries it.
- **One button, two gestures.** A click on the ornament button opens the builder; a drag from it onto a node applies the active ornament. The button always shows the active ornament's symbol.
- **The active ornament is chosen in the selection panel.** Selecting an ornament there both opens it in the build panel for editing and makes it the one the button applies.
- **The symbol sits at the top of the node's area** on the main canvas.
- **A library across projects.** Ornaments are saved in the user's own library, not in a project, so every ornament is available in every project. Each has a name, shown as a tag under its symbol in the selection panel, where ornaments are chosen, created, renamed, duplicated and deleted.
- **Sharing.** To give someone your ornaments, you share your ornament library with them.
- **Its own window.** The ornament button opens one window holding both panels: the build panel and the selection panel.
- **The ornament button is in the bottom bar.**
- **One ornament per node.** Dropping an ornament on a node that already has one replaces it. Right-clicking the node offers "Delete ornament" in its context menu, and Shift+Right-Click on the ornament's symbol removes it directly; on the node body, Shift+Right-Click still deletes the node.
- **A retimed figure stretches.** If the ornamented note's length changes while it plays, by a tempo change or by dragging the node, the rest of the figure stretches or shrinks to keep filling the note exactly.
- **Applying and removing ornaments are canvas gestures,** so per the project rules they become a class of their own that `NodeController` routes to, not new states inside it.
- **Playback highlighting is unchanged.** An ornamented node lights up for the whole figure, as a plain node does for its note.
- **Chords are ornamented note by note.** An ornament dropped on a chord's first node ornaments that note only; each other chord note takes its own.
- **Modulators cannot carry ornaments.**
- **Deleting asks first.** Deleting an ornament from the library asks the user to confirm, and every node that carried it plays plain again.
- **A missing ornament degrades.** A node whose ornament isn't in the library plays its plain note and shows a missing-ornament mark.
- **The symbol** is drawn freehand in the symbol editor and saved as strokes, so it scales cleanly wherever it is shown. It identifies the ornament and does not affect the sound.
- **Nothing existing changes.** A graph with no ornaments sounds and looks exactly as it does today.

## Settled Decisions

| Question | Answer |
|---|---|
| How does the user design what an ornament plays? | A mini graph of regular nodes and one host node, with a button pane like the title bar's for choosing the node kind. |
| Can the ornament graph branch? | Not for now: one chain only. |
| Where does an ornament node's duration come from? | Arrow length, as on the main canvas. |
| How are pitches held? | As semitone offsets from the ornamented node's actual pitch, on every ornament node including the host. |
| How do durations apply? | As proportions of the ornamented node's actual duration. |
| How many host nodes? | Exactly one. |
| How is it published? | A Publish button, disabled until a host node is connected. A published ornament can still be edited, and every node using it updates. |
| How is the builder opened? | By clicking the ornament button, which opens the ornament window. |
| Is it a window or a panel? | Its own window, holding the build panel and the selection panel. |
| How is the builder laid out? | Build panel on the left; selection panel of every ornament on the right, each a symbol with a name tag under it. Selecting one opens it in the build panel. |
| How is the symbol drawn? | Freehand, in a symbol editor reached by a small button at the side of the build panel. |
| What else can an ornament node set? | Velocity. |
| How does the last node get its length? | From a dangling arrow, as on the main canvas. |
| Is velocity fixed or relative? | A percentage of the ornamented node's velocity, for now. |
| Where are ornaments saved? | In a library available to every project. |
| Are ornaments named and managed? | Yes: named, with the selection panel for choosing, creating, renaming, duplicating and deleting. |
| How is the active ornament chosen? | By selecting it in the selection panel; the ornament button then shows it. |
| How is an ornament applied? | By dragging from the ornament button onto a node. |
| Where does an applied ornament's symbol appear? | At the top of the node's area. |
| What does deleting an ornament do? | Asks the user to confirm; nodes that carried it play plain again. |
| How do ornaments reach another user? | The user shares their ornament library. |
| Dropping onto an ornamented node? | Replaces its ornament. One ornament per node. |
| What lights up while a figure plays? | The node, as now. Inner notes are not shown. |
| Chords? | Ornamented note by note; dropping on the chord's first node ornaments only that note. |
| Modulators? | Cannot carry ornaments. |
| A project whose ornament is missing from the library? | The node plays plain and shows a missing-ornament mark. |

| Where is the ornament button? | In the bottom bar. |
| How is an ornament taken off a node? | "Delete ornament" in the node's right-click context menu. |
| A note retimed while its figure plays? | The figure stretches to keep filling the note. |

| Is there a shortcut for removing an ornament? | Shift+Right-Click on the ornament's symbol. Shift+Right-Click on the node body still deletes the node. |

## Open Questions

None.

## Out of Scope

- **Branching ornaments**, and ornaments whose choices follow count limits. Left out for now by the owner's decision.
- **Symbols from preset shapes or a notation library.** The symbol is freehand only.

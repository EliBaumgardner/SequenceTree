# SequenceTree Scripting Language

A traversal script decides how SequenceTree walks your graph and what each step plays. You write it in the Traversal Rules window, and it runs every time a note ends.

Without a script, the walk follows the built-in rules: count limits, switch counts and sub-loops pick the next node, and that node plays its own pitch, velocity and arrow length. A script can keep all of that and change only what is played, or take over the walk entirely.

The editor recompiles your script 250 ms after you stop typing. Errors are marked on the line they occur on, and the status bar shows how many instructions the script compiled to. While a script has errors, the last version that compiled keeps playing.

- [Script structure](#script-structure)
- [Language basics](#language-basics)
- [Built-ins](#built-ins)
- [Node properties](#node-properties)
- [Limits and errors](#limits-and-errors)
- [Examples](#examples)


## Script structure

A script is one class that builds on `defaultTraversal`. Optional `import` lines come first, then the class, closed with `};`.

```
import core;

class Traversal : defaultTraversal {

public:

    Node selectedNode;

    void main() {
        selectedNode = advance(1);
        playNote(selectedNode, selectedNode.pitch, selectedNode.duration, selectedNode.velocity);
    }

};
```

The class holds two kinds of things:

- **Members** are variables declared in the class body, such as `int steps;` or `Node lastPlayed;`. They keep their value from one step to the next. A number member starts at 0 and a `Node` member starts at `none`; you cannot give a member a starting value in its declaration. A class can have up to 32 members.
- **Functions** are written as `type name(parameters) { ... }`. Use `void` for a function that returns nothing. Two names are special:
    - `void main()` runs each time a note ends. It moves the walk with `advance` and chooses what is heard with `playNote`. If a step never calls `playNote`, that step is silent but still keeps time, and the walk carries on.
    - `Node advance(int numSteps)` is optional. Write it to replace how the walk moves. Yours then owns counting, switch counts and sub-loops, and returns the node to land on, or `none` to end the walk. Without it, the built-in walk is used. Your `advance` cannot call `advance` or `playNote` itself; it only chooses where the walk goes.

Any other function you write is a helper you can call from `main` or from other functions. Calls can nest up to 8 deep. `public:` is accepted for readability and changes nothing.


## Language basics

The syntax is C-like. Every statement ends with `;`, and a block in `{ }` ends on its own. Line breaks are just whitespace, so an expression can span several lines.

### Types

| Type | Holds |
| --- | --- |
| `int` | A whole number |
| `float` | A decimal number |
| `double` | A more precise decimal number |
| `Node` | A node in your graph, or `none` |

Numbers mix freely, and the result takes the more precise type: `int` with `float` gives `float`, and anything with `double` gives `double`. Storing a decimal in an `int` drops the fraction. A `Node` and a number never mix.

### Variables

```
int steps = 0;
Node next = advance(1);
let spread = 0.25;
```

`let` takes the type of the value it is given, so it always needs one. A function can hold up to 32 local variables.

### Control flow

```
if current.count > 3 {
    ...
} else if current.count == 0 {
    ...
} else {
    ...
}

while steps < 4 {
    steps += 1;
}

for child in children {
    if child == none { continue; }
    ...
}
```

Conditions need no parentheses. A condition must be a number, where 0 is false and anything else is true; compare nodes with `==` or `!=`. `break` and `continue` work inside `while` and `for`. `return value;` leaves a function, and a `void` function uses `return;`.

`for` only walks children: `for child in children` walks the arrows leaving the current node, and `for child in someNode.children` walks those of any node.

### Operators

| Kind | Operators |
| --- | --- |
| Arithmetic | `+` `-` `*` `/` `%` |
| Comparison | `==` `!=` `<` `<=` `>` `>=` |
| Logic | `and` `or` `not` (or `&&` `\|\|` `!`) |
| Assignment | `=` `+=` `-=` |

`%` works on `int` only. Dividing by zero gives 0 rather than an error.

### Comments

`//` starts a comment that runs to the end of the line. A second `//` on the same line ends it early, so `x = 1; // note // y = 2;` still runs `y = 2;`.


## Built-ins

| Name | What it does |
| --- | --- |
| `advance(numSteps)` | Moves the walk `numSteps` nodes and returns the node it lands on, or `none` when the walk has ended. Nodes it passes over are skipped entirely and are not counted. Call it from `main`. |
| `playNote(node, pitch, duration, velocity)` | Plays the next note on `node`. Pitch and velocity are MIDI values, clamped to 0–127. Duration is in milliseconds and sets how long this note lasts. Call it from `main`. |
| `current` | The node the walk is on. It can be read but not assigned; move the walk with `advance`. |
| `children` | The arrows leaving `current`, walked with `for child in children { }`. A child the walk cannot take right now (its count limit, trigger limit or traversal rules rule it out) reads as `none`. |
| `traversal.id` | Which traversal this is. |
| `traversal.instance` | Which running copy of that traversal this is. |
| `traversal.random` | A large non-negative whole number, new on every step. Use `%` to bring it into range, as in `traversal.random % 4`. |
| `core::random(value, spread)` | A decimal spread evenly around `value`: `core::random(50, 0.5)` is anywhere from 25 to 75. Needs `import core;` at the top of the script. |


## Node properties

Read a property with a dot, as in `current.pitch` or `child.parent.id`. Only the four counts can be changed, with `=`, `+=` or `-=`. Reading a property of `none` gives `none` for `id`, `parent` and `lastChild`, and 0 for everything else.

| Property | Type | Can change | Meaning |
| --- | --- | --- | --- |
| `id` | int | No | The node's id |
| `pitch` | int | No | The node's MIDI pitch |
| `velocity` | int | No | The node's MIDI velocity |
| `duration` | int | No | How long the node lasts in milliseconds, from the length of the arrow leaving it |
| `count` | int | Yes | How many times the walk has left this node |
| `switchCount` | int | Yes | Visits held on this node by its switch count |
| `triggerCount` | int | Yes | How many times this node has triggered |
| `subLoopCount` | int | Yes | How many sub-loops this node has run |
| `limit` (or `countLimit`) | int | No | The node's count limit |
| `triggerLimit` | int | No | The node's trigger limit |
| `switchLimit` | int | No | The node's switch count |
| `subLoopLimit` | int | No | The node's sub-loop limit |
| `repeat` | int | No | The node's repeat value |
| `probability` | int | No | The node's probability setting |
| `childCount` | int | No | How many arrows leave the node |
| `eligible` | int | No | 1 for any child reached through `children`; children the walk cannot take already read as `none` |
| `lastChild` | Node | No | The child the walk last chose from this node |
| `parent` | Node | No | The node this one hangs from, or `none` for a root |


## Limits and errors

Scripts run while audio plays, so they have fixed limits. Each run of a function may take at most 8,192 instructions, which stops a loop that never ends from stalling playback. A run that goes over its budget stops where it is; if `main` had not called `playNote` yet, that step is silent.

| Limit | Value |
| --- | --- |
| Instructions per run | 8,192 |
| Members per class | 32 |
| Local variables per function | 32 |
| Nested function calls | 8 |

Compile errors are reported with the line and column they occur on, and every error in the script is reported at once. A script with errors is never played: the last version that compiled keeps running until the errors are fixed.


## Examples

Each example is a complete script you can paste into the Traversal Rules window.

### Humanised velocity

Plays every node as written, but varies its velocity by up to 20% either way.

```
import core;

class Traversal : defaultTraversal {

public:

    void main() {
        Node next = advance(1);
        double velocity = core::random(next.velocity, 0.2);

        playNote(next, next.pitch, next.duration, velocity);
    }

};
```

### Octave jump every fourth note

A member keeps counting across steps, so every fourth note is raised an octave.

```
class Traversal : defaultTraversal {

public:

    int steps;

    void main() {
        Node next  = advance(1);
        int  pitch = next.pitch;

        steps += 1;

        if steps % 4 == 0 {
            pitch += 12;
        }

        playNote(next, pitch, next.duration, next.velocity);
    }

};
```

### Random rests

About half the steps are silent. The walk still moves and keeps time on every step.

```
class Traversal : defaultTraversal {

public:

    void main() {
        Node next = advance(1);

        if traversal.random % 2 == 0 {
            playNote(next, next.pitch, next.duration, next.velocity);
        }
    }

};
```

### Random walk

Replaces the built-in walk: from each node, it moves to a random child the walk is allowed to take, and ends when there is none.

```
class Traversal : defaultTraversal {

public:

    void main() {
        Node next = advance(1);

        playNote(next, next.pitch, next.duration, next.velocity);
    }

    Node advance(int numSteps) {
        int options = 0;

        for child in children {
            if child != none {
                options += 1;
            }
        }

        if options == 0 {
            return none;
        }

        int pick = traversal.random % options;

        current.count += 1;

        for child in children {
            if child != none {
                if pick == 0 {
                    return child;
                }

                pick -= 1;
            }
        }

        return none;
    }

};
```

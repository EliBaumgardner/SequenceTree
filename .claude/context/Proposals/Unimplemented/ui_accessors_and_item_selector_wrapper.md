# UI Accessors and `ItemSelector::addItem`

> Status: Draft
> Written 2026-09-25 at 7f59922. Sources: Reviewed/ProgramAudits/software_architecture.md (What Survives: "P3: 8 accessors and the `ItemSelector::addItem` wrapper"; ledger rows 16–18)

## Summary
`refactor.smell Source/UI` finds 8 accessors that hand out a non-public field. Under the Key Design Rules, each field should be public and its accessor deleted. Two of the accessors have no callers, so those are deleted and their fields stay private. The other six fields move to public scope, and their 27 call sites are rewritten mechanically. `ItemSelector::addItem` is a one-line `push_back` wrapper. Whether it is dissolved or declared core-purpose API is for you to decide. The whole change is structural, is 3 steps, and changes no behaviour. Priority P3.

## The Problem
### Mechanism
Each accessor below returns a private member, by value or by `const&`. Findings [58]–[65] from `refactor.smell Source/UI` at 7f59922:

| # | Accessor | Field (declared) | Callers |
|---|---|---|---|
| 58 | `ButtonPane::getSelectedButton()` `Source/UI/Buttons/ButtonPane.h:79` | `selectedButton` `:171` | **none** |
| 59 | `ArrowManager::all()` `Source/UI/Canvas/ArrowManager.h:26` | `arrows` `:82` | 6: `CanvasHitTester.cpp:68, 80, 90, 100`; `NodeController.cpp:68, 89` |
| 60 | `NodeCanvas::getApplicationContext()` `Source/UI/Canvas/NodeCanvas.h:98` | `applicationContext` `:35` | 1: `ValueField.cpp:21` |
| 61 | `NodeManager::all()` `Source/UI/Canvas/NodeManager.h:29` | `nodes` `:63` | 16: `ValueField.cpp:134, 253, 373, 389`; `CanvasHitTester.cpp:114, 124, 140`; `EncapsulationView.cpp:175, 184`; `NodeCanvas.cpp:208`; `SelectionOps.cpp:37, 53, 62, 71, 729`; `NodeController.cpp:515` |
| 62 | `FileLabel::isGrabbed()` `Source/UI/Editors/FileLabel.h:26` | `grabbed` `:47` | 1: `CustomLookAndFeel_Buttons.cpp:488` |
| 63 | `FileLabel::isSelected()` `FileLabel.h:29` | `selected` `:48` | 2: `CustomLookAndFeel_Buttons.cpp:484, 495` |
| 64 | `ItemSelector::getSelectedItemId()` `Source/UI/Menus/ItemSelector.h:30` | `selectedItemId` `:62` | **none** |
| 65 | `ItemSelector::getSelectedLabel()` `ItemSelector.h:32` | `selectedLabel` `:63` | 1: `TraversalMenuListener.h:25` |

`ItemSelector::addItem` (`ItemSelector.cpp:35-38`) is `items.push_back({ itemId, std::move(label), std::move(onChosen) });`. Its callers are `Titlebar.cpp:59`, `TraversalMenu.cpp:99` and `ArrowBindBar.cpp:25-26` (4 calls at 3 sites). `items` (`ItemSelector.h:60`) and `struct Item` (`:44-48`) are private.

### Evidence
`refactor.smell Source/UI` at HEAD reports "57 wrappers · 8 accessors" and names [42] `ItemSelector::addItem`. The call sites come from a grep of `Source/` and `Tests/` at HEAD.

### Why It Matters
Key Design Rule: "the moment a member needs an accessor to be reached, it is not internal: move the member to public scope and delete the accessor." The rule is not met, and the design gate will keep reporting these until it is. It has no user-facing effect. **P3.**

## Current Design
### API Surface
As in the table. Setters that do work stay: `ButtonPane::setSelectedButton` (notifies and toggles buttons, `ButtonPane.h:62-77`), and `FileLabel::setSelected` / `setGrabbed` (early-return and `repaint()`, `FileLabel.cpp:68-86`), because they are not plain setters. `ItemSelector::setSelectedItem`, `removeItem`, `clearItems` and `findItem` keep `items` in use inside the class.

### Structure
All the classes are message-thread UI in `Source/UI/`. `NodeManager` and `ArrowManager` are parts owned by `NodeCanvas`. `ItemSelector` is a menu component used by `Titlebar`, `TraversalMenu` and `ArrowBindBar`. `FileLabel` is painted by `CustomLookAndFeel`.

`NodeCanvas.h` opens with a `private:` section that holds only `applicationContext` (`:34-35`), followed by `public:` (`:37`). Moving the field public removes that leading section.

### Data Flow
Message thread only. No audio-thread code reads any of these members.

## Approaches Considered
1. **Follow the rule literally.** Delete accessors with no callers. Move the remaining fields public and rewrite their callers. **Recommended.** Cost: 6 headers, and 27 call sites through `refactor.rewrite`.
2. **Keep the `const&` accessors as read-only views.** `all()` stops outside code from mutating `nodes` / `arrows`. The rule puts public access ahead of that protection, and keeping them needs a rule exception, which is not a planning choice. Recorded here so that the loss of `const` is a known trade.

## Plan

### Step 1 — Delete the accessors nobody calls
- **Changes:** remove `ButtonPane::getSelectedButton` and `ItemSelector::getSelectedItemId`. The fields stay private, since nothing outside reaches them.
- **Refactor commands:** `refactor.decap ButtonPane::getSelectedButton`, then `refactor.decap ItemSelector::getSelectedItemId`.
- **Behaviour delta:** none.
- **Real-time safety:** not applicable (UI only).
- **Verification:** build `SequenceTree_Standalone` and `SequenceTree_GraphTests`, then run `ctest`.
- **Rollback:** `refactor.undo` for each.

### Step 2 — Make reached fields public and dissolve their accessors
- **Changes:** one class per sub-step, each building on its own.
  - (a) `NodeManager`: move `nodes` above `private:`. Rewrite callers, then delete `all()`.
  - (b) `ArrowManager`: move `arrows` the same way, then delete `all()`.
  - (c) `NodeCanvas`: move `applicationContext` into the public section, which removes the leading `private:` (`:34`). Rewrite `ValueField.cpp:21` to `owner.applicationContext`, then delete `getApplicationContext`.
  - (d) `FileLabel`: move `grabbed` and `selected` public. Rewrite the three look-and-feel reads to `fileLabel.grabbed` / `fileLabel.selected`, then delete `isGrabbed` / `isSelected`.
  - (e) `ItemSelector`: move `selectedLabel` public. Rewrite `TraversalMenuListener.h:25` to `displayMenu.selectedLabel.isEmpty()`, then delete `getSelectedLabel`.
- **Refactor commands:** accessor bodies are one forwarding expression, so `refactor.decap` inlines each into its callers: `refactor.decap NodeManager::all`, `refactor.decap ArrowManager::all`, `refactor.decap NodeCanvas::getApplicationContext`, `refactor.decap FileLabel::isGrabbed`, `refactor.decap FileLabel::isSelected`, and `refactor.decap ItemSelector::getSelectedLabel`. Each runs after its field has moved public, because `decap` builds and the inlined `nodes` must be reachable. If `decap` refuses an inline-defined accessor, fall back to, for example, `refactor.rewrite '$M.all()' '$M.nodes' all --where 'M type NodeManager'`, and the same for `ArrowManager`, followed by `refactor.decap` to delete the accessor that no longer has callers.
- **Behaviour delta:** none. The only difference is that callers now have non-`const` access to `nodes`, `arrows`, `grabbed`, `selected` and `selectedLabel`. Writing `grabbed` / `selected` directly would skip `repaint()`. The setters stay, and no caller writes the fields.
- **Real-time safety:** not applicable.
- **Verification:** each sub-step builds `SequenceTree_Standalone` and passes `.claude/gates/readability.sh --file` on the touched header (one `public:` and one `private:`). After (e), run `SequenceTree_GraphTests`. Manual check: canvas hit-testing, drag-select, paint mode and file-label highlight in the rules window all behave as before.
- **Rollback:** `refactor.undo` per sub-step.

### Step 3 — `ItemSelector::addItem` (per Decision 1)
- **If dissolved:** move `struct Item` and `items` to public scope, then `refactor.decap ItemSelector::addItem`. The 4 calls become `selector.items.push_back({ id, label, action })`. Check that the `Action onChosen = nullptr` default still works at `TraversalMenu.cpp:99` / `ArrowBindBar.cpp:25-26`, which pass two arguments. With aggregate `push_back`, the third member is value-initialised to an empty `std::function`, which matches.
- **If declared core purpose:** add `Source/UI/Menus/ItemSelector.h:addItem` to `core_purpose_api` in `.claude/gates/design-rules.sh`. No source change.
- **Verification:** build, then open each selector (display mode in the title bar, the traversal menu, the arrow bind bar) and pick every item.
- **Rollback:** `refactor.undo`, or revert the gate list.

## Risks
- **The `decap` inlining step fails on header-defined accessors.** It would show up as a refusal or a build error. The `refactor.rewrite` fallback covers it.
- **Mutable public containers invite later misuse.** The rule accepts this. Reviews should catch it.

## Out of Scope
- The other 57 wrappers `refactor.smell Source/UI` reports, including `TraversalMenu::addTraversalToMenu` (`TraversalMenu.cpp:98-100`), which is itself a one-line wrapper over `addItem`. The review asks for these to be ranked (Question 5) before any are proposed.

## Decisions for the Owner
1. **Is `ItemSelector::addItem` core-purpose API, or a wrapper?**
   - Declaring it core purpose keeps "add an item" as the selector's vocabulary, next to `removeItem` / `clearItems`. It needs a `core_purpose_api` entry.
   - Dissolving it means `items` and `Item` go public.
   
   Recommendation: **declare it core purpose**. A selector exists to hold and choose among items, and its add/remove pair is the thing it is for. If you prefer to dissolve it, Step 3's first branch applies.
2. **Keep the `const` read-only view on `NodeManager::nodes` / `ArrowManager::arrows`?** The rule says no. Confirm you accept losing `const` on those two containers.

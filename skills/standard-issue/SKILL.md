---
name: standard-issue
description: Build and validate Standard Issue (Torogao Jam Program week 14, C++ / Unreal). Use when working on the Standard Issue jam entry — its mechanism group, C++ / Unreal build chain, single-file delivery, and port-back.
---

# Standard Issue — week 14 jam entry

**Canon core:** Earn one Certified Standard — a short exam game, Fit + Signals, administered by Piper by spoken law.
**Cast:** Piper · **Language:** C++ / Unreal (tiger-style-cpp)

## Mechanism group

- The ACHIEVEMENT-SYSTEM certification loop
- T-gate arithmetic (T2 needs Fit+Signals; T3 needs two Certified Standards)
- Permission Plug phrase sets per protocol level — the exam is administered through them
- The education layer — the exam teaches the basics it tests; safety basics are never gated, the test is the teach
- The book of record — the pass is entered, aloud and by heart

## Build

- Delivery: Named seam: UE5 has no HTML5 export. The entry is C++ compiled via Emscripten and inlined as wasm; the Unreal pairing runs at language + broker level — seahawk playtests the design inside UE 5.8 on primo, and that session is part of the week's evidence. Toolchain home: primo (Unreal lives there).
- Planning book: `book/` (mdBook; `mdbook build book`).
- Engine/library pins: book chapter "Engine Research" — read before adding
  any dependency; the from-scratch register (book chapter "Write From
  Scratch") names what must NOT be replaced by a library.

## Gates

1. bloch: start the roadmap item before work; `bloch task done` earns green.
2. Single-file assembly runs offline with all ≥6 screens.
3. Seahawk playtest pass; breakage filed to bloch.
4. Visual gate (D-221) against the Laplace3D quality bar (book chapter).
5. Port-back note + language verdict + §9 filing (book chapter "Port-Back").

## Standing law

D-292 keeper art only · audio silence (metrics only, Owner presses play) ·
18+ surfaces flagged (GJ-07) · restricted lane.

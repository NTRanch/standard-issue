# Changelog — Standard Issue

All notable changes, newest first. Format: Keep a Changelog.

## [1.0.0] — 2026-10-30

First playable release: the full W14 mechanism group, built and gated in
the sandbox under the Owner's v1.0 jam directive. The UE 5.8 pairing is
the primo follow-up (see docs/BREAKAGE-LOG.md B-3); the shipped core is
the PLANNING's named seam — C++ via the Emscripten-class route, inlined
as wasm.

- **The certification loop** (C++20, freestanding wasm32): teach →
  arm → paper → book entry. Certification as a game, not a menu.
- **The education layer**: safety basics are the first teach card of
  every paper, always shown, never gated. The test is the teach.
- **Permission Plug phrase sets**: each paper is administered through
  its plug, armed by its own phrase (MEASURE ME. / READ ME.).
- **The Fit paper**: hold the marker in a sweeping zone; a second in
  the zone is a rep; the standard asks its number.
- **The Signals paper**: a four-glyph signal is flashed and taken
  away; reproduce it by heart on the glyph pad.
- **The book of record + T-gate arithmetic**: passes are entered the
  moment they are earned; the tier is computed from the book (T2 =
  Fit + Signals; T3 shown locked with its arithmetic — it needs a
  second standard this game does not issue).
- **Three exams**: The Fit Standard, The Signals Standard, The
  T-Gate (both papers, one sitting).
- **Screens**: 18+ gate, title with the book, the brief, gameplay,
  pause, settings (exam pace, text size), victory, game over.
- **Audio**: generated WebAudio cues answering player input only.
- **Previews**: `docs/previews/level-{1,2,3}-standard-issue.mp4`,
  rendered from the shipped core in demo mode.
- **Gates**: 469 native checks (validation + adversarial) on the
  shipped translation unit, settings/book bridge checks, performance
  budget (0.010 ms/step, 17 KB wasm), scripted playtest passing all
  three exams.

## [0.1.0] — 2026-10-30

- Repo scaffolded 2026-10-30: bloch roadmap (kind=game, lane=restricted),
  planning, per-game skill, baseline C++ / Unreal config, NTRanch-themed
  mdBook, brand assets (header + icon).

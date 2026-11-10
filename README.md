> **Game page:** the full storefront page for this game lives at [docs/GAME-PAGE.md](docs/GAME-PAGE.md).

# Standard Issue

**Nice Thoughts Ranch** · Torogao Jam Program week 14 (Wave D — Features) · Piper · **C++ / Unreal**

Earn one Certified Standard — a short exam game, Fit + Signals, administered by Piper by spoken law.

An 18+ game built for the weekly Gameotron jam on itch.io (NTRanch), as one
rung of the 14-week Torogao system-validation program (D-333/D-334/D-335/D-336).
The full planning book lives in [`book/`](book/) (mdBook, NTRanch theme);
this repo's bloch roadmap is [`.ai/roadmap.md`](.ai/roadmap.md).

- **Delivery:** one self-contained `index.html` (see `docs` in the book).
- **Cast/art law (D-292):** existing keeper art only; a missing asset is an
  abstract stand-in, logged — never a generation job.
- **Quality bar:** Laplace3D-class animation or the medium's equivalent
  (book chapter: Quality Bar).
- **Audio:** nothing is ever played aloud by an agent; the Owner presses play.

## Play

Open [`web/index.html`](web/index.html) in a browser — it runs fully
offline. Every paper opens with **teaching**: safety first, always
shown, never gated — the test is the teach. Then the paper's plug:
press **B** to speak its phrase (FIT: *MEASURE ME.* — SIGNALS:
*READ ME.*) and the exam opens.

**Fit:** hold the canvas (or SPACE) to raise the marker, release to
let it settle; keep it in the sweeping zone. A second in the zone is
a rep. **Signals:** a four-glyph signal flashes and is taken away;
reproduce it on the glyph pad, by heart. Pass, and the standard is
**entered in the book of record** — the moment it is earned, aloud.

The book computes your tier: **T2 needs Fit + Signals.** T3 needs two
Certified Standards; this book holds one, and says so.

- **Exam 1 — The Fit Standard:** 8 reps inside 60 s.
- **Exam 2 — The Signals Standard:** 4 of 5 signals whole.
- **Exam 3 — The T-Gate:** both papers in one sitting — 10 reps, 4 of 5.

Settings (exam pace, text size) and the book of record persist in the
browser. Settings are reachable before any play.

## Preview videos (rendered from the shipped core, demo mode)

- [Exam 1 — The Fit Standard](docs/previews/level-1-standard-issue.mp4)
- [Exam 2 — The Signals Standard](docs/previews/level-2-standard-issue.mp4)
- [Exam 3 — The T-Gate](docs/previews/level-3-standard-issue.mp4)

## Build & verify

```sh
just build   # C++ core -> wasm + assemble web/index.html
just check   # 469 native checks, bridges, perf budget, playtest
just previews
```

Status: **v1.0.0** (2026-10-30) — playable, gated, tagged. The UE 5.8
pairing + seahawk exam session on primo and the live itch.io entry
checks (GJ-07) remain scheduled follow-ups; both are recorded in
`docs/BREAKAGE-LOG.md`.

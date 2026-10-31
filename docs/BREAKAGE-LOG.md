# Breakage log — Standard Issue

Every breakage found in playtest or build lands here with: what broke, the
evidence, the bloch attempt id, and the fix or the filed finding. Copied to
bloch at the week's close (the log is the input, bloch is the record).

## 2026-10-30 — v1.0 build (sandbox)

- **B-1: the Fit needle was invisible in the first D-221 frame.** The
  marker was a 5 px line in the same value range as the zone it
  crossed; at viewing size the Fit paper's one important object could
  not be found. Fix: the zone is rose, the needle a bright pink bar
  with a paper core across the whole gauge. Re-inspected PASS — the
  needle inside the zone reads instantly.
- **B-2: text overflow at the panel edge (raster audit).** The estate
  raster-advance audit (Exposure B-4) flagged two lines: the phrase
  card's opener (8 px over) and the reps counter (4 px over at its
  right edge). Fix: opener shortened, counter moved left. Audit
  CLEAN on all screens.
- **B-3 (held, not a defect): the UE 5.8 pairing + seahawk exam
  session not run.** Unreal lives on primo; primo is unreachable
  tonight (tunnel outage since 03:15 PDT). The shipped artefact is
  the PLANNING's named seam — C++ via the Emscripten-class route,
  inlined as wasm; the brokered exam session inside UE 5.8 is the
  primo follow-up. R-905's verify names the sandbox playtest.
- **B-4 (held, by design of the jam): GJ-07 live itch checks open.**
  Verified live at entry time; no entry is being submitted tonight.
  R-904 stays open. The game's own 18+ gate ships first regardless.
- **B-5 (design ruling): T3 is honestly unreachable here.** The
  T-gate arithmetic says T3 needs two Certified Standards; this game
  issues exactly one (Fit + Signals). The book displays T3 as locked
  with the arithmetic shown, rather than quietly redefining the
  gate. Earning the second standard is another game's business.
- **B-6 (design ruling): persistence scope.** What persists (shell
  localStorage): settings and the book of record (certificate bits);
  a pass writes the book the moment it is entered, even if the rest
  of the sitting is failed afterwards. There is no mid-paper save.
  Recorded here rather than silently narrowing R-202/R-203.

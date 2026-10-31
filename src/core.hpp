// standard-issue/src/core.hpp — Standard Issue core (C++20, freestanding).
//
// Tiger Style (tiger-style-cpp): explicit contracts, bounded work,
// disciplined ownership, no hidden behaviour. Fixed-step deterministic
// simulation; the core never reads a wall clock and reports everything
// it wants drawn as a draw list in a static buffer. The same unit
// compiles for wasm32-freestanding and the native test runner.
//
// Game: earn one Certified Standard. Piper administers the exam by
// spoken law: a teach layer first (safety basics are never gated —
// the test IS the teach), then the paper, through a Permission Plug
// armed with the paper's own phrase. Two papers — FIT (hold the
// position through counted reps) and SIGNALS (a flashed sequence,
// reproduced by heart). The T-gate arithmetic is the core's own:
// T2 requires Fit + Signals; T3 requires two Certified Standards.
// A pass is entered in the book of record, aloud and by heart.

#pragma once

namespace stdissue {

// ---------------------------------------------------------------------------
// Domain constants
// ---------------------------------------------------------------------------

inline constexpr int kCanvasPx = 960;
inline constexpr int kMaxOps = 1024;
inline constexpr int kDrawBufferBytes = 64 * 1024;
inline constexpr int kStepMs = 16;
inline constexpr int kGlyphCount = 4;
inline constexpr int kSignalLen = 4;
inline constexpr int kSignalsPerPaper = 5;

namespace palette {
inline constexpr unsigned char kBg = 0;    // #0d0208
inline constexpr unsigned char kPink = 1;  // #ff2e88
inline constexpr unsigned char kSkin = 2;  // #f8f0ed
inline constexpr unsigned char kMauve = 3; // #ac9393
inline constexpr unsigned char kPlum = 4;  // #540e46
inline constexpr unsigned char kRose = 5;  // #ec8584
inline constexpr unsigned char kPaper = 6; // #eaeaea
inline constexpr unsigned char kDim = 7;   // #68636c
inline constexpr unsigned char kGold = 8;  // #ffd166
inline constexpr unsigned char kDeep = 9;  // #1a0510
} // namespace palette

namespace input {
inline constexpr unsigned kTap = 1;
inline constexpr unsigned kConfirm = 2;
inline constexpr unsigned kBack = 4;
inline constexpr unsigned kPause = 8;
inline constexpr unsigned kUp = 16;
inline constexpr unsigned kDown = 32;
inline constexpr unsigned kArm = 64; // speak the paper's phrase
} // namespace input

namespace event {
inline constexpr unsigned kNone = 0;
inline constexpr unsigned kRep = 1;        // a Fit rep counted
inline constexpr unsigned kGlyph = 2;      // a signal glyph entered
inline constexpr unsigned kSignalDone = 3; // a signal reproduced whole
inline constexpr unsigned kArmed = 4;      // the plug arms
inline constexpr unsigned kCertified = 5;  // a standard entered in the book
inline constexpr unsigned kVictory = 6;
inline constexpr unsigned kGameOver = 7;
inline constexpr unsigned kTeach = 8;
} // namespace event

enum class Screen : unsigned {
  kGate = 0,
  kTitle = 1,
  kHowto = 2,
  kPlay = 3,
  kPause = 4,
  kSettings = 5,
  kVictory = 6,
  kGameOver = 7,
};

// Papers. Certificate bits are the book's own arithmetic.
enum class Paper : unsigned { kFit = 0, kSignals = 1 };
inline constexpr unsigned kCertFit = 1;
inline constexpr unsigned kCertSignals = 2;

// Exam phases within a sitting.
enum class Phase : unsigned {
  kTeach = 0,  // the basics, taught (safety card first, always)
  kArm = 1,    // the plug waits for the paper's phrase
  kFit = 2,    // hold the position; reps are counted
  kShow = 3,   // a signal is flashed, glyph by glyph
  kRecall = 4, // reproduce the signal
  kResult = 5, // a paper's result, spoken
};

// A sitting: which papers, in what order, at what bar.
struct Level {
  const char* name;
  Paper papers[2];
  int paper_count;
  int fit_reps;          // reps that pass the Fit paper
  int fit_ms;            // the Fit paper's clock
  int signals_to_pass;   // of kSignalsPerPaper
  long long flash_ms;    // per-glyph flash duration
};

inline constexpr Level kLevels[3] = {
    {"THE FIT STANDARD", {Paper::kFit, Paper::kFit}, 1, 8, 60000, 5, 700},
    {"THE SIGNALS STANDARD", {Paper::kSignals, Paper::kSignals}, 1, 8, 60000, 4, 700},
    {"THE T-GATE", {Paper::kFit, Paper::kSignals}, 2, 10, 75000, 4, 600},
};
inline constexpr unsigned kLevelCount = 3;

// The phrase set, one per protocol level (paper).
inline constexpr const char* kPhraseFit = "MEASURE ME.";
inline constexpr const char* kPhraseSignals = "READ ME.";

// ---------------------------------------------------------------------------
// Deterministic RNG (xorshift32; the jam cores' shared family)
// ---------------------------------------------------------------------------

struct Rng {
  unsigned state;
  explicit Rng(unsigned seed) : state(seed == 0 ? 0x9E3779B9u : seed) {}
  unsigned next() {
    unsigned x = state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    state = x;
    return x;
  }
  int range(int lo, int hi) {
    return lo + static_cast<int>(next() % static_cast<unsigned>(hi - lo + 1));
  }
};

// ---------------------------------------------------------------------------
// The game
// ---------------------------------------------------------------------------

struct Game {
  Rng rng;
  bool demo;
  Screen screen = Screen::kGate;
  long long screen_ms = 0;
  unsigned level_idx = 0;
  int menu_cursor = 0;
  unsigned title_level = 0;

  // Sitting state (reset by start_level).
  Phase phase = Phase::kTeach;
  long long phase_ms = 0;
  int paper_idx = 0;
  int teach_idx = 0;
  bool armed = false;
  bool paper_passed = false;
  int papers_passed = 0;
  // Fit paper.
  int marker_milli = 500;   // the candidate's held position, 0..1000
  long long fit_ms_left = 0;
  long long in_zone_ms = 0; // consecutive time in the zone
  int reps = 0;
  // Signals paper.
  int signal[kSignalLen];
  int signal_idx = 0;   // during show: glyphs shown so far
  int recall[kSignalLen];
  int recall_len = 0;
  int glyph_cursor = 0;
  int signals_done = 0;     // exact reproductions
  int signals_attempted = 0; // completed recalls, exact or not
  bool signal_live = false;

  // Presentation.
  const char* caption = "";
  long long caption_ms = 0;
  unsigned event_code = event::kNone;

  // Bridges (shell-persisted): settings + the book of record.
  bool gentle_pace = false;
  bool text_large = false;
  unsigned certs = 0; // kCertFit | kCertSignals, the book itself

  Game(unsigned seed, bool demo_mode);
  bool start_level(unsigned idx);
  void step(unsigned dt_ms, unsigned input_bits);
  unsigned screen_id() const { return static_cast<unsigned>(screen); }
  // The T-gate arithmetic, computed by the core from the book:
  // T1 = any one paper; T2 = Fit + Signals; T3 = two standards (this
  // game issues one standard, so T3 stays arithmetically locked here).
  unsigned tier() const {
    if ((certs & kCertFit) && (certs & kCertSignals)) return 2;
    if (certs != 0) return 1;
    return 0;
  }
  int fit_zone_lo() const; // the zone is a function of exam time
  int fit_zone_hi() const;
  Paper current_paper() const;
  int teach_cards() const;
  int render(char* buf, int cap) const;

 private:
  void set_screen(Screen s);
  void set_caption(const char* text, long long ms);
  void begin_paper();
  void begin_signal();
  void finish_paper();
  void finish_sitting();
  unsigned demo_policy();
  void step_play(long long dt, unsigned inp);
  void step_title(unsigned inp);
  void step_pause(unsigned inp);
  void step_settings(unsigned inp);
};

} // namespace stdissue

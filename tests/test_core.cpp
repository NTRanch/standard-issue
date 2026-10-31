// standard-issue/tests/test_core.cpp — native test runner for the core.
// Half validation / half adversarial, per estate test law. Exit code is
// the gate: 0 = all pass. Build: clang++ -std=c++20 (see justfile).

#include "../src/core.hpp"

#include <cstdio>
#include <cstring>
#include <string>

using namespace stdissue;

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond)                                                     \
  do {                                                                  \
    g_checks++;                                                         \
    if (!(cond)) {                                                      \
      g_failures++;                                                     \
      std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);       \
    }                                                                   \
  } while (0)

static void steps(Game& g, int n, unsigned inp = 0) {
  for (int i = 0; i < n; i++) g.step(kStepMs, inp);
}

static void through_teach(Game& g) {
  int guard = 0;
  while (g.phase == Phase::kTeach && guard++ < 1000) g.step(kStepMs, input::kConfirm);
  CHECK(g.phase == Phase::kArm);
}

static void arm(Game& g) {
  g.step(kStepMs, input::kArm);
  CHECK(g.armed);
}

// Play the Fit paper with the demo's own tracking rule, by hand.
static void play_fit(Game& g) {
  int guard = 0;
  while (g.phase == Phase::kFit && guard++ < 20000) {
    int center = (g.fit_zone_lo() + g.fit_zone_hi()) / 2;
    g.step(kStepMs, g.marker_milli < center ? input::kTap : 0);
  }
}

// Reproduce the current signal exactly, through real input.
static void play_signal(Game& g) {
  int guard = 0;
  while (g.phase == Phase::kShow && guard++ < 2000) g.step(kStepMs, 0);
  CHECK(g.phase == Phase::kRecall);
  guard = 0;
  while (g.phase == Phase::kRecall && guard++ < 500) {
    int want = g.signal[g.recall_len];
    if (g.glyph_cursor < want) {
      g.step(kStepMs, input::kDown);
    } else if (g.glyph_cursor > want) {
      g.step(kStepMs, input::kUp);
    } else {
      g.step(kStepMs, input::kConfirm);
    }
  }
}

static void play_signals_paper(Game& g) {
  int guard = 0;
  while (g.phase != Phase::kResult && guard++ < 100) {
    play_signal(g);
  }
}

static Game play_game(unsigned level, unsigned seed = 42) {
  Game g(seed, false);
  CHECK(g.start_level(level));
  return g;
}

// -- validation ---------------------------------------------------------

static void test_teach_then_arm_then_paper() {
  Game g = play_game(0);
  CHECK(g.phase == Phase::kTeach);
  through_teach(g);
  arm(g);
  CHECK(g.phase == Phase::kFit);
}

static void test_fit_reps_are_counted_and_pass() {
  Game g = play_game(0);
  through_teach(g);
  arm(g);
  play_fit(g);
  CHECK(g.phase == Phase::kResult);
  CHECK(g.paper_passed);
  CHECK(g.reps >= kLevels[0].fit_reps);
  CHECK((g.certs & kCertFit) != 0); // entered in the book at once
}

static void test_signals_reproduction_passes() {
  Game g = play_game(1);
  through_teach(g);
  arm(g);
  play_signals_paper(g);
  CHECK(g.phase == Phase::kResult);
  CHECK(g.paper_passed);
  CHECK(g.signals_done >= kLevels[1].signals_to_pass);
  CHECK((g.certs & kCertSignals) != 0);
}

static void test_t_gate_arithmetic_is_the_cores() {
  Game g(9, false);
  CHECK(g.tier() == 0);
  g.certs = kCertFit;
  CHECK(g.tier() == 1);
  g.certs = kCertSignals;
  CHECK(g.tier() == 1);
  g.certs = kCertFit | kCertSignals;
  CHECK(g.tier() == 2);
  // T3 needs two standards; this book can only ever hold one.
  CHECK(g.tier() < 3);
}

static void test_full_t_gate_sitting_certifies() {
  Game g = play_game(2);
  through_teach(g);
  arm(g);
  play_fit(g);
  steps(g, 200, 0); // result -> paper 2 teach
  CHECK(g.phase == Phase::kTeach);
  through_teach(g);
  arm(g);
  play_signals_paper(g);
  steps(g, 200, 0); // result -> sitting ends
  CHECK(g.screen == Screen::kVictory);
  CHECK(g.tier() == 2);
}

static void test_demo_completes_all_levels() {
  for (unsigned lvl = 0; lvl < kLevelCount; lvl++) {
    Game g(9000 + lvl, true);
    CHECK(g.start_level(lvl));
    int n = 0;
    while (g.screen != Screen::kVictory && g.screen != Screen::kGameOver && n < 40000) {
      g.step(kStepMs, 0);
      n++;
    }
    CHECK(g.screen == Screen::kVictory);
    CHECK(g.tier() == (lvl == 2 ? 2u : 1u));
  }
}

// -- adversarial --------------------------------------------------------

static void test_exam_stays_locked_without_the_phrase() {
  Game g = play_game(0);
  through_teach(g);
  // Everything except B: the plug does not care.
  steps(g, 100, input::kConfirm);
  steps(g, 100, input::kTap);
  steps(g, 100, input::kDown);
  CHECK(g.phase == Phase::kArm);
  CHECK(!g.armed);
}

static void test_wrong_signal_is_not_counted() {
  Game g = play_game(1);
  through_teach(g);
  arm(g);
  steps(g, 2000, 0); // through the show phase (bounded below)
  int guard = 0;
  while (g.phase == Phase::kShow && guard++ < 4000) g.step(kStepMs, 0);
  CHECK(g.phase == Phase::kRecall);
  // Enter a deliberately wrong sequence: every glyph + 1.
  guard = 0;
  while (g.phase == Phase::kRecall && guard++ < 500) {
    int want = (g.signal[g.recall_len] + 1) % kGlyphCount;
    if (g.glyph_cursor < want) {
      g.step(kStepMs, input::kDown);
    } else if (g.glyph_cursor > want) {
      g.step(kStepMs, input::kUp);
    } else {
      g.step(kStepMs, input::kConfirm);
    }
  }
  CHECK(g.signals_done == 0);
  CHECK(g.signals_attempted == 1);
}

static void test_fit_clock_expiry_fails_the_paper() {
  Game g = play_game(0);
  through_teach(g);
  arm(g);
  g.fit_ms_left = 32;
  steps(g, 4, 0);
  CHECK(g.phase == Phase::kResult);
  CHECK(!g.paper_passed);
  CHECK((g.certs & kCertFit) == 0);
}

static void test_book_keeps_a_pass_when_the_sitting_fails() {
  Game g = play_game(2);
  through_teach(g);
  arm(g);
  play_fit(g); // Fit entered
  CHECK((g.certs & kCertFit) != 0);
  steps(g, 200, 0);
  through_teach(g);
  arm(g);
  // Fail Signals on purpose: wrong glyphs every time.
  int guard = 0;
  while (g.phase != Phase::kResult && guard++ < 200) {
    int inner = 0;
    while (g.phase == Phase::kShow && inner++ < 4000) g.step(kStepMs, 0);
    inner = 0;
    while (g.phase == Phase::kRecall && inner++ < 500) {
      int want = (g.signal[g.recall_len] + 1) % kGlyphCount;
      if (g.glyph_cursor < want) {
        g.step(kStepMs, input::kDown);
      } else if (g.glyph_cursor > want) {
        g.step(kStepMs, input::kUp);
      } else {
        g.step(kStepMs, input::kConfirm);
      }
    }
  }
  steps(g, 200, 0);
  CHECK(g.screen == Screen::kGameOver);
  CHECK((g.certs & kCertFit) != 0); // the book kept the Fit entry
  CHECK(g.tier() == 1);
}

static void test_start_level_rejects_out_of_range() {
  Game g(3, false);
  CHECK(!g.start_level(kLevelCount));
  CHECK(!g.start_level(4000000000u));
  CHECK(g.screen == Screen::kGate);
}

static void test_gentle_pace_widens_the_zone() {
  Game a = play_game(0);
  Game b(42, false);
  b.gentle_pace = true;
  CHECK(b.start_level(0));
  int wa = a.fit_zone_hi() - a.fit_zone_lo();
  int wb = b.fit_zone_hi() - b.fit_zone_lo();
  CHECK(wb > wa);
}

static void test_determinism_same_seed_same_frame() {
  auto run = []() {
    Game g(777, false);
    g.start_level(2);
    unsigned x = 12345;
    for (int i = 0; i < 3000; i++) {
      x = x * 1103515245u + 12345u;
      g.step(kStepMs, (x >> 16) % 128);
    }
    static char buf[kDrawBufferBytes];
    int n = g.render(buf, kDrawBufferBytes);
    return std::string(buf, buf + n);
  };
  CHECK(run() == run());
}

static void test_draw_lists_well_formed_on_all_screens() {
  auto check = [](Game& g) {
    static char buf[kDrawBufferBytes];
    int n = g.render(buf, kDrawBufferBytes);
    CHECK(n > 0 && n < kDrawBufferBytes);
    int ops = 0;
    for (int i = 0; i < n; i++) {
      if (buf[i] == '\n') {
        ops++;
        CHECK(i + 1 >= n || buf[i + 1] == 'r' || buf[i + 1] == 'c' || buf[i + 1] == 'l' ||
              buf[i + 1] == 'p' || buf[i + 1] == 't');
      }
    }
    CHECK(ops <= kMaxOps);
  };
  Game g(11, false);
  check(g); // gate
  g.step(kStepMs, input::kConfirm);
  check(g); // title (with the book)
  g.step(kStepMs, input::kDown);
  g.step(kStepMs, input::kConfirm);
  CHECK(g.screen == Screen::kHowto);
  check(g);
  g.step(kStepMs, input::kBack);
  g.step(kStepMs, input::kDown);
  g.step(kStepMs, input::kDown);
  g.step(kStepMs, input::kConfirm);
  CHECK(g.screen == Screen::kSettings);
  check(g);
  g.start_level(0);
  check(g); // teach
  through_teach(g);
  check(g); // arm
  arm(g);
  steps(g, 30, input::kTap);
  check(g); // fit
  g.step(kStepMs, input::kPause);
  CHECK(g.screen == Screen::kPause);
  check(g); // pause
}

int main() {
  test_teach_then_arm_then_paper();
  test_fit_reps_are_counted_and_pass();
  test_signals_reproduction_passes();
  test_t_gate_arithmetic_is_the_cores();
  test_full_t_gate_sitting_certifies();
  test_demo_completes_all_levels();
  test_exam_stays_locked_without_the_phrase();
  test_wrong_signal_is_not_counted();
  test_fit_clock_expiry_fails_the_paper();
  test_book_keeps_a_pass_when_the_sitting_fails();
  test_start_level_rejects_out_of_range();
  test_gentle_pace_widens_the_zone();
  test_determinism_same_seed_same_frame();
  test_draw_lists_well_formed_on_all_screens();
  std::printf("standard-issue core tests: %d checks, %d failures\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}

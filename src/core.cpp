// standard-issue/src/core.cpp — implementation. See core.hpp.

#include "core.hpp"

namespace stdissue {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static int str_len(const char* s) {
  int n = 0;
  while (s[n] != '\0') n++;
  return n;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

Game::Game(unsigned seed, bool demo_mode) : rng(seed), demo(demo_mode) {}

void Game::set_screen(Screen s) {
  screen = s;
  screen_ms = 0;
  menu_cursor = 0;
}

void Game::set_caption(const char* text, long long ms) {
  caption = text;
  caption_ms = ms;
}

Paper Game::current_paper() const { return kLevels[level_idx].papers[paper_idx]; }

int Game::teach_cards() const { return 3; } // safety, basics, the phrase

bool Game::start_level(unsigned idx) {
  if (idx >= kLevelCount) return false;
  level_idx = idx;
  paper_idx = 0;
  papers_passed = 0;
  begin_paper();
  set_caption("THE EXAM BEGINS WITH THE TEACHING. IT ALWAYS DOES.", 3000);
  set_screen(Screen::kPlay);
  return true;
}

void Game::begin_paper() {
  phase = Phase::kTeach;
  phase_ms = 0;
  teach_idx = 0;
  armed = false;
  paper_passed = false;
  reps = 0;
  in_zone_ms = 0;
  marker_milli = 500;
  fit_ms_left = kLevels[level_idx].fit_ms;
  signals_done = 0;
  signals_attempted = 0;
  signal_live = false;
  recall_len = 0;
}

void Game::begin_signal() {
  for (int i = 0; i < kSignalLen; i++) signal[i] = rng.range(0, kGlyphCount - 1);
  // A signal of four identical glyphs teaches nothing; redraw it.
  if (signal[0] == signal[1] && signal[1] == signal[2] && signal[2] == signal[3]) {
    signal[3] = (signal[3] + 1) % kGlyphCount;
  }
  signal_idx = 0;
  recall_len = 0;
  glyph_cursor = 0;
  signal_live = true;
  phase = Phase::kShow;
  phase_ms = 0;
}

void Game::finish_paper() {
  if (paper_passed) {
    papers_passed++;
    if (current_paper() == Paper::kFit) {
      certs |= kCertFit;
    } else {
      certs |= kCertSignals;
    }
    event_code = event::kCertified;
    set_caption("ENTERED IN THE BOOK. ALOUD, AND BY HEART.", 2400);
  } else {
    set_caption("NOT YET. THE BOOK STAYS OPEN.", 2400);
  }
  phase = Phase::kResult;
  phase_ms = 0;
}

void Game::finish_sitting() {
  const Level& lvl = kLevels[level_idx];
  if (papers_passed >= lvl.paper_count) {
    event_code = event::kVictory;
    set_screen(Screen::kVictory);
  } else {
    event_code = event::kGameOver;
    set_screen(Screen::kGameOver);
  }
}

// The Fit zone sweeps on a triangle wave (period 8 s) across the gauge.
int Game::fit_zone_lo() const {
  const Level& lvl = kLevels[level_idx];
  long long elapsed = lvl.fit_ms - fit_ms_left;
  long long t = elapsed % 8000;
  long long tri = t < 4000 ? t : 8000 - t;
  int center = 240 + static_cast<int>(tri * 520 / 4000);
  int half = gentle_pace ? 150 : 110;
  return center - half;
}

int Game::fit_zone_hi() const {
  const Level& lvl = kLevels[level_idx];
  long long elapsed = lvl.fit_ms - fit_ms_left;
  long long t = elapsed % 8000;
  long long tri = t < 4000 ? t : 8000 - t;
  int center = 240 + static_cast<int>(tri * 520 / 4000);
  int half = gentle_pace ? 150 : 110;
  return center + half;
}

// ---------------------------------------------------------------------------
// Stepping
// ---------------------------------------------------------------------------

void Game::step(unsigned dt_ms, unsigned raw_input) {
  long long dt = dt_ms > 50 ? 50 : static_cast<long long>(dt_ms);
  event_code = event::kNone;
  unsigned inp = demo ? demo_policy() : raw_input;
  screen_ms += dt;
  if (caption_ms > 0) caption_ms -= dt;
  switch (screen) {
    case Screen::kGate:
      if (inp & (input::kConfirm | input::kTap)) set_screen(Screen::kTitle);
      break;
    case Screen::kTitle:
      step_title(inp);
      break;
    case Screen::kHowto:
      if (inp & (input::kBack | input::kConfirm)) set_screen(Screen::kTitle);
      break;
    case Screen::kPlay:
      step_play(dt, inp);
      break;
    case Screen::kPause:
      step_pause(inp);
      break;
    case Screen::kSettings:
      step_settings(inp);
      break;
    case Screen::kVictory:
    case Screen::kGameOver:
      if (inp & (input::kConfirm | input::kTap)) set_screen(Screen::kTitle);
      break;
  }
}

void Game::step_title(unsigned inp) {
  if (inp & input::kDown) menu_cursor = menu_cursor < 3 ? menu_cursor + 1 : 3;
  if (inp & input::kUp) menu_cursor = menu_cursor > 0 ? menu_cursor - 1 : 0;
  if (inp & (input::kConfirm | input::kTap)) {
    if (menu_cursor == 0) {
      start_level(title_level);
    } else if (menu_cursor == 1) {
      set_screen(Screen::kHowto);
    } else if (menu_cursor == 2) {
      set_screen(Screen::kSettings);
    } else {
      title_level = (title_level + 1) % kLevelCount;
    }
  }
}

void Game::step_pause(unsigned inp) {
  if (inp & input::kDown) menu_cursor = menu_cursor < 2 ? menu_cursor + 1 : 2;
  if (inp & input::kUp) menu_cursor = menu_cursor > 0 ? menu_cursor - 1 : 0;
  if (inp & input::kPause) {
    set_screen(Screen::kPlay);
    return;
  }
  if (inp & (input::kConfirm | input::kTap)) {
    if (menu_cursor == 0) {
      set_screen(Screen::kPlay);
    } else if (menu_cursor == 1) {
      start_level(level_idx);
    } else {
      set_screen(Screen::kTitle);
    }
  }
}

void Game::step_settings(unsigned inp) {
  if (inp & input::kDown) menu_cursor = menu_cursor < 2 ? menu_cursor + 1 : 2;
  if (inp & input::kUp) menu_cursor = menu_cursor > 0 ? menu_cursor - 1 : 0;
  if (inp & input::kBack) {
    set_screen(Screen::kTitle);
    return;
  }
  if (inp & (input::kConfirm | input::kTap)) {
    if (menu_cursor == 0) {
      gentle_pace = !gentle_pace;
    } else if (menu_cursor == 1) {
      text_large = !text_large;
    } else {
      set_screen(Screen::kTitle);
    }
  }
}

void Game::step_play(long long dt, unsigned inp) {
  if (inp & input::kPause) {
    set_screen(Screen::kPause);
    return;
  }
  const Level& lvl = kLevels[level_idx];
  phase_ms += dt;
  switch (phase) {
    case Phase::kTeach: {
      bool adv = (inp & (input::kConfirm | input::kTap)) != 0 || phase_ms >= 3000;
      if (adv) {
        teach_idx++;
        phase_ms = 0;
        event_code = event::kTeach;
        if (teach_idx >= teach_cards()) {
          phase = Phase::kArm;
          phase_ms = 0;
        }
      }
      break;
    }
    case Phase::kArm:
      if (inp & input::kArm) {
        armed = true;
        event_code = event::kArmed;
        set_caption("THE PHRASE IS SPOKEN. THE EXAM OPENS.", 1600);
        if (current_paper() == Paper::kFit) {
          phase = Phase::kFit;
          phase_ms = 0;
        } else {
          begin_signal();
        }
      }
      break;
    case Phase::kFit: {
      if (inp & input::kTap) {
        marker_milli += 4;
        if (marker_milli > 1000) marker_milli = 1000;
      } else {
        marker_milli -= 3;
        if (marker_milli < 0) marker_milli = 0;
      }
      fit_ms_left -= dt;
      if (marker_milli >= fit_zone_lo() && marker_milli <= fit_zone_hi()) {
        in_zone_ms += dt;
        if (in_zone_ms >= 1000) {
          reps++;
          in_zone_ms = 0;
          event_code = event::kRep;
          set_caption("COUNTED. HOLD THE NEXT.", 1200);
          if (reps >= lvl.fit_reps) {
            paper_passed = true;
            finish_paper();
            break;
          }
        }
      } else {
        in_zone_ms = 0;
      }
      if (fit_ms_left <= 0) {
        fit_ms_left = 0;
        paper_passed = false;
        finish_paper();
      }
      break;
    }
    case Phase::kShow: {
      long long flash = lvl.flash_ms * (gentle_pace ? 3 : 2) / 2;
      long long per = flash + 180;
      signal_idx = static_cast<int>(phase_ms / per);
      if (phase_ms >= per * kSignalLen) {
        phase = Phase::kRecall;
        phase_ms = 0;
      }
      break;
    }
    case Phase::kRecall: {
      // The recall pad: arrows choose the glyph, confirm enters it.
      if (inp & input::kDown) glyph_cursor = (glyph_cursor + 1) % kGlyphCount;
      if (inp & input::kUp) glyph_cursor = (glyph_cursor + kGlyphCount - 1) % kGlyphCount;
      if (inp & (input::kConfirm | input::kTap)) {
        recall[recall_len++] = glyph_cursor;
        event_code = event::kGlyph;
        if (recall_len >= kSignalLen) {
          bool exact = true;
          for (int i = 0; i < kSignalLen; i++) {
            if (recall[i] != signal[i]) exact = false;
          }
          signal_live = false;
          signals_attempted++;
          if (exact) {
            signals_done++;
            event_code = event::kSignalDone;
            set_caption("THE SIGNAL, WHOLE. BY HEART.", 1500);
          } else {
            set_caption("NOT THE SIGNAL. IT IS TAUGHT AGAIN BY DOING.", 1500);
          }
          if (signals_attempted >= kSignalsPerPaper) {
            paper_passed = signals_done >= lvl.signals_to_pass;
            finish_paper();
          } else {
            begin_signal();
          }
        }
      }
      break;
    }
    case Phase::kResult:
      if (phase_ms >= 2200) {
        paper_idx++;
        if (paper_idx >= lvl.paper_count) {
          finish_sitting();
        } else {
          begin_paper();
        }
      }
      break;
  }
}

// ---------------------------------------------------------------------------
// The demo policy: a prepared candidate. It holds the Fit position on
// the zone's centre and reproduces signals it was shown.
// ---------------------------------------------------------------------------

unsigned Game::demo_policy() {
  switch (screen) {
    case Screen::kGate:
      return screen_ms > 600 ? input::kConfirm : 0;
    case Screen::kTitle:
      return screen_ms > 700 ? input::kConfirm : 0;
    case Screen::kVictory:
    case Screen::kGameOver:
      return screen_ms > 1500 ? input::kConfirm : 0;
    case Screen::kPlay:
      break;
    default:
      return 0;
  }
  switch (phase) {
    case Phase::kTeach:
      return phase_ms > 400 ? input::kConfirm : 0;
    case Phase::kArm:
      return input::kArm;
    case Phase::kFit: {
      int center = (fit_zone_lo() + fit_zone_hi()) / 2;
      return marker_milli < center ? input::kTap : 0;
    }
    case Phase::kRecall: {
      int want = signal[recall_len];
      if (glyph_cursor < want) return input::kDown;
      if (glyph_cursor > want) return input::kUp;
      return input::kConfirm;
    }
    default:
      return 0;
  }
}

} // namespace stdissue

// ---------------------------------------------------------------------------
// Draw-list emission (bounded writer) + rendering
// ---------------------------------------------------------------------------

namespace stdissue {
namespace {

struct StrBuf {
  char bytes[160];
  int len = 0;
  void push_str(const char* s) {
    for (int i = 0; s[i] != '\0' && len < 159; i++) bytes[len++] = s[i];
    bytes[len] = '\0';
  }
  void push_int(long long v) {
    if (v < 0) {
      push_str("-");
      v = -v;
    }
    char digits[20];
    int n = 0;
    if (v == 0) digits[n++] = '0';
    while (v > 0) {
      digits[n++] = static_cast<char>('0' + (v % 10));
      v /= 10;
    }
    for (int i = n - 1; i >= 0; i--) {
      if (len < 159) bytes[len++] = digits[i];
    }
    bytes[len] = '\0';
  }
  const char* c_str() const { return bytes; }
};

struct Writer {
  char* buf;
  int cap;
  int pos = 0;
  int ops = 0;
  int dropped = 0;
  Writer(char* b, int c) : buf(b), cap(c) {}
  void raw(const char* s) {
    int n = str_len(s);
    if (pos + n > cap) {
      dropped++;
      return;
    }
    for (int i = 0; i < n; i++) buf[pos++] = s[i];
  }
  void num(int v) {
    StrBuf b;
    b.push_int(v);
    raw(b.c_str());
  }
  bool begin_op(const char* kind) {
    if (ops >= kMaxOps) {
      dropped++;
      return false;
    }
    ops++;
    raw(kind);
    return true;
  }
  void field(int v) {
    raw(" ");
    num(v);
  }
  void rect(int x, int y, int w, int h, int c) {
    if (!begin_op("r")) return;
    field(x); field(y); field(w); field(h); field(c);
    raw("\n");
  }
  void circle(int x, int y, int r, int c) {
    if (!begin_op("c")) return;
    field(x); field(y); field(r); field(c);
    raw("\n");
  }
  void line(int x1, int y1, int x2, int y2, int c, int w) {
    if (!begin_op("l")) return;
    field(x1); field(y1); field(x2); field(y2); field(c); field(w);
    raw("\n");
  }
  void tri(int x1, int y1, int x2, int y2, int x3, int y3, int c) {
    if (!begin_op("p")) return;
    field(x1); field(y1); field(x2); field(y2); field(x3); field(y3); field(c);
    raw("\n");
  }
  void text(int x, int y, int size, int c, const char* s) {
    if (!begin_op("t")) return;
    field(x); field(y); field(size); field(c);
    raw(" ");
    raw(s);
    raw("\n");
  }
};

int tsize(const Game& g, int base) { return g.text_large ? base * 5 / 4 : base; }

void render_room(Writer& w) {
  w.rect(0, 0, kCanvasPx, kCanvasPx, palette::kBg);
  for (int i = 0; i < 40; i++) {
    w.circle((i * 397 + 53) % 940 + 10, (i * 613 + 91) % 940 + 10, 2, palette::kDim);
  }
  w.line(40, 845, 920, 845, palette::kPlum, 3);
}

void render_figure(Writer& w) {
  const int cx = 250;
  const int head_y = 390;
  w.tri(cx - 110, 845, cx + 110, 845, cx, 500, palette::kPlum);
  w.rect(cx - 48, 484, 96, 86, palette::kRose);
  w.circle(cx - 78, head_y - 78, 34, palette::kPlum);
  w.circle(cx + 78, head_y - 78, 34, palette::kPlum);
  w.circle(cx, head_y - 18, 98, palette::kPlum);
  w.circle(cx, head_y, 86, palette::kSkin);
  w.circle(cx - 44, head_y + 28, 13, palette::kRose);
  w.circle(cx + 44, head_y + 28, 13, palette::kRose);
  w.circle(cx - 29, head_y - 6, 7, palette::kBg);
  w.circle(cx + 29, head_y - 6, 7, palette::kBg);
  w.line(cx - 44, head_y - 30, cx - 16, head_y - 30, palette::kPlum, 4);
  w.line(cx + 16, head_y - 30, cx + 44, head_y - 30, palette::kPlum, 4);
  w.line(cx - 11, head_y + 48, cx + 11, head_y + 48, palette::kPlum, 4);
  w.rect(cx - 24, 470, 48, 10, palette::kGold);
}

void draw_glyph(Writer& w, int glyph, int cx, int cy, int r, int c) {
  switch (glyph) {
    case 0: // ring
      w.circle(cx, cy, r, c);
      w.circle(cx, cy, r * 2 / 3, palette::kBg);
      break;
    case 1: // triangle
      w.tri(cx, cy - r, cx - r, cy + r, cx + r, cy + r, c);
      break;
    case 2: // square
      w.rect(cx - r, cy - r, r * 2, r * 2, c);
      break;
    default: // diamond
      w.tri(cx, cy - r, cx - r, cy, cx + r, cy, c);
      w.tri(cx, cy + r, cx - r, cy, cx + r, cy, c);
      break;
  }
}

void render_play(const Game& g, Writer& w) {
  const Level& lvl = kLevels[g.level_idx];
  render_room(w);
  render_figure(w);
  w.text(36, 28, tsize(g, 26), palette::kPaper, lvl.name);
  StrBuf paper_line;
  paper_line.push_str("PAPER ");
  paper_line.push_int(g.paper_idx + 1);
  paper_line.push_str(" OF ");
  paper_line.push_int(lvl.paper_count);
  paper_line.push_str(g.current_paper() == Paper::kFit ? " - FIT" : " - SIGNALS");
  w.text(36, 64, tsize(g, 20), palette::kMauve, paper_line.c_str());
  StrBuf tier_line;
  tier_line.push_str("THE BOOK: T");
  tier_line.push_int(static_cast<long long>(g.tier()));
  w.text(700, 28, tsize(g, 20), palette::kGold, tier_line.c_str());
  const int px = 440;
  switch (g.phase) {
    case Phase::kTeach: {
      bool fit = g.current_paper() == Paper::kFit;
      if (g.teach_idx == 0) {
        w.text(px, 150, tsize(g, 28), palette::kGold, "SAFETY FIRST");
        w.text(px, 230, tsize(g, 20), palette::kPaper, "THE BASICS ARE NEVER GATED.");
        w.text(px, 270, tsize(g, 20), palette::kPaper, "A WORD STOPS ANYTHING, ANY TIME.");
        w.text(px, 310, tsize(g, 20), palette::kPaper, "THE EXAM TEACHES WHAT IT TESTS.");
        w.text(px, 350, tsize(g, 20), palette::kPaper, "NOTHING HERE CAN HURT YOU.");
      } else if (g.teach_idx == 1 && fit) {
        w.text(px, 150, tsize(g, 28), palette::kGold, "THE FIT PAPER");
        w.text(px, 230, tsize(g, 20), palette::kPaper, "HOLD THE MARKER IN THE MOVING ZONE.");
        w.text(px, 270, tsize(g, 20), palette::kPaper, "HOLD (TAP) TO RISE. RELEASE TO SETTLE.");
        w.text(px, 310, tsize(g, 20), palette::kPaper, "ONE SECOND IN THE ZONE IS ONE REP.");
        StrBuf need;
        need.push_str("THE STANDARD ASKS ");
        need.push_int(lvl.fit_reps);
        need.push_str(" REPS.");
        w.text(px, 350, tsize(g, 20), palette::kPaper, need.c_str());
      } else if (g.teach_idx == 1) {
        w.text(px, 150, tsize(g, 28), palette::kGold, "THE SIGNALS PAPER");
        w.text(px, 230, tsize(g, 20), palette::kPaper, "A SIGNAL IS FLASHED, GLYPH BY GLYPH.");
        w.text(px, 270, tsize(g, 20), palette::kPaper, "THEN IT IS GONE. REPRODUCE IT BY HEART.");
        w.text(px, 310, tsize(g, 20), palette::kPaper, "ARROWS CHOOSE A GLYPH. ENTER SETS IT.");
        StrBuf need;
        need.push_str("THE STANDARD ASKS ");
        need.push_int(lvl.signals_to_pass);
        need.push_str(" OF ");
        need.push_int(kSignalsPerPaper);
        need.push_str(" WHOLE.");
        w.text(px, 350, tsize(g, 20), palette::kPaper, need.c_str());
      } else {
        w.text(px, 150, tsize(g, 28), palette::kGold, "THE PHRASE");
        w.text(px, 230, tsize(g, 20), palette::kPaper, "ADMINISTERED THROUGH THE PLUG.");
        w.text(px, 270, tsize(g, 20), palette::kPaper, "ITS PHRASE IS:");
        w.text(px, 330, tsize(g, 34), palette::kPink, fit ? kPhraseFit : kPhraseSignals);
        w.text(px, 400, tsize(g, 20), palette::kPaper, "PRESS B TO SPEAK IT AND OPEN THE EXAM.");
      }
      StrBuf card;
      card.push_str("CARD ");
      card.push_int(g.teach_idx + 1);
      card.push_str(" OF ");
      card.push_int(g.teach_cards());
      w.text(px, 540, tsize(g, 20), palette::kMauve, card.c_str());
      break;
    }
    case Phase::kArm:
      w.text(px, 200, tsize(g, 28), palette::kGold, "THE PLUG IS LOCKED");
      w.text(px, 280, tsize(g, 20), palette::kPaper, "SPEAK THE PAPER'S PHRASE TO OPEN THE EXAM:");
      w.text(px, 340, tsize(g, 34), palette::kPink,
             g.current_paper() == Paper::kFit ? kPhraseFit : kPhraseSignals);
      w.text(px, 430, tsize(g, 20), palette::kRose, "PRESS B");
      break;
    case Phase::kFit: {
      // The gauge: zone band + marker needle.
      const int gx = 560;
      const int gy = 150;
      const int gh = 520;
      auto y_of = [&](int milli) { return gy + gh - (milli * gh / 1000); };
      w.rect(gx, gy, 90, gh, palette::kDeep);
      w.rect(gx, y_of(g.fit_zone_hi()), 90, y_of(g.fit_zone_lo()) - y_of(g.fit_zone_hi()),
             palette::kRose);
      // The needle: a bright bar across the whole gauge, impossible
      // to miss against the zone and the frame alike.
      w.rect(gx - 18, y_of(g.marker_milli) - 4, 126, 8, palette::kPink);
      w.line(gx - 18, y_of(g.marker_milli), gx + 108, y_of(g.marker_milli), palette::kPaper, 2);
      w.line(gx, gy, gx, gy + gh, palette::kDim, 2);
      w.line(gx + 90, gy, gx + 90, gy + gh, palette::kDim, 2);
      StrBuf reps_line;
      reps_line.push_str("REPS ");
      reps_line.push_int(g.reps);
      reps_line.push_str(" OF ");
      reps_line.push_int(lvl.fit_reps);
      w.text(680, 200, tsize(g, 26), palette::kPaper, reps_line.c_str());
      StrBuf clock;
      clock.push_str("");
      clock.push_int((g.fit_ms_left + 999) / 1000);
      clock.push_str("S");
      w.text(680, 250, tsize(g, 26), palette::kMauve, clock.c_str());
      // The counted second, filling.
      if (g.in_zone_ms > 0) {
        w.rect(700, 320, static_cast<int>(g.in_zone_ms * 180 / 1000), 14, palette::kGold);
      }
      w.text(700, 360, tsize(g, 20), palette::kMauve, "THE COUNT");
      w.text(px, 720, tsize(g, 18), palette::kDim, "HOLD TO RISE - RELEASE TO SETTLE");
      break;
    }
    case Phase::kShow: {
      w.text(px, 150, tsize(g, 26), palette::kGold, "WATCH");
      long long flash = lvl.flash_ms * (g.gentle_pace ? 3 : 2) / 2;
      long long per = flash + 180;
      long long into = g.phase_ms % per;
      int idx = static_cast<int>(g.phase_ms / per);
      if (idx < kSignalLen && into < flash) {
        draw_glyph(w, g.signal[idx], 640, 400, 90, palette::kPink);
      }
      for (int i = 0; i < kSignalLen; i++) {
        int c = i < idx ? palette::kPink : (i == idx ? palette::kGold : palette::kDim);
        w.circle(560 + i * 54, 580, 10, c);
      }
      break;
    }
    case Phase::kRecall: {
      w.text(px, 150, tsize(g, 26), palette::kGold, "BY HEART, THEN");
      for (int i = 0; i < g.recall_len; i++) {
        draw_glyph(w, g.recall[i], 500 + i * 90, 300, 30, palette::kPaper);
      }
      for (int i = g.recall_len; i < kSignalLen; i++) {
        w.circle(500 + i * 90, 300, 8, palette::kDim);
      }
      for (int gi = 0; gi < kGlyphCount; gi++) {
        int cx = 500 + gi * 110;
        if (gi == g.glyph_cursor) {
          w.rect(cx - 46, 430, 92, 92, palette::kPlum);
        }
        draw_glyph(w, gi, cx, 476, 30, gi == g.glyph_cursor ? palette::kPink : palette::kPaper);
      }
      StrBuf done_line;
      done_line.push_str("SIGNALS WHOLE: ");
      done_line.push_int(g.signals_done);
      done_line.push_str(" OF ");
      done_line.push_int(kSignalsPerPaper);
      w.text(px, 580, tsize(g, 20), palette::kMauve, done_line.c_str());
      w.text(px, 640, tsize(g, 18), palette::kDim, "ARROWS CHOOSE - ENTER SETS THE GLYPH");
      break;
    }
    case Phase::kResult:
      w.text(px, 240, tsize(g, 30), g.paper_passed ? palette::kPink : palette::kMauve,
             g.paper_passed ? "THE STANDARD IS MET" : "NOT YET");
      w.text(px, 320, tsize(g, 20), palette::kPaper,
             g.paper_passed ? "IT IS ENTERED IN THE BOOK." : "THE BOOK STAYS OPEN. SIT AGAIN.");
      break;
  }
  if (g.caption_ms > 0) {
    w.text(90, 862, tsize(g, 20), palette::kRose, g.caption);
  }
}

void render_book(const Game& g, Writer& w, int x, int y) {
  w.text(x, y, tsize(g, 26), palette::kGold, "THE BOOK OF RECORD");
  w.text(x, y + 50, tsize(g, 20), palette::kPaper,
         (g.certs & kCertFit) ? "FIT - ENTERED" : "FIT - NOT YET ENTERED");
  w.text(x, y + 84, tsize(g, 20), palette::kPaper,
         (g.certs & kCertSignals) ? "SIGNALS - ENTERED" : "SIGNALS - NOT YET ENTERED");
  StrBuf t;
  t.push_str("TIER: T");
  t.push_int(static_cast<long long>(g.tier()));
  w.text(x, y + 130, tsize(g, 22), palette::kPink, t.c_str());
  w.text(x, y + 168, tsize(g, 18), palette::kMauve, "T2 NEEDS FIT + SIGNALS. T3 NEEDS TWO STANDARDS.");
  w.text(x, y + 196, tsize(g, 18), palette::kMauve, "THIS BOOK HOLDS ONE STANDARD. T3 STAYS LOCKED HERE.");
}

void render_gate(const Game& g, Writer& w) {
  w.rect(0, 0, kCanvasPx, kCanvasPx, palette::kBg);
  w.text(190, 250, tsize(g, 60), palette::kPink, "STANDARD ISSUE");
  w.text(190, 350, tsize(g, 26), palette::kPaper, "18+ ONLY BEYOND THIS GATE");
  w.text(190, 410, tsize(g, 19), palette::kMauve, "AN ADULT EXAM GAME, BY NICE THOUGHTS RANCH.");
  w.text(190, 445, tsize(g, 19), palette::kMauve, "CERTIFICATION AS A GAME: FIT + SIGNALS, BY SPOKEN LAW.");
  w.text(190, 480, tsize(g, 19), palette::kMauve, "EVERYTHING IS ABSTRACT AND SYMBOLIC. NO ONE IS REAL.");
  w.text(190, 560, tsize(g, 20), palette::kGold, "PRESS ENTER OR TAP TO CONFIRM YOU ARE 18+");
}

void render_title(const Game& g, Writer& w) {
  render_room(w);
  w.text(190, 120, tsize(g, 56), palette::kPink, "STANDARD ISSUE");
  w.text(190, 200, tsize(g, 20), palette::kMauve, "EARN ONE CERTIFIED STANDARD - FIT + SIGNALS");
  const char* items[3] = {"BEGIN THE EXAM", "THE BRIEF", "SETTINGS"};
  for (int i = 0; i < 3; i++) {
    int y = 300 + i * 60;
    int c = g.menu_cursor == i ? palette::kPink : palette::kPaper;
    if (g.menu_cursor == i) w.text(150, y, tsize(g, 32), palette::kPink, ">");
    w.text(190, y, tsize(g, 32), c, items[i]);
  }
  int y = 300 + 3 * 60;
  if (g.menu_cursor == 3) w.text(150, y, tsize(g, 32), palette::kPink, ">");
  StrBuf row;
  row.push_str("EXAM: ");
  row.push_str(kLevels[g.title_level].name);
  w.text(190, y, tsize(g, 32), palette::kPaper, row.c_str());
  w.text(190, y + 44, tsize(g, 20), palette::kMauve, "TAP ROW TO CHANGE");
  render_book(g, w, 190, 620);
  w.text(190, 900, tsize(g, 18), palette::kDim, "ARROWS CHOOSE - ENTER CONFIRMS");
}

void render_howto(const Game& g, Writer& w) {
  w.rect(0, 0, kCanvasPx, kCanvasPx, palette::kBg);
  w.text(110, 90, tsize(g, 40), palette::kPink, "THE BRIEF");
  const char* lines[] = {
      "SAFETY FIRST - THE BASICS ARE NEVER GATED.",
      "A WORD STOPS ANYTHING, ANY TIME. THE EXAM TEACHES",
      "THE BASICS IT TESTS. THAT IS THE EDUCATION LAYER.",
      "",
      "EACH PAPER OPENS WITH TEACHING, THEN ITS PLUG:",
      "SPEAK THE PAPER'S PHRASE (B) TO ARM AND BEGIN.",
      "",
      "FIT: HOLD THE MARKER IN THE MOVING ZONE. A SECOND",
      "IN THE ZONE IS A REP. THE STANDARD ASKS ITS NUMBER.",
      "",
      "SIGNALS: A SEQUENCE IS FLASHED AND TAKEN AWAY.",
      "REPRODUCE IT BY HEART ON THE GLYPH PAD.",
      "",
      "PASS AND IT IS ENTERED IN THE BOOK OF RECORD.",
      "T2 NEEDS FIT + SIGNALS. T3 NEEDS TWO STANDARDS.",
  };
  for (int i = 0; i < 16; i++) {
    w.text(110, 175 + i * 42, tsize(g, 20), palette::kPaper, lines[i]);
  }
  w.text(110, 860, tsize(g, 20), palette::kGold, "ENTER OR ESC TO RETURN");
}

void render_settings(const Game& g, Writer& w) {
  w.rect(0, 0, kCanvasPx, kCanvasPx, palette::kBg);
  w.text(240, 160, tsize(g, 44), palette::kPink, "SETTINGS");
  StrBuf a;
  a.push_str("EXAM PACE: ");
  a.push_str(g.gentle_pace ? "GENTLE" : "STANDARD");
  StrBuf b;
  b.push_str("TEXT: ");
  b.push_str(g.text_large ? "LARGE" : "NORMAL");
  const char* rows[3] = {a.c_str(), b.c_str(), "BACK"};
  for (int i = 0; i < 3; i++) {
    int y = 300 + i * 70;
    int c = g.menu_cursor == i ? palette::kPink : palette::kPaper;
    if (g.menu_cursor == i) w.text(200, y, tsize(g, 32), palette::kPink, ">");
    w.text(240, y, tsize(g, 32), c, rows[i]);
  }
  w.text(240, 560, tsize(g, 20), palette::kMauve, "SETTINGS ARE NEVER GATED. THEY APPLY AT ONCE");
  w.text(240, 590, tsize(g, 20), palette::kMauve, "AND ARE REMEMBERED BETWEEN VISITS.");
}

void render_pause(const Game& g, Writer& w) {
  render_play(g, w);
  w.rect(230, 260, 500, 400, palette::kDeep);
  w.text(350, 300, tsize(g, 44), palette::kPink, "HELD");
  const char* items[3] = {"RESUME", "RESTART EXAM", "QUIT TO TITLE"};
  for (int i = 0; i < 3; i++) {
    int y = 400 + i * 64;
    int c = g.menu_cursor == i ? palette::kPink : palette::kPaper;
    if (g.menu_cursor == i) w.text(310, y, tsize(g, 32), palette::kPink, ">");
    w.text(350, y, tsize(g, 32), c, items[i]);
  }
}

void render_end(const Game& g, Writer& w, bool won) {
  render_room(w);
  if (won) {
    w.text(170, 170, tsize(g, 40), palette::kPink, "CERTIFIED STANDARD");
    w.text(170, 240, tsize(g, 20), palette::kPaper, "ISSUED, ENTERED, AND READ ALOUD.");
  } else {
    w.text(170, 170, tsize(g, 40), palette::kPaper, "NOT YET CERTIFIED");
    w.text(170, 240, tsize(g, 20), palette::kMauve, "THE BOOK STAYS OPEN. THE EXAM TEACHES AGAIN.");
  }
  render_book(g, w, 170, 330);
  w.text(170, 640, tsize(g, 20), palette::kGold, "ENTER OR TAP TO RETURN");
}

} // namespace

int Game::render(char* buf, int cap) const {
  Writer w(buf, cap);
  switch (screen) {
    case Screen::kGate: render_gate(*this, w); break;
    case Screen::kTitle: render_title(*this, w); break;
    case Screen::kHowto: render_howto(*this, w); break;
    case Screen::kPlay: render_play(*this, w); break;
    case Screen::kPause: render_pause(*this, w); break;
    case Screen::kSettings: render_settings(*this, w); break;
    case Screen::kVictory: render_end(*this, w, true); break;
    case Screen::kGameOver: render_end(*this, w, false); break;
  }
  return w.pos;
}

} // namespace stdissue

// ---------------------------------------------------------------------------
// FFI surface: the whole contract with the shell.
// ---------------------------------------------------------------------------

#if defined(__wasm__)
namespace {
stdissue::Game* g_game = nullptr;
char g_draw[stdissue::kDrawBufferBytes];
int g_draw_len = 0;

stdissue::Game& game_ref() {
  if (g_game == nullptr) {
    static stdissue::Game fallback(1, false);
    return fallback;
  }
  return *g_game;
}

void render_global() {
  stdissue::Game& g = game_ref();
  g_draw_len = g.render(g_draw, stdissue::kDrawBufferBytes);
}
} // namespace

extern "C" {

__attribute__((export_name("init"))) unsigned init(unsigned seed, unsigned level) {
  if (level >= stdissue::kLevelCount) return 1;
  static stdissue::Game g(seed, false);
  g = stdissue::Game(seed, false);
  g.title_level = level;
  g_game = &g;
  render_global();
  return 0;
}

__attribute__((export_name("init_demo"))) unsigned init_demo(unsigned seed, unsigned level) {
  if (level >= stdissue::kLevelCount) return 1;
  static stdissue::Game g(seed, true);
  g = stdissue::Game(seed, true);
  g.title_level = level;
  g_game = &g;
  render_global();
  return 0;
}

__attribute__((export_name("start_level"))) unsigned start_level(unsigned idx) {
  bool ok = game_ref().start_level(idx);
  render_global();
  return ok ? 0u : 1u;
}

__attribute__((export_name("step"))) void step(unsigned dt_ms, unsigned bits) {
  game_ref().step(dt_ms, bits);
  render_global();
}

__attribute__((export_name("screen"))) unsigned screen() { return game_ref().screen_id(); }

__attribute__((export_name("level_idx"))) unsigned level_idx() { return game_ref().level_idx; }

__attribute__((export_name("apply_settings"))) void apply_settings(unsigned gentle, unsigned large) {
  stdissue::Game& g = game_ref();
  g.gentle_pace = gentle != 0;
  g.text_large = large != 0;
  render_global();
}

__attribute__((export_name("settings_bits"))) unsigned settings_bits() {
  stdissue::Game& g = game_ref();
  return (g.gentle_pace ? 1u : 0u) | (g.text_large ? 2u : 0u);
}

__attribute__((export_name("apply_certs"))) void apply_certs(unsigned certs) {
  game_ref().certs = certs & 3u;
  render_global();
}

__attribute__((export_name("certs_bits"))) unsigned certs_bits() { return game_ref().certs; }

__attribute__((export_name("stat_tier"))) unsigned stat_tier() { return game_ref().tier(); }

__attribute__((export_name("event_code"))) unsigned event_code() { return game_ref().event_code; }

__attribute__((export_name("stat_reps"))) unsigned stat_reps() {
  return static_cast<unsigned>(game_ref().reps);
}

__attribute__((export_name("draw_ptr"))) const char* draw_ptr() { return g_draw; }

__attribute__((export_name("draw_len"))) unsigned draw_len() {
  return static_cast<unsigned>(g_draw_len);
}

} // extern "C"
#endif // defined(__wasm__)

// playtest.mjs — automated playtest: the demo policy sits every exam
// and passes it against the shipped wasm core (the T-Gate sitting ends
// at tier T2 in the book). This is the sandbox playtest of record; the
// seahawk pass on primo is the follow-up named in docs/BREAKAGE-LOG.md.
import { readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { createGame } from "../web/game.mjs";

const root = join(dirname(fileURLToPath(import.meta.url)), "..");
const wasmBytes = readFileSync(join(root, "build/stdissue.wasm"));
const SCREENS = ["gate", "title", "howto", "play", "pause", "settings", "victory", "gameover"];

let failures = 0;
for (let level = 0; level < 3; level++) {
  const game = await createGame(wasmBytes, { seed: 9000 + level, level, demo: true });
  game.startLevel(level);
  let steps = 0;
  const maxSteps = (6 * 60 * 1000) / 16;
  while (game.screen() !== 6 && game.screen() !== 7 && steps < maxSteps) {
    game.step(0);
    steps++;
  }
  const won = game.screen() === 6;
  const wantTier = level === 2 ? 2 : 1;
  console.log(`level ${level + 1}: screen=${SCREENS[game.screen()]} steps=${steps} certs=${game.certsBits()} tier=${game.tier()} ${won ? "VICTORY" : "FAIL"}`);
  if (!won || game.tier() !== wantTier) failures++;
}
if (failures) {
  console.error(`playtest: ${failures} exam(s) not passed by the demo policy`);
  process.exit(1);
}
console.log("playtest: all 3 exams passed; the T-Gate sitting certifies T2");

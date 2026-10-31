// settings-check.mjs — gates R-202/R-203/R-403: the settings AND book
// bridges round-trip, and the settings screen is reachable from boot
// with no progression (safety/accessibility options are never gated —
// and neither is the safety brief).
import { readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { createGame, INPUT } from "../web/game.mjs";

const root = join(dirname(fileURLToPath(import.meta.url)), "..");
const wasmBytes = readFileSync(join(root, "build/stdissue.wasm"));
const game = await createGame(wasmBytes, { seed: 7, level: 0, demo: false });

function steps(n, bits = 0) { for (let i = 0; i < n; i++) game.step(bits); }
function fail(msg) { console.error(`settings-check: ${msg}`); process.exit(1); }

if (game.screen() !== 0) fail("boot screen is not the 18+ gate");
game.applySettings(true, false);
if (game.settingsBits() !== 1) fail("apply_settings(gentle_pace) did not round-trip");
game.applySettings(false, true);
if (game.settingsBits() !== 2) fail("apply_settings(text_large) did not round-trip");
game.applySettings(true, true);

// The book bridge: certificates restore, and the tier is computed.
game.applyCerts(1);
if (game.certsBits() !== 1 || game.tier() !== 1) fail("book bridge: FIT alone should read T1");
game.applyCerts(3);
if (game.certsBits() !== 3 || game.tier() !== 2) fail("book bridge: FIT + SIGNALS should read T2");
game.applyCerts(0);

// Navigate gate -> title -> settings with plain input, no play first.
steps(1, INPUT.confirm);
if (game.screen() !== 1) fail("confirm did not leave the gate");
steps(1, INPUT.down); steps(1, INPUT.down);
steps(1, INPUT.confirm);
if (game.screen() !== 5) fail(`settings not reachable from title (screen=${game.screen()})`);
steps(2);
if (!game.frameText().includes("GENTLE")) fail("settings screen does not show restored GENTLE pace");
if (!game.frameText().includes("LARGE")) fail("settings screen does not show restored LARGE text");
console.log("settings-check: settings + book bridges round-trip; tier arithmetic reads through the bridge; settings reachable ungated");

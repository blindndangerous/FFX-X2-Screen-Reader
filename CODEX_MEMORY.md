# FFX current memory — November16

November16 mergecheck integration: working files resolve all six conflicts between
detached `3e7aa54` and `feat/mod-toggle` `31844f4`. Both guide fallback and pending
guide requests remain; navigation keeps pressure-episode suppression and worker
failure/recovery packets. The controlled message-mode callback retains the raw
`0x46C840` guard. Kilika diagnostics and the `4245b9d` load-popup revision fix remain.
New toggle fixtures include pre-main dialog suppression. The runner has one
evidence-gated navigation-service block with the swimming implementation linked.
Navigation query/capture entry points reject Off before speech or native reads;
a focused regression failed before the guard and passes in the final suite.
No-deploy build exits 0. Full native suite exits 0: 101 PASS records plus proxy
forwarding, 4 evidence SKIPs, 0 failures. Separate ATEL run exits 0 with 2 PASS
records. Missing evidence: archived reader body, collision replay body, Kilika
floor inputs and utility maps. Actual game speech/timing remains unverified.
Receipts: `.local/merge-build.log`, `.local/merge-tests.log`,
`.local/merge-atel-tests.log`, `.local/merge-verification.json`.
Git staging failed with permission denied creating the shared worktree metadata's
`index.lock`; its directory lies outside this session's writable root. The index
still records six unmerged paths, and no merge commit exists. Finish by staging
the resolved files and committing with `.local/merge-commit-message.txt` in a
session permitted to write this worktree's Git metadata. No deploy, push or other
worktree changes. Luna performed a read-only fixture/runner path audit; the parent
reviewed its findings, made all edits and ran verification. No delegated issues
remain. See the [runtime integration record](Docs/features/runtime/index.md#mergecheck-integration--november16).

November16 current-area toggle repair is offline verified, uncommitted on
`feat/mod-toggle` at base `41cef74`. The user reports Load-dialog Off/On now
works, but field helpers fail after enabling in Besaid Great Hall. The supplied
log had restarted at 18:56:44 when read; the earlier reported sequence was absent.
No deployment, commit, game control/probe or Tester Logs access.

Cause: toggles advanced the allocation lifetime floor, Off hid native ATEL
receipts, and resource-worker cleanup could reclaim still-live prepared banks.
All hooks previously skipped allocation/free observation while Off. Native
capture failure then latched its one waiting message; F9/F11 context gates also
rejected requests without a ready trial identity. Resource lifetime observers
and native load scopes now stay active while Off, with bounded native capture
and worker decoding. Frees/replacements still retire exact serials. Speech,
commands, movement and navigation snapshots retain generation cancellation;
current-area field/puzzle state is recaptured after enable without another load.
Worker failure packets report each waiting command, then one readiness notice.
Pending navigation admits keyboard guide requests; ordinary ready-field key
handback and supported-guide-only controller availability remain intact.

Focused fixtures reproduced receipt loss, skipped Off lifetime/load callbacks,
the silent second query and blocked F9. Current-area navigation/guide, area load
while Off, address reuse, cache pressure, worker cleanup and readiness recovery
have targeted coverage. No-deploy build exits 0; full native suite exits 0 with
91 PASS records / 3 SKIP / 0 failures (prior 86/3). Separate ATEL resource fixture
passes, including zero native heap operations and zero worker engine reads. Resume fixture 17
checks and hook ABI 11000 mixed calls pass. Warm same-area toggle through first
navigation output measured 19.004 ms in controlled fixture scheduling; cold
copied pipeline 15.794 ms. Actual game timing and audible NVDA remain unverified.
Warnings: C4244 conversion, C4459 shadow declarations and suite D9026 remain.
Skips: archived reader body, collision replay body and utility maps evidence
are absent. Receipts use
`build/field-toggle-*.log`. Follow the updated
[runtime contract and prioritized test procedure](Docs/features/runtime/index.md#whole-mod-toggle--november16).
The earlier fresh-area-load limitation below is superseded by this repair.


November16 toggle speech follow-up is offline verified, uncommitted on
`feat/mod-toggle` at `43bd042`. Claude's injected-key allowance in
`src/native/mod_toggle.cpp:19` is preserved. The user's supplied 18:29 session
log was read; no deployment, commit, game launch/control or Tester Logs access.
Toggle feedback used a reserved path that discarded `interrupt=true` and
logged neither its text nor its backend result. Generation/Off gates did not
reject that path in the real speech-worker fixture; the exact original live
Off loss remains unproven. Both confirmations now interrupt and log backend
acceptance. Automatic resume lines queue until the next input, fixing the
one-line protection that let Yes interrupt On. Resume requires a valid field
before announcing its map; navigation capture excludes title/save-load room23.
Fresh Yes/No callbacks read unchanged focus or focus changed while Off.

Requested no-deploy build exits0. Full `src/native/tests/run_tests.bat` exits0:
86 PASS records /3 SKIP /0 failures (prior84/3). Menu fixture414 checks;
new native-owner resume fixture15 checks. Speech fixtures cover exact feedback
interruption, multiple resume lines, input restoring interruption, Off capture/API
rejection, delayed Off-generation results, full queues and cleared repeat history.
Red fixtures reproduced feedback interruption, false map arrival and false
title navigation failure before their fixes. Existing Kilika C4244, fixture
shadow and D9026 warnings remain. Skips: archived reader body, collision replay
body and utility maps evidence are absent. Receipts: `build/toggle-build.log`,
`build/toggle-suite.log`, `build/toggle-*-red.log`; final runtime sequence in
`build/toggle-tests/runtime_test.log`. Real NVDA audio remains unverified;
ATEL helpers still need a fresh observed resource load after enable.
See the updated [runtime contract and prioritized dialog test](Docs/features/runtime/index.md#whole-mod-toggle--november16).

Earlier implementation checkpoint:

November16 whole-mod toggle is implemented, uncommitted in `feat/mod-toggle`
(`ffx-wt-modtoggle`; base `0d7166e`). Ctrl+Shift+F12 toggles the default-On mod
using one flag, persistent hooks, native-owner cancellation/key handback and
fresh state arrival. FFX-2 gets the shared toggle listener, not screen reading.
The standard native suite exits0:84 PASS /3 SKIP (baseline81/3); separate ATEL
and hook-threading fixtures pass, including full stale receipt recovery and
controller/speech-pressure replay prevention. Final no-deploy build exits0;
the final three input fixtures pass after the held-key rearm repair. Existing
Kilika C4244 and fixture shadow warnings remain. Full changed-file/gate locations
and receipts: `build/modtoggle-report.txt` (local artifact).
ATEL-dependent helpers need a fresh observed resource load after re-enable;
live gameplay, physical key/audio response and FFX-2 remain unverified. No
commit, deployment, game/probe or Tester Logs access. See the
[architecture, exact recovery limit and prioritized gameplay procedure](Docs/features/runtime/index.md#whole-mod-toggle--november16).

November16 late-session floor/guide/fixture follow-up on detached `0ba4a49`.
The user confirms that this deployed revision's homing and one-cue push speech
work. Exact read-only extraction of `klyt/klyt10/bin/mapout.vpa` reproduces the
273.708-unit stair route and 0.129274 first-gap fraction. The native completion
opens group 53 and blocks group 55; no supported floor joins Y -10 to Y +10 at
the pedestal. The user clarified that nobody crossed directly. Routing and
all drop checks remain unchanged. The retained diagnostic classifies these
long approaches as informational height changes, retaining the full gap data.
Shift+F11 outside trials now says "No puzzle guide for this area." Manual
navigation requests before observation startup also explain unavailable data.
All 92 fixture entry sources embed pre-main process-local error-mode, CRT-abort
and WER no-UI suppression. The service fixture's unmapped message-mode getter
jumped to raw 0x46C840; the old adapter reproduces an access violation there.
Its controlled callback/window adapters and current semantic endpoints are
repaired; the default suite now includes it. Five deliberate hidden failures
verify suppression independently of runner inheritance, including CRT assertions.
Final no-deploy build and full native suite both exit 0: 96 PASS groups plus
proxy forwarding; three missing-evidence skips (readers, reachability, utilities).
The Windows `small` macro collision was also repaired. Full receipts are in
`.local/three-verification.json` and the linked record. No deployment, commit,
game control, probe, packaging or Tester Logs access. See the [root causes, final verification,
changed locations and prioritized nonvisual procedure](Docs/features/navigation/kilika_floor_guide_fixtures_20261116.md).


November16 Kilika detour/speech follow-up on detached `b3d6d9d`, using the user's
19:38–19:40 live log from `4245b9d`: the long step-16 route remains unexplained.
Retained event data lacks the actual collision floor; the synthetic inactive-
pedestal/reset-obstacle retirement fixtures pass. No speculative route change.
Requested routes now emit bounded owned-worker diagnostics when length exceeds
twice direct distance or direct distance plus six units: request/data age,
start/destination/waypoints, floor components, effective/copied obstacle and
live/inactive/reset sources, endpoint eligibility and straight-line blockers.
Automatic push arrival and turn-help producers caused redundant speech. Matching
owned automatic sessions now suppress arrival/camera prose, speak direction
once per leg and retain the existing completion. Interrupted-plan renumbering
also preserves the spoken-leg state; manual help remains. Final no-deploy build
and full native suite exit 0: 90 PASS groups plus proxy forwarding, three existing
missing-evidence SKIPs (readers, reachability, utilities). All 384 arrival-ramp
and 96 pedestal-ramp cases pass. Existing C4244/C4456/D9026 warnings remain.
Diagnostic-only maximum measured cost: 47.445 ms on a synthetic 32,767-triangle
floor; short-route maximum 0.001 ms. Full live latency/audio remain unverified.
Receipts: `.local/detour-verification.json`, `.local/detour-build-final.log`,
`.local/detour-tests-final.log`, `.local/detour-benchmark.log`. Luna supplied one
read-only retained-data locator; Sol owns all edits/review/verification.
No deployment, commit, game control/probe or Tester Logs access. See the
[cause, changed locations, limits and prioritized nonvisual procedure](Docs/features/navigation/index.md#kilika-detour-diagnostics-and-automatic-push-speech--november16).

November16 late-run arrival follow-up on detached `4245b9d`: the user's supplied
Entry door/Fire chamber log confirms failure after the 300–333ms settling phase;
it does not reveal the preceding homing duration or exit branch. The original
walker fails a corrected cold native-ramp fixture starting 4.5 units outside:
33ms frames, 10 units/second, one-frame input latency, Z=16.996 at 3333ms where
the zone begins at Z=17. Its fixed 3000ms cutoff and original-endpoint steering
are replaced by a cached worker correction to the nearest reachable zone point,
actual-membership settling, a 3000ms no-progress deadline and a 15000ms safety
cap. Outside-zone failures now capture geometry, nearest-point validity,
distance, homing/total/no-progress timing and exit reason through owned logging.
The 384-case ramp matrix and disconnected-floor failure case pass; longest
synthetic success 4059ms. Final no-deploy build and full native suite exit 0:
90 PASS groups plus proxy forwarding, 3 existing missing-evidence SKIPs
(readers, reachability, utilities). Existing C4244/C4456/D9026 warnings remain.
Receipts: `.local/homing-verification.json`, `.local/homing-build-final.log`,
`.local/homing-tests-final.log`, `.local/homing-baseline-result.log`. Gameplay and
audible output remain unverified. No deployment, commit, game control/probe or
Tester Logs access. See the [cause, limits and prioritized nonvisual procedure](Docs/features/navigation/index.md#kilika-arrival-homing-follow-up--november16).

November16 evening-run repair on `e79200a`: step22 now names the actual worker37
row, `Exit 1 to Kilika - Antechamber`. Step21 requires both the retained0x63=3
milestone and an empty0x62 recess, and returns after reinsertion. Ordinary
arrival requires actual zone membership, permits safe correction past a distant
pedestal keep-out, and allows3000ms correction plus final settling. Popup revisions
reject obsolete Yes/No returns and their duplicate loading Autodetail. No-deploy
build/full native suite exit0:90 PASS groups plus proxy forwarding,3 existing
missing-evidence SKIPs (readers, reachability, utilities). All96 arrival cases
and407 menu checks pass; longest synthetic success2970ms, blocked recovery3366ms.
Existing C4244/C4456/D9026 warnings remain. Receipts: `.local/kilika-build-final.log`,
`.local/kilika-tests-final.log`, `.local/kilika-verification.json`. Gameplay and
audible speech remain unverified.
Step19's initial long detour remains unresolved: inactive-pedestal retirement
passes, and the later `9 southwest` was camera-cut speech, not a replan. Missing
live floor/route geometry prevents proving whether the original detour was needed.
The supplied log rotated during work; its matching archive was recovered and
preserved under `%TEMP%/ffx-kilika-e79200a/original-session.log`. No deployment,
commit, game control/probe or Tester Logs access. See the
[causes, limits, verification and prioritized gameplay procedure](Docs/features/navigation/index.md#kilika-evening-run-repairs--november16).

November16 visible-message hint repair on `4dc7d1d` separates native message mode
from an open TK window with a nonempty text source. Guide close-message hints
and Auto walk's message-open refusal now require that copied visible-text fact;
script/control holds without text use scene/push wait wording. Movement guards
and the existing speech producer remain unchanged. The fixture runner suppresses
Windows crash dialogs in child processes; external evidence loads also set the
error mode before possible pre-main exceptions. No-deploy build and full native
suite exit 0: 89 PASS groups plus proxy forwarding, 3 existing missing-evidence
SKIPs (readers, reachability, utilities). Regression coverage includes 192 TK
visibility cases and 32 production guide cases; the old guide branch fails.
Installer tests pass 667 assertions and UI compilation; PRISM-dependent cases
remain skipped. Existing C4244/C4456/D9026 warnings remain. Receipts:
`.local/message-build.log`, `.local/message-tests.log`,
`.local/message-installer-tests.log`, `.local/message-verification.json`.
Gameplay/rendered visibility/audible speech remain unverified. No deployment,
commit, game access or Tester Logs access occurred. See the
[cause, offline limits and prioritized procedure](Docs/features/navigation/index.md#visible-message-hints--november16).

November16 steps16–17 repair follows the user's live confirmation that `19cca78`
completes all three step15 legs and says “Pedestal push complete.” Inactive
contact pedestals no longer propagate or retain ordinary-target keep-outs.
Ordinary assisted talk routes defer arrival speech until native settling
confirms the current interaction zone. Their bounded correction uses completed
pulse/coast calibration and a 2000ms home budget; step15 retains its existing
input, timing and speech. A synthetic32-case native-ramp matrix passes with one
outcome per case, longest simulated recovery2376ms; this is not gameplay proof.
Busy notices remain latched through intermittent captures until full drain or
observation revision change. Explicit failed routes log player/native floor IDs,
components, target-zone polygons and obstacle sources on the owned worker.
Final no-deploy build and full native suite exit0:87 PASS groups,3 existing
missing-evidence SKIPs (readers, reachability, utilities). The192-case step15
route/input matrix and96-case step15 ramp matrix remain green. Final receipts:
`.local/r18-build-final.log`, `.local/r18-tests-final.log` and
`.local/r18-verification.json`. Existing C4244/C4456/D9026 warnings remain.
No commit, deployment, packaging, game or Tester Logs access occurred. See the
[cause, changed files, offline limits and prioritized procedure](Docs/features/navigation/index.md#kilika-steps-16-17-after-the-completed-push--november16).

November16 step-15 input-path investigation supersedes the prior live-success
assumptions. Retained native Installed82D180 has four 0–256 directional ramps,
32/frame with deadzone48; the old instantaneous plant missed movement retained
after reversals. Axial staging/contact now brakes and pulses through those ramps;
detours, push legs and speech remain unchanged. A bounded emission trace records
actual keys/axes, timing and both navigation/native camera-vector yaws. Camera
agreement, local walls and live consumption timing remain unverified; staging
has no facing-angle gate. The corrected fixture fails against HEAD376a236 and
passes96 cold/warm ramp cases plus camera-change/cancel/restart/trace checks.
The existing192-case route/input matrix passes. Final full native suite exits0:
85 PASS groups,3 existing missing-evidence SKIPs; no-deploy build exits0.
Receipts: `.local/r17-tests-final.log`, `.local/r17-build-final.log`. Existing
C4244/C4456/D9026 warnings remain. Independent Sol review findings are repaired
and tested; no further runtime defect was found. No commit, deployment or game
access occurred or is authorized. See the
[input-path evidence, limits and prioritized procedure](Docs/features/navigation/index.md#kilika-step-15-native-input-ramp-repair--november16).

November16 step-15 redesign supersedes the failed live waypoint-hold repair.
The radial entry now leads into an axial staging suffix; the per-frame walker
owns staging/contact handoff and releases movement in the transition tick.
Missed intermediate holds and stopped approaches request bounded silent replans.
Progress and diagnostics use the effective steering target. Both supplied live
traces pass the combined real-session/walker fixture with a frozen worker leg,
17 ms native ticks and a 10-units/second synthetic plant. The final no-deploy
build and full native suite exit 0: 85 PASS groups and 3 existing missing-evidence
SKIP groups. The 192-case production route/input matrix and moving-target
progress regression pass. Receipts: `.local/r16-build-final.log` and
`.local/r16-tests-final.log`. Existing C4244 compiler and C4456/D9026 harness
warnings remain. Gameplay and audible speech remain unverified.
No commit, deployment, packaging or game access is authorized. See the
[current cause, design and prioritized procedure](Docs/features/navigation/index.md#kilika-step-15-native-staging-handoff--november16).

The following step-15 entries retain historical offline results; their live
failures and superseding implementation are recorded above.

Latest November16 step-15 live-staging follow-up reproduces skipped intermediate
waypoint observations and digital-stride reversals at the supplied reset/loop
coordinates. Native input now holds reached intermediate push waypoints, while
outside staging and final correction share a one-unit alignment and a wider
outside radial band. Native contact claims, push legs and speech are unchanged.
The 192-case route/input matrix and final full native suite pass: 84 PASS groups,
3 missing-evidence SKIP groups, exit 0. The documented installer suite passes
667 assertions and UI compilation, with PRISM-dependent cases skipped. The final
no-deploy build exits 0; existing compiler/harness warnings remain. Receipts:
`.local/r15-tests-final.log`, `.local/r15-installer-tests.log` and
`.local/r15-build-final.log`. Sol review found no actionable defect. Gameplay,
actual native movement timing and audible output remain unverified.
No commit, deployment or game access occurred or is authorized. See the
[cause, evidence and current gameplay procedure](Docs/features/navigation/index.md#kilika-step-15-staging-convergence--november16).

Latest November16 orbit/south-contact follow-up adds a 30-second whole-approach
deadline, a 15-second final-contact improvement check, live-centre pairing and
rejection of delayed staging shapes after contact entry. One-second bounded
diagnostics record timing, live geometry, alignment and radius. Orbit, shifted
reset-centre and delayed-shape regressions reproduced before repair; focused
pedestal fixtures pass. Full native suite exited 0: 84 PASS groups / 3 SKIP groups;
no-deploy build exited 0. Receipts: `.contact-tests-final.log` and
`.contact-build-final.log`. Existing compiler/harness warnings remain. Actual
gameplay/contact timing and audible speech are unverified. No commit, deployment
or game access occurred or is authorized. See the [current record and prioritized gameplay
procedure](Docs/features/navigation/index.md#kilika-approach-deadline-and-live-contact-follow-up--november16).

Earlier November16 follow-up addresses the user's step-15 stall report on `0d7166e`
in the `ffx-wt-mergecheck` worktree. Final push-side steering centres before
inward contact; progress follows waypoints; the pedestal session owns silent
stall retries and one final reason. Ordinary live-pedestal clearance remains.
No commit, deployment or game access is authorized. See the
[follow-up record and prioritized gameplay procedure](Docs/features/navigation/index.md#kilika-step-15-stall-follow-up--november16).
Requested no-deploy build exited 0; full native suite exited 0: 82 PASS / 3 SKIP
(one additional PASS group over baseline). Receipts: `.kilika-build-final.log`
and `.kilika-tests-final.log`. Existing compiler warnings remain. Gameplay
completion and audible speech remain unverified.

[Release1.0.2](Docs/release.md#release102--november4) is built, guarded-deployed,
packaged and [published Latest](https://github.com/bladestorm360/FFX-X2-Screen-Reader/releases/tag/v1.0.2).
807 installer/130 final-EXE dependency/20 scan-gate
checks, actual short/long install/reinstall, final launcher/startup,26 resources,
all7 installed hashes and unchanged INI pass. Exact EXE/ZIP/embedded DLL scans
report no detection; unsigned status and unresolved old download report remain
explicit. Source7387db8/tagv1.0.2 are pushed; draft and anonymous public downloads
match local SHA2566e43b43b…e015. User explicitly approved publication with the
documented limitations. [Single ledger](Docs/plan.md#november4-release102--scope-ledger)
is complete / Closed; receipts install/release-1.0.2-20261104, especially
publication-verification.json. Prior packages/releases/Closed work are preserved.

[November4 controller puzzle guidance](Docs/features/runtime/index.md#input-and-settings)
is complete / Closed, offline verified and guarded-deployed. View/Select, release,
then L1/LB reads selected guidance like F11 in supported field puzzles. An active
saved LB mod binding keeps priority; F8 Home, ordinary native/Grid controls,
one-shot hold/drain and stale/pressure handback are preserved. Real SDL/both
native decoders, seven SDL-owner cases, actual menu predicates and input/COM
checks pass; warning-free release build, exact-payload Defender and all7 installed
hashes pass, INI unchanged. No navigation/bake changes or new UI strings/INI IDs.
Actual hardware/audio pending; previous Closed systems/source remain intact.
[Single ledger](Docs/plan.md#november4-controller-puzzle-guidance--scope-ledger);
evidence Analysis/FFX/ControllerPuzzle_20261104/verification.json. No game/probe,
Tester Logs, saves, installer/publication, remote write or delegation.

[November3 Blitzball setup/possession repair](Docs/features/puzzles/minigame_completion_20261020.md)
and [equipment capacity menu](Docs/features/menus/equipment_overflow_20261103.md)
are complete / Closed, offline verified and guarded-deployed. Blitz visibility
recognizes all24
retained setup/match variants across loaded workers. Exact match completion uses
the live W23 clock regardless of possession/current worker; PR1's foreground fix
is adapted with attribution and existing membership/lifetime guards. Capacity
reads native window7 prompts/name substitutions, effective Keep/Discard focus,
equipment and I/Autodetail. Both173-hook profiles, profile/menu/settings/shared
utility checks and warning-free release build pass; all7 scanned/staged/installed
hashes match, INI unchanged, Defender reports no detection.144 native units/38
settings have current indexed coverage. Actual gameplay/audio remains pending.
No game/probe, Tester Logs, saves, remote write, installer/publication or delegation.
[Single ledger](Docs/plan.md#november3-blitzball-setup-and-equipment-overflow--scope-ledger);
evidence Analysis/FFX/SetupOverflow_20261103. Existing Closed work is preserved.

[November2 shop description/extra-info repair](Docs/features/menus/shop_details_20261102.md)
is Closed, offline verified and guarded-deployed. I/Autodetail read selected
modifiers first, then the visible compared character. U reads only the visible
comparison pane; independent default-Off Automatically read extra info is directly
below Autodetail in F8 → Miscellaneous. Both keys still repeat in every toggle
combination. Both173-hook profiles, menu/settings/input, warning-free release
build, exact-payload Defender scan and all7 installed hashes pass; existing INI
unchanged.38 settings preserve previous37 IDs/keys. Actual gameplay/audio pending;
no game/probe, Tester Logs, personal saves, installer/publication or delegation.
[Single ledger](Docs/plan.md#november2-shop-descriptions-and-extra-information--scope-ledger);
final523-input evidence `Analysis/FFX/ShopDetails_20261102/verification.json`.

[November1 Mushroom Rock/shared route repair](Docs/features/navigation/mushroom_rock_lag_20261101.md)
is Closed, offline verified and guarded-deployed.433 map assets/327 floors,
20928 original/current visibility/654 route comparisons and both173-hook reader
profiles pass. Sampled Mushroom routes54–390ms→5–22ms; same Sea/City long medians
79–301ms→5–14ms. Warning-free release-flavor build/exact-payload Defender scan,
all7 installed/staged/scanned hashes and unchanged INI pass. Native hooks/settings
and reachability pose-cache behavior retained; actual game/frame/audible response
pending. No game/probe, personal/tester save read, Tester Logs, installer/package,
publication or delegation. [Single ledger](Docs/plan.md#november1-mushroom-rock-lag-investigation--scope-ledger);
final evidence `Analysis/FFX/MushroomRockFix_20261101/verification.json`.

[Installer security follow-up](Docs/features/distribution/security_20261024.md)
is implemented and offline verified: system-folder native DLL lookup, real
publisher metadata and mandatory exact-artifact Defender packaging gate.
Original/rebuilt EXE/ZIP/all6 embedded DLLs report no detection;807 source,
130 dependency,20 gate checks and actual89/307-character install/reinstall pass.
All26 resources/424 native inputs match deployed1.0.1; no game deployment/launch.
The reported Edge download detection remains unresolved; no threat/file name
was supplied, Edge is unavailable and trusted signing is not configured.
The1.0.1 replacement candidate stayed local; the user later explicitly approved
the hardening's distribution in1.0.2 with the report unresolved. No external
vendor-submission authorization is inferred. Original1.0.1 preserved;
candidate ZIP SHA25689efe9c345b5845803fd517c8e0015b1bbee064ced2ef26d3e62830d088336f0.
Evidence/review request: `install/defender-20261024`. No protections changed,
Tester Logs, sample uploads or publication occurred in that October24 task;
no delegation. The October23 receipt below is historical; its asset is unchanged.

[Release1.0.1](Docs/release.md#release101--october23) is built, guarded-deployed
and [published](https://github.com/bladestorm360/FFX-X2-Screen-Reader/releases/tag/v1.0.1).
Source5dc8cf6/tagv1.0.1; final installer/ZIP, draft and anonymous public downloads
match SHA256d38fdcd355f52f0de1891f765c66f24a51353aedc3b5e69d023ba54201218f52.
It includes destruction-sphere flag retention and all-eight-language launcher
navigation/settings restoration. Besaid collection is gameplay-confirmed; remote
Baaj Ruins unlock without warps and actual launcher/audio remain pending.
Zanarkand was already present in the supplied save despite a missing-sphere
report. Release excludes the console and supersedes the earlier delivery holds
below. All7 hashes/26 resources/5 ZIP files, source805/dependency130 installer
assertions and actual short/long installs pass; INI/published1.0 ZIP unchanged.
[Receipt](install/release-1.0.1-20261023/verification.json).
Luna player docs reviewed/corrected by Sol; all technical work retained by Sol.

[Launcher language/settings repair](Docs/features/distribution/launcher_languages_20261023.md)
is complete/offline verified and staged. The old English-only attachment gate
reproduces Japanese failure; all eight startup locales/seven actions and64
settings transitions pass the real-adapter/PRISM surrogate after repair.
Warning-free dev build retains console,419 prior inputs unchanged plus new label
header. Installed seven-file console payload/config/settings/1.0 ZIP unchanged;
deployment held while user tests. Real launcher confirmation pending. Approved
read-only MetaMenu.vbf evidence remains outside Git; no game launch/probe or
Tester Logs. [Receipt](install/launcher-languages-20261023/verification.json).
Exit direction behavior is left unchanged at the user's October23 request.

[Current developer console](Docs/features/runtime/console_f1_20261020.md#october23-current-development-console-delivery)
is enabled and guarded-deployed at the user's request. Default development build
is warning-free; isolated console host, both preflights and all7 installed hashes
pass. All420 prior verified source/test inputs unchanged; October22 flag repair
retained. [Receipt](install/dev-console-20261023/verification.json).
Shift+F1, `warp besaid` or `warp zanarkand`, Enter: ordinary entrance0 transitions,
no forced story/sphere setup; native destination entry scripts run normally.
Settings/1.0 ZIP unchanged; no agent game launch/probe/warp or installer rebuild.

[October22 sphere-flag follow-up](Docs/features/puzzles/destruction_flags_20261022.md)
checks Besaid first, Zanarkand second. Named Summon save still0x3E: only Besaid
missing. Auto now verifies an exact owned final chest RET and retains its Baaj
bit before completion, preserving other flags. Original1536 bytecode cases,
both native384-case matrices, both173-hook readers, warning-free build and
guarded deployment pass. All7 installed hashes verified; INI/save/1.0 ZIP
unchanged. [Receipt](install/destruction-flags-20261022/verification.json).
Historic missing-flag cause is not proven by the save. No save edits, game
launch/probe, Tester Logs or installer/publication work. Gameplay pending.

[Release1.0/current procedure](Docs/release.md): user superseded deployment and
installer holds, requests final1.0/public repo/GitHub ZIP publication following
FFPR Core. Warning-free1.0 native build guarded-deployed; all7 hashes, both image
profiles,1.0.0.0 metadata, installer source/actual dependency and short/long
install/reinstall,26 resources and exact five-file ZIP pass. Current evidence:
`install/release-1.0-20261021`. ZIP SHA256
`768c49042b3c73966a97656ed0f22973402860c908614b0b90d17ddd49a2963d`.
Settings/prior releases preserved; gameplay pending; no game launch/probe.
Publication is complete: [public Latest1.0](https://github.com/bladestorm360/FFX-X2-Screen-Reader/releases/tag/v1.0),
source625af61/tagv1.0; verified draft and anonymous downloads match the ZIP hash.
After CLI DELETE403 the user manually deleted old private ID1191559099.
Authenticated404 preceded new private ID1428963798, clean master/three tags,
all32 known removed objects404, then public visibility/publication. Physical
GitHub backup erasure is not claimed. Recovery remains outside Git in
Analysis/FFX/RepositoryCleanup_20261021. The user chose unchanged replacement
permissions; no collaborator invitations. [Evidence](Docs/features/remote_cleanup_handoff_20261020.md).
GitHub README formatting was corrected in71ac394 with short paragraphs/semantic
lists; actual published HTML verified. Packaged README/installer/ZIP/tag unchanged.
Future release
requests include commit/push/tag/draft download verification/publication; Dropbox
is historical. Luna supplied bounded1.0 player docs; Sol reviewed, corrected
wording/format and owns implementation/deployment/verification. No Astra.

Earlier implementation checkpoints below retain their initial holds; the1.0
request and deployed/package state above supersede those holds.

[HP/MP follow-up](Docs/features/battle/hp_sources_20261021.md): H and targets now
read committed native totals, replacing stale/uncapped action scratch values.
Percentages and maximum preferences preserved. Switch's effective maximum is
verified against original getters, renderer and native capacity doubling
(4618 base→9236 effective; genuine6116 current). Both173-hook profiles, real
owned status worker, warning-free release build and both image preflights pass.
Current source/payload receipt: `install/battle-hp-20261021/verification.json`.
The user's deployment hold persists; installed modules/settings/releases remain
unchanged. Installer held; fresh gameplay pending. HP work retained by Sol.

[Current speech repairs](Docs/features/battle/seymour_amount_prompts_20261021.md):
Mortiphasm elements/turning mechanics, Spare Change/Yojimbo amounts/decimal step,
native parameter prompt/Yes/No, Formation active/reserve and both learned-mode
name substitutions are implemented and offline verified in both173-hook profiles.
Warning-free release build and installed-image preflight pass. The user explicitly
holds deployment while playing; staged payload is ready, no modules replaced.
Installer remains held. [Prior repair receipt](install/battle-prompts-20261021/verification.json).
Own current log beside FFX.exe works; sandbox showed a stale empty view.
Use real host view for log/process checks. Seymour asset read was approved.
Luna gpt-6-luna supplied bounded popup/Formation edits; Sol corrected ABI/fixture
errors, integrated/reviewed and ran original-data/native/worker checks. No Astra.

Active source: `src/native`, x86 dinput8 proxy and PRISM. Start with the
**[complete feature index](Docs/features/index.md)** and
[design/status register](Docs/features/status.md). They link source/data flow,
thread/lifetime ownership, controls/settings, native evidence, safe extension and
verification limits. [Source map](Docs/features/source-map.md) covers144 native
units; [settings](Docs/features/runtime/settings.md) covers38 persisted options.

Packaged release: **0.5.9**, requested after the minigame investigation to unblock
testers. Warning-free release build, guarded deployment, single installer/ZIP,
805 source/130 actual installer assertions and short/long install/reinstall pass.
[Refreshed release receipt](install/nucleus-prearm-20261020/release-verification.json).
Dropbox replaced; its link was sent to Dylan through native desktop Discord,
verified in message history. Attachment picker closed; no browser login.
Previous releases/settings/saves preserved; console excluded; no publication.
Current local **0.6** adds all four documented completion assists: all Calm Lands
training/perfect Catcher, Remiem first-win/next unearned chest tier, both butterfly
courses/current phase, and Blitzball win after a confirmed live Action Commands
choice. Independent saved default-Off F8 → Miscellaneous options appear before
starting. [Implementation, native mechanisms and limits](Docs/features/puzzles/minigame_completion_20261020.md).
All9 original variants/64 Remiem flags/score-choice-story bounds, settings/menu,
both171-hook readers and native regressions pass. Warning-free release-flavor
build (console excluded) guarded-deployed; all7 hashes and329 source inputs
verified, INI/package hashes unchanged. [Delivery](install/minigame-verification-20261020/verification.json).
Gameplay pending; **HOLD installer rebuild/package**. Prior0.5.9 package preserved.
Prioritized [accessible checks](Docs/features/verification.md#optional-minigame-completion-06).
Luna supplied bounded settings/menu/tests/player docs; Sol reviewed, corrected
fixture indexing and wording, validated/integrated/deployed; no Astra.
Sea of Sorrow is now user gameplay-confirmed; later Sin testing is left to a tester.

Earlier Cid repair and Baaj diagnostics were guarded-deployed from warning-free
developer source: [receipt](install/dev-console-20261020/cid-baaj-verification.json).
Both171-hook reader profiles pass all64 treasure-flag combinations in both
callback proofs and540 original native endpoint comparisons each. Six labels
match original temple treasure writes. Macalania's qualified cleanup-only
unavailable branch is now recognized. [Baaj mechanics, evidence and current-save correction](Docs/features/navigation/baaj_seals_20261020.md).
The installed Baaj slot0 was read directly and exactly matches the named
Summon file. Earlier older/different-save wording was wrong. Latest user gameplay
confirms Besaid starts/remains inactive because its treasure is missing, and all
eligible seals work. User accepts the result: Baaj report CLOSED. Final door
opening with all six treasures was not tested. No further capture requested;
bounded native flag/counter diagnostics remain deployed for a future recurrence.

Development-flavor builds provide Shift+F1, `warp 196 0`, Enter. The current
installed0.6 uses the release flavor and excludes the development console.
Exclusive key ownership prevents native turbo leakage, including queue pressure,
focus/console transitions and held repeats. Plain F1 passes through.
[Console/input](Docs/features/runtime/console_f1_20261020.md).
[Free Search audio lifetime repair](Docs/features/puzzles/airship_audio_20261020.md)
and [Shift+M/F-key/Password controls](Docs/features/puzzles/airship_keys_20261020.md)
remain deployed. Exact audible producer and fresh gameplay remain unconfirmed.

[Repository/source boundary](Docs/features/repository_content_20261020.md): original
archives,32 replay bodies and pre-cleanup history are outside every Git tree.
Guarded tools/tests use `FFX_EVIDENCE_ROOT`; mod adapters retain no original
bodies. Sanitized history and current mod source are public in the replacement;
all32 known removed objects404. [Documentation audit and delegation review](Docs/features/documentation_audit_20261020.md).

INI, named saves and0.5.7/0.5.8 ZIPs preserved. The preserved0.5.9 package remains verified; local0.6 gameplay testing is pending.
No game launch/control/probe, Tester Logs or registry repair.
Heartbeat PAUSED;2% remaining stop retained. Actual reported Proceed? prompt and
Free Search Y investigation remain deferred. Full mod UI localization/HRTF and
other unimplemented scope stay explicit in the register. User-side prioritized
checks: [verification](Docs/features/verification.md).

[Cid/Wakka repair](Docs/features/navigation/airship_cid_20261020.md) is implemented,
offline verified and guarded-deployed: actual actor identity, story-independent
presence, current pose and selection transfer. Both171-hook profiles cover four
original variants and actual command selection across movement/worker transfer.
Gameplay confirmation remains pending; no running-session replacement occurred.

[Sea of Sorrow repair](Docs/features/navigation/sin_sorrow_routes_20261020.md): implemented, offline verified
and guarded-deployed. Original floor/callback replay now detours around reverse
ledge crossings; spoken paths preserve bends. Both171-hook profiles, six critical
routes/all16 event states, reachability, beacons, Auto walk and native actions
pass. Receipt: `install/sin-route-20261020/verification.json`. User confirms the
Sea of Sorrow repair works in gameplay on October20.
[Nucleus optional collection](Docs/features/puzzles/sin_collection_20261020.md) is implemented, offline verified and guarded-deployed: F8 →
Miscellaneous → Sin object collection → Collect objects automatically. Default
Off, saved across entry/battles/restarts; menu visible only on all Sin maps,
actual assistance only in the exact Nucleus resource/both room aliases. Original
native control/reward/count10 progression is preserved. The missing-menu/lifetime
follow-up passes fresh component/menu/settings/both171-hook reader profiles,
warning-free build, guarded deployment and installer/ZIP verification.
[Delivery](install/nucleus-prearm-20261020/release-verification.json).
[Later Sin gates](Docs/features/navigation/sin_late_obstacles_20261020.md): stale retained routes and queued movement repaired/deployed; native
remaps immediately pause input while a fresh route replans. Both171-hook reader
profiles, original/native component hosts and warning-free build pass. Gameplay
pending. User-approved exact City/Nucleus floors pass56210 original-native
predicate comparisons,192 City routes/all64 gate states and all40401 Nucleus
candidate positions. Four missing City platform boarding stops are now listed
when natively enabled; both reader profiles and warning-free build pass.
[Latest guarded delivery](install/sin-maps-20261020/verification.json): all7
hashes match, settings/named saves/ZIPs unchanged. Full ride/gameplay pending.
Originals remain outside Git. These changes are included in verified0.5.9;
no game launch/control/probe.

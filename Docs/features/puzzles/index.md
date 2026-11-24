# Trials, progression puzzles, minigames and airship

The [Sin Nucleus collection helper](sin_collection_20261020.md) is implemented
in source with original-native component/menu checks and guarded deployment.
Gameplay remains unconfirmed. Its default-Off saved toggle is visible on Sin
maps only; actual assistance is qualified to The Nucleus and resumes after entry
or battle. The linked topic records the0.5.9 availability/lifetime follow-up.

## Shared script ownership

`utility_memory.cpp`, `utility_context.cpp`, `utility_scripts.cpp` and prepared
ATEL resources qualify exact chunk/allocation/worker/header/declaration/entry
identities. Native interaction/request/RET events drive features; no background
memory watcher searches for a convenient state. `NativeAction` prepares and
rechecks context, invokes on the verified native owner without a reader lock,
then publishes completion. Workers receive owned guide/text/snapshot data.
Failing a hash/owner/context check refuses intervention and leaves native behavior.

## Cloisters

In the first-visit Zanarkand floor trial, W38 and W57–W83 receive periodic
interpreter calls while the panel layout is stable. Other workers can also
change the route. The pedestal entry 2 of W44–W47 writes the stage binding
0x5c000004. For ffx-s4, native panel input pauses during any interpreter
call. On return, the mod checks the captured script
allocation, player radius, ten pattern values and all 27 panel workers. Changed
or unreadable routing facts invalidate the observation. A new complete scene
can authorize movement again. Snapshots taken inside a call cannot do so.
The existing 250 ms age limit still applies.

November 17 r27 guide and collection behavior

- Bevelle entrance assistance uses the armed crossing plan through W39:6,
  including when the player starts at staging. W38 boarding now publishes a
  stop-specific target from the verified script, active channel 3 callback,
  pedestal readiness flags and native activation band. The route and refreshed
  field list share the same identity. W38:8 needs X from -68 inclusive to -62
  exclusive, with logical Down or the matching stick angle. W38:9 needs X from
  24 inclusive to 27.5 exclusive, with Right or the matching angle.
- W43 timing speech follows the objective. Wait for left to reach the source
  and right to reach the power recess. Actionable Expected and Path speech
  requires a snapshot at most 100 ms old. A visible rail prompt must match its
  native producer and be the sole visible message.
- Direct Bevelle collection requires field control. The owned final RET records
  the epoch, script chunk and allocation serial. Only a matching collection
  and treasure flag allow W54:6 at PC 0x1cde2 to bypass W16:8's reward body.
  The child returns at its original RET, leaving W54 to handle
  its exit flag, camera, rail and field control. A second inventory credit in
  the original behavior has not been established.
- Besaid Path refreshes the tracked recess step after message close. Macalania
  uses copied motion, a native destination phase and control handback.
- In Zanarkand, Path routes around other enabled player-entry rectangles. The
  guide exempts its selected panel by local and global worker identity. Clearance
  uses the controlled actor's collision radius. A missing worker, stale sample or
  blocked route leaves the walk stopped. The route owner checks new observations
  during a walk. The input owner suppresses old movement while qualification is pending.

The final r27 build and native suite both exited 0. The native log contains
155 PASS records and four missing-evidence skips. Strict replay passed 724
checks over 49 events across six sequences, with zero XFAIL. Python discovery
passed 15 tests and skipped the archive class that requires the absent
six-trial manifest. Physical input, inventory credit and audible speech timing
remain gameplay checks.

November17 sequence regression host: `src/native/tests/puzzle_sequence_fixture.cpp`
feeds ordered owned observations through the production guide, scene
zone decoder, target configuration, pedestal rules and geometry helpers. Run
`src\native\tests\run_tests.bat --puzzle-sequences`; the full suite includes it.
Expected actions cite ATEL worker/PC evidence or the November17 live reports;
the host does not import the production route or permitted-panel tables as its
oracle. Six streams cover entrance collection, collection after riding, the
live W70/W69 reset followed by seven legal pattern-2 panels, Besaid's single
message close, equal-count coalesced reset with late activation, and missed
Bevelle departure observations with a new room epoch.

Each event checks spoken step/target, step monotonicity except explicit resets,
level access, completed reward/panel target retirement, duplicate completion
cues and message action ownership. The r26 replay passed six streams, 45 events and 620 checks.
The previous five failed assertions reproduce on the earlier build. The full native
suite and no-deploy build both exited 0. The suite recorded 123 PASS lines plus
proxy forwarding and four evidence skips.

In Bevelle, W16:8 grants the HP Sphere. W15 independently writes rail node 64
and pedestal phase 14 at PCs 0x134d6 and 0x134dc after the ascending ride.
The guide waits for that landing or a subsequent upper-platform pedestal state
before the treasure checkpoint advances it to the final recess. The entrance
still targets W13. At a qualified junction Path names the configured Confirm
input. At a different parked stop it routes to W38 only if its current boarding
rectangle matches the native stop and the player's level. During an unobserved
handoff the guide gives the rail instruction and withholds the walking route.
In Besaid, a visible result message owns both Expected and Path, including
the interval after the control and window flags clear. Both give one close hint.

### Bevelle rail junction map and auto turn (November 18)

Static read of `bvyt1100` plus live runs. `gNN` is binding `0x0c0000NN`.
Every junction W41-W51 accepts on the first in-region frame where Confirm is
held and its guard holds. Arrow cyclers alternate every 400 ms. They freeze at
an accepted press and resume 5672 ms later with the opposite value.

- W40 boarding prompt: Confirm edge starts the car.
- W41 (upper rail, northbound, guard `g0b==1`): turn east and climb to the hub.
- W42 (same point, southbound): always accepts; climbs to the hub when
  `g0b==1`, otherwise U-turns north.
- W43 (no guard, `g0c`): phase 1 west to pedestal 1 (Upper Bevelle Sphere
  source); phase 2 east to pedestal 3 (upper power recess).
- W44 (guard `g0d==3`): U-turn south.
- W45-W47 hub (`g0e`, car parks): 0 dead end to pedestal 4 (Lower Bevelle
  Sphere source); 2 lower rail north (needs `g25==1`); 3 back to the upper rail.
- W48 (guard `g0f==1`): east to pedestal 6. W49 (guard `g10==1`): east to
  pedestal 10. W50 (guard `g11==1`): east to pedestal 12. No turn: the car runs
  to the rail end and parks at the hub again.
- W51 is never enabled.

With Auto walk on, the mod presses Confirm for the junction the current step
needs (`src/native/bevelle_rail.cpp`). At an unguarded junction it presses only
while the phase matches. At a guarded junction it holds Confirm for the whole
region. At the hub it waits until a departure press makes the car reach W48
(7807 ms), W49 (9309 ms) or W50 (10811 ms) during the wanted phase. Those
departure-to-zone times come from live logs.

Step 13 does not push the pedestal. At stop 14 the W38 E14 handler is empty.
The hidden W53 call point, where the Destruction Sphere treasure was, calls the
empty pedestal to stop 15. W54 enables it when the chest is opened. Holding
Left at stop 15 with player Z of at least 267 boards it (W38 E15), and the ride
reaches the optional chest. The mod lists W53 as "Rail pedestal call point".
Live on November 18, all 15 steps passed in one replay, including the Knight
Lance chest and the antechamber exit.

This is an observation replay, not a game emulator or original-floor replay.
Level checks use evidenced transport prerequisites and coarse connected areas;
the route-service boundary records the real target and declines traversal.
Native reads, movement and audio fail if reached. One Confirm is represented by
the resulting closed-message observation; native key consumption is not tested.
Reward checks cover guide claims/targets, not native inventory grants. Existing
utility-control fixtures separately cover final-RET ownership. Collision paths,
safe panel avoidance, rail/camera recovery, actual speech and timing remain
unverified. The current log retained the W70/W69 reset; the earlier Bevelle run
was available through `bevelle-r25-live.md`, not the overwritten log segment.
The original failure receipt is `.local/sequence-replay-verification.json`.
The repair receipts are `.local/step3-strict.log`, `.local/step3-final-suite.log`
and `.local/step3-build.log`.

Puzzle data headers are hand-maintained because their generator inputs are no longer retained; all three generators refuse differing in-place output unless `--overwrite-maintained` is explicit, and support `--output-dir` for separate output. Regenerate-and-diff testing is skipped while those inputs are missing.

November17 r25 Zanarkand floor-guide repair (offline; no deployment): the tile
catalog generator treated the first duplicate-group occupancy flag as a full
quota. Pattern 2 therefore offered at most six distinct successes although its
native counter requires seven. Pattern 4 had the same two-slot defect in both
groups. The corrected blocked lists require every native quota slot to be full;
Zanarkand now captures 18/32 guide bindings, including 0x26 and 0x28.

Retained dome0700 EBP SHA256
`a530da97346bc9d9bee90ab5239663d283d128d13203fe3e55bc4e308c1d29c6` proves:

- Pattern 2 requires panels 11, 21, 23 and 28; any two distinct panels from
  15/19/24; and any one from 13/17/18. All seven may be touched in any order.
  The ten catalog rows are alternatives, not ten required successes.
- Bindings 0x0c000025/26 are the first/second occupied slots for workers
  70/74/79; 0x0c000027/28 serve workers 68/72/73. Pattern 1 accepts one from
  each group; pattern 2 accepts two/one; pattern 3 all nine panels; pattern 4
  accepts two/two. W38:24 reaches success at count seven (0x62ea..0x635c).
- A wrong panel or an over-quota alternative requests the native reset.
  W38:9 clears count 0x24 and all four quota flags at 0x5af7..0x5b0f.
  Native collision reactivation makes those panels eligible again.

The guide now uses current valid interaction facts instead of remembered tile
completion receipts. A failed/reset entry cannot poison its own eligibility,
and a reset followed by progress between captures cannot leave an old visited
set behind. Missing or all-inactive rows report unavailable floor observations.
Native scene ownership suppresses transient floor advice. Completed steps are
checked before selecting another panel; first-room completion is required
before pedestal advice. Existing Bevelle/Macalania work is preserved.

The r25 F2 mitigation was an explicit warning on floor and pedestal guide responses:
auto walk does not avoid other active panels and can reset the pattern.
At r25, routing was unchanged. The shared route builder and walker replans treat floor
triggers as interactions, not forbidden collision areas; an anonymous guide
point also lacks target-worker identity. A correct avoidance change needs
qualified trigger geometry and target exemption across both paths. Adding
unverified circles or altering only the first guide route would not prove safety.
The reported no-progress stops remain unverified.

Owned-worker diagnostics use `Zanarkand floor event=tile-completion` or
`event=state-change`, with worker (panel = worker minus 55), current/previous
stage/count, `reset-observed`, completion milestone, four quota flags, copied
channels and data age. These are observations after queued completion requests,
not a lossless native-event trace. Equal-count resets can escape the diagnostic;
current interaction facts still recover eligibility. No hook waits, game writes,
new worker-side native reads or native-thread logging were added.

Verification receipts: `.local/zanarkand-r25-{red,green,script-audit,full-tests,build}.log`
and `.local/zanarkand-r25-verification.json`. Eleven regression groups fail against
the preserved pre-fix guide and pass afterward; nine legal pattern-2 combinations
exercise 63 selections. The offline integer-branch audit checks 1,197 cases
against retained EBP data, treating native calls as opaque barriers and rejecting
native-result dependencies. It does not emulate graphics, collision or timing.
Final full suite exits 0: 121 PASS records plus proxy forwarding, four unchanged
evidence skips and no failures. No-deploy build exits 0. Source hashes match
through both gates; warning categories are unchanged or reduced from r25.

Prioritized nonvisual live check, only after separately authorized deployment:

1. First visit, initial floor pattern: touch panels 2/4/6/9 and either 3 or 8.
   Before completion, requesting pedestal 1 must give remaining floor advice.
   After completion, reviewing step 2 must never request the unused alternative.
2. After completing pattern 1, activate pedestal 2. Move manually around other
   floor panels. Follow seven permitted recommendations; expect four fixed
   panels, two from 15/19/24 and one from 13/17/18, then pedestal 3. Failure:
   only six accepted panels, a lit panel repeated without reset, or the old
   stopped-pedestal response. Warning speech must precede relying on auto walk.
3. During pattern 2, deliberately touch a wrong or over-quota panel once.
   After native control returns, ask again. Previously lit panels must be
   eligible again; diagnostics should show the count drop when observed.
   Repeat rapid queries and a guide reopen mid-pattern; no old visited history
   should survive current interaction facts. Do not expect a safe auto-walk route.
4. Progress normally through pattern 3 to pedestal 4. Expect two panels from
   each duplicate group, plus 11/21/27. Check first/repeated queries and busy
   scene handback for premature advice, missing speech or repeated unavailable
   messages. Audible response timing and complete live navigation remain untested.


October22 [destruction-flag follow-up](destruction_flags_20261022.md) verifies
the supplied save is missing Besaid alone, then checks Zanarkand/all six. Auto
now accepts only the original chest's owned final RET and preserves/verifies its
actual Baaj prerequisite before continuing. Gameplay confirmation is pending.

`puzzle_solver.cpp`, `puzzle_completion.cpp`, `utility_trials.cpp` and generated
trial data support Besaid, Kilika, Djose, Macalania, Bevelle and Zanarkand.
The automatic option is separate/default Off; manual `puzzle_guide.cpp`,
`puzzle_guide_owned.cpp`, `puzzle_guide_targets.cpp` and `puzzle_tiles.cpp` provide
step selection, status and path guidance. F9/F10 move guide steps; F11 reads the
current step and Shift+F11 uses the inferred-step route. F8 exposes context
actions and status. Guides are mod-owned assistance, not an extra native menu.
Controller Mod mode then L1/LB now requests the same F11 selected guidance when
that mod control is unassigned. It shares actual-guide/context checks, preserves
saved assignments and does not enable automatic puzzle solving.

November17 r25 utility/entrance repair: direct `trial collect` ran Bevelle W16:8
at `1370f` through final RET `1379a`, omitting its native caller W54:6. That
caller waits for the child at `1cde2` and restores control with Common005D at
`1ce0a`. The owned utility now supplies that native restore only at the verified
final chest return, before native next-signal dispatch. Exact signal, entry,
worker/chunk, controller, epoch and allocation serial must still match. It does
not replay W54's movement, exit flag or camera effects. Other temples already
restore control in their chest continuations: Besaid `2a4c`, Kilika `30c4`,
Djose `bc08`, Macalania `d27c`, Zanarkand `48e5`; they receive no extra restore.

Bevelle entrance instructions now describe selecting the Rail pedestal and
pressing backslash with Auto walk enabled. Manual direction-plus-push wording
remains for the nonautomatic pushes, including Macalania. Besaid/Kilika retain
walk-consent instructions. At the exact armed entrance W39:6 launch, the mod
retires the walking route before handing off to the native ride. This prevents
the stale floor route from later saying "Route lost: no connected route" while
the rider moves. Ordinary route failures remain audible. Junction cue design,
timing and native rail input are unchanged.

The r25 fail-before receipts are `.local/bevelle-r25-{a,b,c}-red.log`;
the final native suite/build receipts are `.local/bevelle-r25-{full-tests,build}.log`.
`.local/bevelle-r25-script-audit.log` records the six retained EBP identities and
cleanup PCs. The control fixture exercises production continuation hooks with
synthetic native memory/API boundaries, including stale/foreign completion and
a next native scene that disables control. No gameplay or audible result is
claimed; later testing must cover collection both at the entrance and on rails.
Final r25 full suite exits 0: 110 PASS records plus proxy forwarding, four
unchanged evidence skips and zero failures. The new control fixture passes 34
checks. No-deploy build exits 0; warning categories match r24. An initial run
is superseded by the final rerun after repairing the new fixture's null-read
stub and its runner's handling of negative Windows exception exit codes.
Production inputs remained unchanged after the successful build. No commit,
deployment, game interaction or Tester Logs access occurred.

November17 r24 follow-up supersedes the rail naming/timing limits below. Rail
Confirm now uses the copied inverse field-remap mask, shared with single-push
guide naming. Default mask `0x20` selects glyph `0x31` (keyboard C); the old
`0x32` names the distinct push binding (keyboard X). Step/path and proactive
rail speech share the correction. Push/Dive uses of `0x32` were checked and
remain intentional. Keyboard, pad, menu-swap, unavailable-name and remapped
Confirm fixture cases cover the distinction.

The existing coalesced `GuideCompleted` refresh now returns a short rail cue
when the active Bevelle guide observes W40-W51 opening or a changed arrow phase.
Repeated observations stay silent; closure/acceptance rearms the next window,
and guide/context reset clears the remembered state. It uses the existing
region and turn-ready predicates, with no input injection or new polling loop.
Snapshots older than 100 ms cannot produce a timing cue. The extra W22 phase
binding `0x0c00000e` brings Bevelle's combined bounded capture to 30 rows of 32.
The old variable-write hook spoke after acceptance (`0x2c=1`), so it is removed.
All formatting and cue decisions stay on the preparation worker.

Original-script timing: W15 initializes `0x0c00002f=12` at `110f9`.
W19-W25 wait that many script ticks per displayed arrow phase: W19 0/1,
W20 1/2, W21 0/3, W22 0/2/3 (route-dependent subsets), W23 0/1,
W24 1/0 and W25 0/1. Twelve ticks is nominally 0.4 seconds at 30 ticks/second;
the common two-phase cycle is 24 ticks, nominally 0.8 seconds. Acceptance
freezes cycling. W40's entrance wait has no expiry; it waits for its pressed
Confirm latch. W41-W51 are spatial type-3 input regions, not fixed-duration
timers: their usable interval ends on leaving the region or accepting a turn.
No exact wall-clock crossing duration can be derived from those waits alone.
W51 initializes disabled, so its fixture is not evidence of a live stop.

W43 tests held Confirm at `1c885`, then sets `0x2c=1` and its turn selection;
there is no left-phase rejection. W20 freezes on that latch. Thus the supplied
200-400 ms delayed press can miss a 12-tick phase or leave the spatial window;
the report does not establish which happened. No speculative readiness guard
or native timing change was added. The query now explains holding Confirm and
snapshot timing. First audible output and physical input acceptance still need
gameplay measurement; the 100 ms freshness guard is not an end-to-end guarantee.

Macalania and other nonautomatic pedestal hints say: with Auto walk on, walk
to the Pedestal with backslash and hold only the configured push key, because
the mod adds the direction and a direction key cancels that assistance (in
game, 2026-11-19). Without Auto walk, hold the key and direction together.
Single-press controls retain their one-press instruction. The shared formatter
also fixes ordinary navigation/pedestal advice. Manual direction cancels the
assistance session and its speech; the active Macalania guide now observes W1's
copied motion and original phase transitions independently. It says one
"Pushing pedestal" and one "Pedestal push complete" after destination phase
and native control return. Phase 99 is animation, reset phases are not completion,
and armed assistance retains its own speech. Missing an entire motion between
snapshots cannot establish that progress. No native hook or input behavior changes.

Earlier r24 verification receipts: `.local/bevelle-r24-final-{full-tests,build}.log` and
matching `.exit` files. Final full suite: exit 0, 109 PASS records plus proxy
forwarding, four unchanged missing-evidence skips and zero failures. Final
no-deploy build: exit 0; warning categories match the previous run. Final source
hashes remain unchanged through both checks. The original entrance, treasure and rail-stop changes
remain. The fixture covers proactive windows/phases, deduplication, acceptance,
freshness/reset, plus manual Macalania motion/completion and assisted handback.
No deployment, commit or interaction with the running game in this task.

November17 Bevelle entrance-guide repair: the owned completion check treats
`bvyt1100` binding `0x0c000008` as departed whenever it is nonzero. Original
W0 pc `f4` initializes it to 0; W13 writes 1 at `4b00`, then 19 at `4c7d`.
The prompt/ride can hide node 1 from `follow()` while it waits for control.
Fresh rooms still expect step 1. The production-guide fixture reproduces
0-to-19 without observing 1, and covers settling, late guide reads and resets.
Bevelle step 13 needs no change: node 17 follows the durable `0x0c00002a=1`
chest checkpoint, which skips the missed node 16. No hook/capture changes.
The first callback-only rail fix passed synthetic tests but failed the supplied
live run: visible entrance text still produced a close-message hint, and later
junctions still said to wait. W40 is only the entrance confirmation region.
The November17 follow-up qualifies the nearby enabled type-3 channel-3/entry-5
regions W40 through W51 from the owned scene. W41?W50 are the later junctions;
W51 has the same callback but is initialized disabled in this script. Distant
regions and accepted inputs remain settling: entrance latch `0x0c000014` and
junction latch `0x0c00002c` must be clear. W39 keeps walking control disabled.
Only W40 and W45?W47 can override visible text, since those callbacks own and
close rail-confirm messages. Windows and an assisted held push still block.

W40 tests action 5 pressed at `1c743`; W41?W51 test action 5 held. Their input
callbacks do not test direction keys. The guide names the configured Confirm
binding and explains timing instead of asking for directional steering.
W41/44/48/49/50 additionally require arrow phases at bindings `0x0b/0x0d/0x0f/
0x10/0x11`; the guide distinguishes an unavailable phase from a scene wait.
At W43, W20 cycles `0x0c` between 1 and 2 (`15528`, `1555b`). W15 reads it at
`11529`: phase 1 leads left to the source, phase 2 right to the opposite power
recess. These are copied observations, not promises that the arrow will stay
unchanged until speech ends. No direction-key-to-branch mapping is invented.
Eight scalar bindings join the existing bounded capture (29 total Bevelle rows
including navigation facts, below capacity 32); no new hook or polling loop.

Explicit Bevelle guide queries while ordinary settling flags are present emit
one `Bevelle rail query` line, limited to once per second. It includes visible
message, window, control/raw message, node `0x0c000008`, binding readiness, both
accepted-input latches, matched input worker, arrow availability, data age and
W40/W39 validity/type/channels/all seven entries. Background completion emits
nothing. Formatting and logging stay on the preparation worker.

The r24 guide accepted a newly observed treasure bit at the rail prompt because
`trial collect` can bypass the normal route's `0x26` flag. It also accepted
`0x26` plus the treasure bit on a late read. The November17 access repair above
supersedes both checkpoint shortcuts. The reward persists across visits, and
the current rail and pedestal state establishes access.
Both reported cases fail against the preserved previous fix and pass with this
follow-up. The focused fixture also covers all 12 callback identities, arrow
phases and both upper-second-junction branches, accepted-input/distant-region
and dialogue guards, query-only logging and its rate limit. Its existing
geometry dependencies are controlled test inputs; native capture and live
region/arrow timing remain unverified. Final full suite exits 0 with 107 PASS records and four retained-evidence
skips; the no-deploy build exits 0. Warning categories match the prior run.
Verification receipts and exact remaining evidence skips are in the report and
`.local/bevelle-rail-{red,green,full-tests,build}.log`.

Prioritized later nonvisual check, after separately authorized deployment:
1. Fresh room: Shift+F11 says Step 1 and explains the automatic entrance push.
   With Auto walk enabled, request guide Path at staging. Expect instructions and
   stationary feet. Select the Rail pedestal and press Backslash. Listen for
   “Rail pedestal: crossing now.” Press Backslash again during the crossing.
   Expect “Rail pedestal: crossing in progress.” The ride should start without
   holding X or a direction. A stale-route warning during the ride is a failure.
   While "Confirm direction" is visible,
   query again. Expect Step 2, the upper sphere source and the configured rail
   Confirm input; neither "close the message" nor a generic scene wait.
2. With the guide active, ride without querying. Expect one short entrance or
   junction cue per opening/phase, naming C on the default keyboard. At W43,
   expect a turn cue for the required branch and a release cue for the other. Hold C promptly at
   the wanted phase; release after the turn. Direction keys do not select it.
   A missed turn should offer a cue on the next circuit. Failures: no cue,
   repeated unchanged-phase cue, or a cue only after acceptance. Compare
   normal/busy and first/subsequent circuits; check whether the short phase
   leaves usable time after actual audible output. The script/fixture cannot
   prove that latency or the cause of the previously missed W43 turn.
3. Macalania step 1, first mound restored: read the guide, approach its push
   side with Auto walk, then hold only the announced push key. Expect one
   start cue and one completion cue after native control returns. Repeat on the
   ramp stage only after restoring its sphere. In game on 2026-11-19, steps 1,
   2 and 7 pushed with the key alone; a held direction key cancelled the
   assistance. The manual key-plus-direction push is not yet tested. Check manual handback after an armed approach for lost or
   doubled speech; no fixture substitutes for this audible check.
4. Test collection in separate fresh runs at the entrance and at the upper
   power-recess rail stop. Activate the guide before `trial treasure` and
   `trial collect`; close the HP Sphere dialogue once. Expect arrow movement
   to work after the chest sequence finishes. At the entrance expect the
   pedestal or the entrance Confirm prompt. On the upper rail follow the
   announced branch or boarding position. Fail if the guide routes to the
   final recess before the ascending ride or repeats the collected chest.
   During a ride with field control disabled, expect collection to wait. After
   direct collection, cross the physical chest trigger and check inventory,
   control and progression. After the ascent, expect Step 12. The optional ride requires the
   upper-platform recess filled; its chest checkpoint advances to Step 15 even
   when node 16 was not observed. Unrelated dialogue/windows still block.
5. In Besaid, close the sphere insertion message once and request Path first.
   Expect the Destruction Sphere. On a first visit to Zanarkand, ask for Path with an active
   wrong panel between you and a permitted one. With Auto walk off, expect
   directions and a stationary player. Turn it on and request Path again.
   Leave the controls idle for several seconds first so the periodic panel
   scripts run. Listen for the named walking cue before movement. Repeated
   stale-facts refusals on a clear detour are a failure. Expect only the chosen
   panel to count and its arrival to be spoken once. Route loss on that touch
   is a failure. From rest beside an active wrong panel, insufficient clearance
   should give a spoken stop before movement. A reset, a different panel counting
   or movement after a refusal is a failure. If the route or observations become unsafe, expect a
   spoken stop. Continue manually or request a fresh route. After completing
   the pattern, try the next pedestal and then the exit when progression allows.

The user reported the earlier entrance-completion fix passing after the ride.
This follow-up has not been deployed or tested in gameplay; no actual speech
output or capture-to-audible latency is claimed.

Natural interaction interception substitutes only the qualified owned script
entry/choice and arranges its actual native solver job. Suppressing every script
is not solving; allowing the original sphere prompt and solver together is also
wrong. Kilika additionally reactivates its exact worker's native active bit before
dispatch. Signal/allocation/epoch/final RET ownership prevents a nested or prior
job from advancing the solver. Disable/cancel/failed transition retires the
owned job rather than leaving an interception armed indefinitely.

Treasure collection and trial/story completion are separate native phases.
Collected native markers skip already collected destruction treasure. Bevelle
uses its rail pedestal as the practical trigger and collects the original
destruction reward plus the remaining verified branch chest before its native
exit. The unverified extra chest count from the report is not invented.
Macalania's separate fayth branch was removed. Native normal first/return exits
and story gates control chamber access; no guessed post-Bevelle threshold or
direct chamber warp is used.

Zanarkand first-visit completion solves the normal gate and preserves the boss
sequence. Return/destruction solving is separate and story-gated. The player
must take the native leave/re-entry path to the next room/encounter; the solver
does not invoke the boss or lift as a completion shortcut. `zanarkand_patterns.cpp`
captures only the interactive display's actual sequence, with a per-display
serial and at most nine glyphs. All27 labels share verified model/colour/graphics
identity between floor, display review and guide; hidden future sequences are
not exposed. The display review is context-owned and supports Autodetail.

Evidence and safe extension: [trial design](../../cloisters.md),
[natural intercept matrices](../../trial_intercepts_20261016.md),
[entry/dispatch ownership](../../trial_entry_followups_20261016.md),
[Kilika initialization repair](../../help_lines_kilika_20261016.md),
[Bevelle treasure/fayth gates](../../bevelle_treasures_fayth_gates_20261016.md),
[Zanarkand labels/sequence](../../zanarkand_patterns_20261015.md).
Retain original EBP hashes, native integer/request/queue/activation replay,
choice-window ownership and entry bodies when extending. An entry name alone
does not establish its arguments or sequencing. User/tester confirmed all six
normal solvers and the Bevelle treasure/Macalania normal-story repairs; a new
report still overrides an older working note.

## Other progression puzzles

`via_purifico.cpp` has a separate default-Off collection action for the original
seven treasure callbacks; native collected bits prevent duplicates. Swim/depth
and exit navigation use the shared Navigation implementation. It is not a new
whole-story solver. [Via collection](../../via_purifico.md) and
[swimming blocker](../../via_swimming_blocker_20261012.md) identify dependencies.

`gagazet.cpp` supports room310's Wakka target and three-swimmer choice through
exact mtgz0700 input/hit operands. The default-Off automatic option preserves
native animations, reveal, completion and route flags. Manual Rikku chooses the
right green opening, Tidus the left blue opening, Wakka the remaining lower red
opening. A wrong partial Rikku choice requires Back/re-entry rather than a silent
rewrite. Native completion bits distinguish the two normal passages from later
story-gated reward/replay bits.

Manual target cues observe the original simultaneous alignment flags0x40/0x80,
with one beep on each native targeting-window entry. October19 removes the
unrequested calibration throw and predicted launch cues. Listen through a couple
of passes and start throwing before the beep, so the ball arrives during alignment.
The retained motions have36+19 animation frames; overlapping script waits and
native animation speed/half-step handling do not by themselves establish a fixed
ring-clock/input-to-hit lead. No manual success, progress or input is changed.
The original30-stage schedules exercise all three speeds with unchanged native
hit predicates. Audio uses the existing separate gate/cancellation and obsolete
queue cutoff. Actual human/device reaction timing remains a gameplay check.

After either trial, return through its local water exit to Mountain Cave; the
next section is reached from the dry cave, not across the same pool. The cave's
Prepare swimming party event and native initializer establish the swimmers and
barriers. Warping an early save alone does not establish that progression.
[Gagazet source/route evidence](../../gagazet_cave_20261016.md),
[October18 historical timing](../../dylan_reports_20261018.md) and the retained
30-stage/MGRP proof explain labels, native counters and current tests. User
confirmed earlier auto solving/returns/cross-water work. New guidance and audible
reaction timing remains a gameplay check; offline hit-window checks are not that
confirmation.

## Minigames

[Optional minigame completion](minigame_completion_20261020.md) records the four
independent default-Off assists: Calm Lands Chocobo training, Remiem race, either
Makalania butterfly course, and Blitzball. November3 repairs setup visibility
across24 retained native Blitz resources and uses the live clock independently
of possession/current worker. All nine native variants, shared utility and
menu/settings/profile checks pass. The warning-free scanned release-flavor
payload is guarded-deployed; prior installers/packages and the remote remain
unchanged by this task. Actual gameplay/audio confirmation is pending.

`jecht.cpp`/`jecht_voice.cpp` own S.S. Winno practice and timed memory prompts.
Default-Off automatic success uses the original practice/memory/result path;
manual cues speak the visible direction plus Confirm, with center requiring
Confirm alone. Captured compound directions and configured bindings matter.
Taunt suppression is scoped to the owned challenge/caption, not general dialogue.
[Jecht evidence and controls](../../jecht_shot_20261014.md).

`lightning.cpp` reads the native Thunder Plains warning-flash producer, offering
an independent default-On warning and default-Off native auto-dodge. Menus,
dialogue, focus loss and battle refuse assistance. The cue precedes the native
dodge window; it is not guessed from an unrelated actor animation.
[Lightning evidence](../../thunder_lightning_20261008.md).

`minigames.cpp`/`minigame_memory.cpp` capture original Chocobo training/race state,
statistics and relevant actor movement into a bounded preparation domain.
Context-owned F1–F5 reports and race F6 pickup cue do not acquire global field
key ownership. `butterfly.cpp`, `butterfly_processing.cpp`,
`butterfly_navigation.cpp` and `butterfly_paths.cpp` qualify the active hunt,
original course/actors and collected/current positions. Blue pickup guidance
and red-hazard warning are distinct sounds; F6 changes only that context's cue.
[Butterfly lifecycle](../../butterfly_catching_20261008.md),
[path/course data](../../butterfly_paths_20261008.md) and
[race/audio acceptance](../../gameplay_acceptance_0.5.6.md) retain the detailed
fixtures and untested course limits. Do not turn ordinary NPCs into hunt targets
or retain a previous race/hunt after epoch/reset.

## Airship

October20: [Free Search audio ownership and loop repair](airship_audio_20261020.md)
replaces the failed temporary blanket mute, shares M's actual active search
state across entry routes, and fixes accepted-pickup toggle/generation checks.
Previews are restored. Both actual entry routes still need user confirmation.

`airship_search.cpp` is the existing **free-search** reader and optional approved
six-region picker. hiku2100 W2 Common00A3 captures displayed X/Y/Z operands;
owned code reproduces the displayed absolute/sign/truncated fraction digits.
Exact bytecode identity, control/focus and250ms freshness guard it. M reads
coordinates. The separate Hidden destinations setting defaults Off and admits
only the six user-approved regions. Its worker computes steering from copied
camera/cursor facts; native keyboard/controller adapters use configured movement
and a single Confirm, with manual movement/Cancel/setting/context/stall handback.
This does not disclose hidden passwords or automatically unlock arbitrary places.

The main destination list now has a separate accepted native layer reader,
`airship_destinations.cpp`. Native map382, visible cursor26/background10 and
selected row11..25 qualify the actual accepted texture. Exact prepared W9
index/scroll/ID is optional indexing metadata, never a gate on the shown label.
The following Proceed? image selector uses shown layer29 and its accepted native
texture variant0/1 for highlighted Yes/No, with an independent lifetime. Only the
focused image text is emitted. Original UI-language map substitution chooses
the verified transcription; there is no runtime OCR or hidden-list dump.
[Airship implementation, assets, ownership and tests](airship.md) is authoritative.
Both reader profiles pass the original-script/controlled-renderer fixture;
in-game prompt/focus/render timing remains a user-side check. The downloaded
save remains intact. Native Select destination/New location prompts retain their
existing TK producer; free-search controls/assistance remain separate.

## Verification and extension

The October20 [shortcut/password/input record](airship_keys_20261020.md) keeps
Shift+M for the six-region picker; the earlier F3 request was superseded. Guide
F9–F11 ownership now returns unused field keys to the game. Password readback
uses the native subject selector copied with the cells. Implementation and
offline checks are complete for this scope; gameplay and deployment are pending.

Run `run_tests.bat --utilities` for development/release trial and puzzle matrices,
`--jecht`, `--lightning`, `--butterflies`, `--minigames`, `--pedestal`, `--readers`
and `--navigation-audio` as affected. Offline scripts retain exact data/code
hashes and original execution dependencies; animation, physical route traversal,
actual input/render/device timing and story/save combinations are identified
separately. Add a puzzle only after its real first/return/request/RET/collected
states are understood. Do not write guessed completion flags or skip a boss to
make a fixture's expected final room appear.

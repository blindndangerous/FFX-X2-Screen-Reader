# Navigation, route validity, audio and movement

## Kilika arrival homing follow-up — November16

Scope: detached `4245b9d`, following the user's 18:59 Entry door recess and
19:09 Fire chamber wall recess failures. The explicitly supplied live log was
read without controlling the game. The 300–333ms interval starts at the **end**
of homing: it is the existing neutral settling period, not the homing duration.
Those old records contain neither the starting pose nor the exit reason, so
they cannot establish which condition ended either individual live correction.

The production walker had an unconditional 3000ms whole-homing cutoff and
steered back to the original navigation endpoint. Checking zone membership
only in the proximity branch did not remove that cutoff. A corrected native-ramp
fixture starts 4.5 units outside, at 10 units/second, 33ms frames and one-frame
input latency. Linked against the original walker, it fails at 3333ms with
player Z=16.996, outside the rectangle starting at Z=17. The old diagnostic
assertion alone is disabled in that comparison because the old walker has no
diagnostics. An earlier test setup incorrectly counted its artificial pose
displacement as a native stride; the retained fixture starts native sampling
after setup, and the original-walker comparison uses that corrected fixture.

The owned navigation worker now computes one cached correction route from the
observed outside pose with the existing interaction-zone/floor/obstacle solver.
It publishes the next steering point, nearest reachable inside endpoint and
unreachable result as copied facts. Target or collision replans invalidate that
cache. Native steering uses the correction point, stops for actual membership,
and confirms success only after neutral input settling. It can resume if coast
leaves the zone. The 3000ms limit now measures lack of improvement by more than
0.1 units toward the steering point; a separate 15000ms total cap bounds repeated
progress or changes. Blocked pedestal segments, unavailable camera and existing
manual/context cancellation still release input. Push/contact routes retain
their existing behavior. No floor scans, route allocation, waits or log formatting
were added to the native owner.

Every outside-zone outcome captures player X/Z, original destination, correction
point, nearest inside endpoint and validity, distance, zone shape/centre/end,
angle/width/depth/range/height, player/zone Y, obstacle and radius, homing duration,
total duration, time without progress, unreachable status and homing exit reason.
The existing log worker formats these owned records. Successful homing also
records duration and exit reason.

Verification: the focused production walker/route fixture passes 384 combinations
of 0.2/2/4.5-unit starts, cold/warm ramps, 17/33ms frames, one/two-frame input
latency, 10/50 units/second, blocked movement and four original endpoints
(including a far-side endpoint). Longest successful synthetic recovery: 4059ms.
Blocked cases assert complete diagnostic records and one failure announcement.
An additional disconnected-floor case checks no movement and one bounded failure.
Full native suite and final no-deploy build exit 0: 90 PASS groups plus proxy
forwarding, with the same three missing-evidence SKIPs for readers, reachability
and utilities. Existing C4244/C4456/D9026 warnings remain. Receipts and hashes:
`.local/homing-verification.json`, `.local/homing-tests-final.log` and
`.local/homing-build-final.log`.
Test executables use `%TEMP%/ffx-homing-4245b9d` because the sandbox cannot write
the default external test-build directory. No deployment or commit is authorized.

Prioritized nonvisual gameplay procedure, after a separately authorized deployment:
1. At Kilika guide step 2, select Entry door recess and start Auto walk from the
   previously failing approach. Expect one arrival after correction and settling;
   Enter must address that recess. Repeat from close range and after moving away.
2. Progress normally to step 10 and repeat for Fire chamber wall recess. Test both
   a cold start and an immediate retry. No early arrival or duplicate failure
   should precede the final result.
3. Cancel a correction with manual movement. Expect immediate input handback.
   If a route cannot enter the zone, expect one failure and stopped movement;
   preserve the new geometry/duration/reason records from that same session.

Limits: synthetic geometry and native input-ramp adapters do not prove the live
Kilika collision layout, actual controller consumption, worker scheduling or
audible output. The 15000ms cap is a safety bound, not an expected response time.
Existing route-solver endpoint selection and copied-scene freshness remain the
limits on nearest reachable point selection; no new live capture was performed.

## Kilika evening-run repairs — November16

Scope: HEAD `e79200a`, the user's November16 17:25–18:03 session. The supplied
log was rotated during investigation; its matching 295683-byte archive is
`logs/FFX_SR_Native-20261116-181941-718-000.log` beside the game. A read-only
copy is retained in `%TEMP%/ffx-kilika-e79200a/original-session.log`.
No deployment, commit, game control, injection or Tester Logs access occurred.

Confirmed causes and changes:

- Step 22 used an authored label absent from the destination list. It now
  names `Exit 1 to Kilika - Antechamber`. The existing identity resolver still
  selects local worker 37, not worker 39 (`Exit 3`), regardless of list order.
- Step 21 treated `0x0c000063=3` as permanently sufficient. Original retained
  `klyt1000` worker 36 writes that milestone at PC `0xe679`; reinsertion writes
  the carried sphere to `0x0c000062` without clearing the milestone. Removal
  clears `0x62`. The completion predicate now also requires `0x62=0`; the
  Kilika final-step correction retires its old progress bits when refilled.
  Existing scene/message ownership still defers advancement until control returns.
- Ordinary arrival stopped within 0.25 units of its chosen endpoint even when
  the actual interaction predicate was false. That produced repeated zero-distance
  homing attempts. A copied pedestal obstacle also disabled all correction,
  even across the room. Endpoint proximity now requires actual zone membership;
  correction checks the segment against the pedestal keep-out instead of rejecting
  every nonzero obstacle radius. Native pulse/coast settling and one final outcome
  remain. A 33ms warm-ramp fixture exhausted the old 2000ms budget 0.035 units
  outside the zone. The correction budget is now 3000ms plus final settling,
  with a 3500ms whole-recovery fixture bound. No waits, queues or allocations
  were added to the native movement owner.
- Loading duplicates came from a returning Yes/No handler after it opened a
  status popup. The popup reset the cursor latch and replaced the saved prompt;
  the old handler then emitted `Yes`, followed by Autodetail's copy of the new
  loading prompt. A native popup revision rejects that obsolete return and
  latches its retired cursor for the next idle call. This is event-lifetime
  validation, not text or time-based suppression. Real reopened questions and
  repeated text remain supported.

Step 19's initial long route is **unresolved**, not claimed fixed. The log lists
the pedestal as inactive at 17:50:04 and 17:51:27. Existing obstacle retirement
and a strengthened straight-route fixture reject the proposed stale keep-out
mechanism for an inactive pedestal. `Route to ...: 9 southwest` at 17:50:50 is
the camera-cut producer's next-leg description; a production fixture confirms
that this event changes neither the route goal nor its final endpoint. It is
not evidence of a replan. The session records no successful-route coordinates,
floor snapshot/remaps or obstacle dump, and the retained evidence here lacks
the Kilika collision mesh. The initial south/west/north route therefore cannot
be distinguished from a necessary lower-floor approach or a route-selection
defect. No obstacle or floor rule was weakened on that assumption.

The requested no-deploy build and full `run_tests.bat` both exit 0. The suite
reports 90 PASS groups plus proxy forwarding, with three existing missing-evidence
SKIPs: readers (`hudTextureClipBody.bin`), reachability (`collisionReplayBody.bin`)
and utilities (`maps_v3.json`). Existing C4244/C4456/D9026 warnings remain.
Receipts are `.local/kilika-build-final.log`, `.local/kilika-tests-final.log` and
`.local/kilika-verification.json`; focused failing/passing fixture logs share the
`.local/kilika-` prefix. New fixtures exercise real guide selection/progression,
real menu hooks and owned speech with Autodetail on/off, 96 arrival ramp cases,
safe/unsafe pedestal correction, camera-cut route retention and inactive-pedestal
straight-route behavior. Menu speech passes 407 checks. All 96 arrival cases
pass; longest successful recovery is 2970ms, and longest blocked recovery is
3366ms including final settling. The prior 192 route/input and 96 pedestal-ramp
cases remain green. Retained ATEL reads confirmed the door writes; fixture
movement remains synthetic and does not establish actual game collision or speech.

Prioritized nonvisual gameplay procedure, after a separately authorized deployment:

1. Load the Kilika save. Expect one initial `Yes`, then one loading status after
   confirmation. Repeat with Autodetail on and off; a reopened question must
   still speak its initial answer.
2. At step 2, request the Entry door recess walk once. Expect one reached notice
   followed by a usable native interaction. Failure: an outside-range stop or
   needing a second walk. Confirm manual movement/cancel still stops assistance.
3. Complete the existing pedestal push and steps 16–18. At step 19, note the
   initial route and any camera-change description separately. Walk to the
   lower recess once and insert the Destruction Sphere. Then walk to the
   revealed chest once and collect it. Both arrivals must permit interaction;
   physical obstructions must still stop assistance rather than push the pedestal.
   The long initial step-19 route remains an open observation, not an accepted fix.
4. After the chest, remove the final door sphere and wait for native control.
   Expect step 22 naming `Exit 1 to Kilika - Antechamber`. Reinsert the sphere,
   close its message and wait for control; expect step 21 on repeated reads and
   after reopening the guide. Remove it again; expect step 22. The guide path
   must choose the same exit as the named list row.

Delegation: Luna performed one bounded read-only decoder-location search. It
found the existing owned-mesh decoder but no retained Kilika mesh or EBP-floor
recipe. Root inspected the result; no delegated edits or correctness claims.

## Visible-message hints — November16

The reported `window=0 message=1 allowed=1 control=0` state does not prove that
dialogue is visible. `window` is the mod's scene ID; `message` is native message
mode, which can remain set while a script owns the player. The guide previously
treated either value as a reason to press Enter.

Navigation capture now copies a separate `visibleMessage` boolean using the
existing safe state readers: one of eight TK slots must have an open window
(phase 1 or 2, matching dialogue review) and a readable, nonempty encoded text
source. The fixed eight-slot read stays on the native owner. Workers receive
only the boolean; no pointer, wait, text decoding, or new speech producer is
added. Raw message/control guards still prevent movement and step advancement.

The guide uses the visible-text fact for its close-message hint. Without it,
script/control holds use the existing scene-or-push wait hint. Auto walk uses
the same distinction and logs `visible-message` alongside the original fields.
Scene refusals retain their existing wording; disabled control or raw message
mode without visible text asks the player to wait.

The native test runner sets `SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX`
before launching its children. External evidence loads also set it before
possible pre-main exceptions, including the scene fixture's static loads.
An already-built stale executable must be rebuilt or run through the harness
to receive this protection.

Offline checks cover 192 TK slot/phase/text cases and 32 calls to the production
guide across control, message mode, scene ID, visible text, and push state.
Restoring the old guide branch makes the new production-guide fixture fail.
The requested no-deploy build exits 0. The full native suite exits 0 with
89 PASS groups plus proxy forwarding and three existing missing-evidence
SKIPs (readers, reachability, utilities). Installer verification passes 667
assertions and UI compilation; PRISM-dependent cases remain skipped.
Existing C4244/C4456/D9026 warnings remain. Receipts are
`.local/message-build.log`, `.local/message-tests.log`,
`.local/message-installer-tests.log`, and `.local/message-verification.json`.
These checks do not establish actual rendered visibility or audible delivery
in a running game. No deployment, commit, game access, or tester-log access is
part of this task.

Prioritized nonvisual procedure, after a separately authorized deployment:

1. During the pedestal sphere script or a scene with no visible text, request
   the selected guide step with F11. Expect the scene-or-push wait hint, never
   a request to close a message. Request Auto walk to a selected target; expect
   a wait/scene refusal and no movement while native control is held.
2. Open an ordinary dialogue window, then request the guide and Auto walk.
   Expect one close-message hint and one message-open refusal per request.
   Close the dialogue normally before continuing progression.
3. After native player control returns, request both again. Expect ordinary
   guidance and navigation; no stale close-message hint or duplicate speech.

## Kilika steps 16-17 after the completed push — November16

The user confirms that `19cca78` completes all three step-15 legs and says
“Pedestal push complete” in gameplay. This repair preserves that push path and
its speech. The supplied evidence is `.local/r18-step16-17-live.log` and
`.local/r18-steps16-22.log`. Enter presses after failed walks belong to the test
driver; their subsequent sphere exchanges are not mod progression failures.

The confirmed routing defect is an inactive contact pedestal remaining an
obstacle. `navigation_scene.cpp::pedestalZone` derives inactivity from the copied
native contact channel, but `nav_zones.cpp::avoidContactPedestals` previously
ignored it. It also left a previously copied obstacle intact when its source
ceased to qualify. `contactRoute` then required a straight outward escape to the
clearance ring before routing anywhere else. In a connected narrow pocket that
escape can leave the mapped floor, producing “no reachable interaction zone”
for both a distant door and a nearby recess. The live log independently lists
the post-push pedestal as inactive. Retained `klyt1000` positions and a synthetic
connected slope/pocket reproduce the refusal and its repair for both named
targets. Ordinary targets now rebuild their keep-out from currently active
contact pedestals; push targets retain their own staging and obstacle rules.

The first step-16 failure also exposes premature arrival reporting.
`zoneRoute` already checks that the returned endpoint is inside the interaction
zone. However, `nav_cues.cpp::advance` announced success on a transient copied
in-zone pose, and the propagated obstacle disabled ordinary inward homing in
`auto_walk.cpp`. Native settling could subsequently fail after that success.
The 367 ms live interval matches the existing 300 ms settlement path. The log
does not contain coordinates proving native coasting, an invalid endpoint, or a
disconnected player polygon; those precise live mechanisms remain unmeasured.

Ordinary assisted talk routes now carry a unique arrival token. They publish
provisional arrival to the input owner, then announce success only after native
settling confirms current zone membership and facing. Exact coincidence needs
no turn. Recovery follows the verified route endpoint rather than an off-floor
actor centre, uses bounded pulses and waits through ten native input updates.
Its pulse estimate comes from completed pulse-and-coast displacement using the
retained native ramp formula. The ordinary home budget is 2000 ms; the legacy
push/home behavior retains 1500 ms. The 300 ms movement-stall timer does not
mistake intentional ordinary input draining for a wall. The 32 synthetic ramp
cases cover 10/50 units per second, 17/33 ms frames, one/two-frame publication
latency, cold/warm opposite input and blocked movement. Their longest simulated
recovery and settling interval is 2376 ms, below the fixture's 2500 ms bound.
These are simulated input-owner intervals, not gameplay or request-latency
measurements.

Tokens retire on cancellation/replan, preserve same-walk settling across target
updates, and complete before a subsequent target move/disappearance can replace
a confirmed result. Failed correction and later worker retirement share one
failure announcement. Passive selection beacons keep their existing arrival
behavior even when the Auto walk setting is enabled. Step-15 contact/push routes,
swimming, crossing and butterfly arrival protocols do not use this handshake.

The repeated busy notice was a separate lifetime bug: each successful scene
capture reset the pressure latch even if the worker had drained only one of four
held slots. The latch now rearms after a fully drained pool or a new observation
revision. A standalone synthetic observation fixture proves the old repeated
notice, intermittent refill suppression, drained recovery and revision reset.

Explicit failed interaction routes now log the player floor polygon and floor
component, native floor ID, target geometry, up to 64 intersecting target-zone
polygons with their components/collision groups/endpoint eligibility, the active
keep-out, and up to 16 contact-pedestal source rows including inactive sources.
Counts disclose truncation. Components describe collision-passable floor portals;
one-way event constraints are reported separately by readiness/count. All scans,
formatting and logging use owned data on the preparation worker. No native wait,
queue wait, lock, I/O or new game pointer is introduced.

Changed runtime files under `src/native`: `auto_walk.cpp`, `nav_audio.hpp`,
`nav_cues.cpp`, `nav_zones.cpp`, `navigation.cpp`, `navigation.hpp`,
`navigation_observations.cpp` and `puzzle_guide_owned.cpp`.
Changed fixture files under `src/native/tests`: `auto_walk_fixture.cpp`,
`nav_cues_fixture.cpp`, `navigation_cancel_fixture.cpp`,
`navigation_zone_followups.inc`, new `navigation_observations_fixture.cpp`,
`pedestal_fixture.cpp`, `reader_fixture.cpp`, `utility_fixture.cpp` and
`run_tests.bat`. The three small fixture adapters preserve the existing
two-argument audio-route host while accepting explicit assisted-route intent.
Documentation changes are this record and `CODEX_MEMORY.md`.

Final `cmd /c "src\proxy\build_dinput8.bat --no-deploy"` and
`src\native\tests\run_tests.bat` both exit 0. The suite records 87 PASS groups
and three existing evidence-dependent SKIPs: readers (`hudTextureClipBody.bin`),
reachability (`collisionReplayBody.bin`) and utilities (`maps_v3.json`). The
192-case step-15 route/input matrix and 96-case step-15 ramp matrix pass, as do
the new ordinary-arrival, token-lifetime, geometry/diagnostic and pressure cases.
The existing C4244 build warning and C4456/D9026 harness warnings remain; no new
compiler warning was introduced. Final receipts are `.local/r18-build-final.log`,
`.local/r18-tests-final.log` and `.local/r18-verification.json`.
Focused receipts are `.local/r18-geometry-final.log`, `.local/r18-arrival-final.log`,
`.local/r18-cues-runner.log` and `.local/r18-cues-final.log`; red receipts include
`.local/r18-geometry-red.log`, `.local/r18-cues-red.log` and
`.local/r18-cues-retirement-red.log`. A bounded `gpt-6-luna` delegation supplied
the busy-latch repair and synthetic fixture, with baseline red/current green
execution; the primary agent reviewed and integrated it. An independent agent
using the inherited primary model reviewed arrival lifetime/callers and ran the
ramp matrix; its findings were repaired and incorporated into maintained tests.
No unresolved delegated runtime finding remains after integration. No game,
Tester Logs, deployment, packaging, commit or remote write was used.

Prioritized nonvisual gameplay procedure, for a separately authorized test:

1. Retain the working step-15 setup. Confirm all three push legs and exactly one
   “Pedestal push complete”; movement input must still cancel assistance.
2. After that completion, select Lower Kilika Sphere recess and press backslash.
   Expect a route and one settled arrival, without the subsequent out-of-range
   failure. Press Enter only after successful arrival/within-range feedback,
   then take the third Kilika Sphere and close the native prompt.
3. From that lower recess, select Antechamber door recess and press backslash.
   Expect a connected route and one arrival. Insert the carried Kilika Sphere
   only after arrival. If routing refuses, retain the new explicit failure dump;
   the supplied older logs cannot identify the actual player component.
4. Check the nearby lower recess again when the guide calls for it. An in-range
   target should report its range/facing or settle successfully; no inactive
   pedestal escape or repeated busy announcement should intervene. A persistent
   blocked correction must produce one failure and no preceding success.
5. Continue steps 18–22 in order: take the Destruction Sphere, put it in the empty
   lower recess, collect the revealed treasure, recover the door's Kilika Sphere,
   then exit. The lower sphere must be taken before door insertion, and the lower
   recess must be empty before accepting the Destruction Sphere. Do not press
   Enter after a failed walk; that was the supplied driver's sphere ping-pong.

Offline limits: no live player coordinates/floor/remaps were captured, no original
Kilika floor was replayed, and native movement/collision/facing/audio remain
synthetic boundaries. The route-pocket fixture proves the mod's phantom-obstacle
failure mechanism on a connected floor; it does not prove which live polygon
failed. Actual gameplay timing, collision response and audible output remain
unverified. No commit, deployment or game access is authorized by this task.

## Kilika step-15 native input ramp repair — November16

This supersedes the live-success assumptions in the staging records below.
The supplied `.local/r17-step15-live.log`, 06:58:15–06:58:45, still loops
laterally and ends at the existing whole-approach deadline on `376a236`.
At 06:58:33.905, 0.631 is the distance to the effective radial-entry waypoint;
the player remains about 10.89 units from the pedestal. The outside-contact
stage accepts radius 7.1–9.0 with lateral error at most one unit. Consequently,
that observation alone does not establish that the contact handoff was missed.

The confirmed input defect is that continuous full-strength octant steering
ignored native directional acceleration and deceleration. Hypothesis (b) holds
at that boundary. The previous ideal plant changed velocity immediately when
the command changed, so it could not test this behavior. This is evidence for
the reported hunting mechanism, not proof that every live oscillation has the
same cause; a new live input trace is still necessary to exclude camera or
collision contributions.

Scoped retained-code analysis used `reader_reference_20261003_235502`, without
accessing an installed game or process. Installed native function `82D180`
maintains four independent 0–256 directional ramps. `82D234–82D324` increases
the held directions by 32 per field frame and decreases released directions
by 32. `82D329–82D566` also filters analog input through these ramps.
`82D56B–82D5FD` subtracts opposing ramps, computes vector magnitude, caps it
at 256 and treats magnitude below 48 as idle. Reversing a key therefore does
not instantly reverse motion. Neutral input can retain up to eight native
ramp updates. A two-frame pulse reaches 64; its release falls below deadzone.
A settled cardinal n-frame pulse, for n=2–8, covers `(n*n-2)/8` full-speed strides.
These are native frame counts, not measured wall-clock or hardware latency.

The actual approach input path is:

1. `input.cpp::inputEvent`, after native input aggregation `471D10`, calls
   `updateAutoWalk`, then `updatePedestalInput`. Precision push starts now leave
   a neutral mask for that event's sole update; eager start updates previously
   could spend two mod counter steps before another native ramp update.
2. `auto_walk.cpp` computes the effective world-space steering error, calls
   `relativeBearing`, rounds `atan2(pan,front)` to one of eight octants and
   chooses Up/Right/Down/Left combinations. Pedestal routes bypass ordinary
   250 ms octant hysteresis. This part has no magnitude control.
3. `nav_memory.cpp::cameraBasis` reads the camera pointer at reference RVA
   `1F11430` on the native owner. Its horizontal direction is camera
   `(+40,+48) - (+65C,+664)`; right is `(forward.z,-forward.x)`. Native calls
   do not inherit the worker's thread-local copied camera. Static lateral
   signs agree with that basis; there is no proved mirrored-axis defect.
4. `autoWalkKeyboard` adds the configured movement scan codes at the real
   DirectInput keyboard read. `autoWalkController` emits axes ±24000 before
   the controller bridge publishes. `controller.cpp::nativePad` preserves X
   and negates Y while converting signed stick values to native fields.
   The field consumer reads per-device history, not only aggregate held bits.
   Installed `82D070` reads that history; `82D180` filters it and calls
   `82B190` to set the movement direction at actor+168. This steering path
   does not wait for visible character-facing alignment. Exact visual turn
   rate and hardware cadence remain unmeasured.
5. Native movement obtains its camera angle through `83F2D0 → 7BCF20 →
   7C4990`, using the camera vector `(+50,+58) - (+66C,+674)`. The new trace
   records that vector's yaw alongside the navigation vector's yaw, both
   measured from world north in radians. Their live agreement is unverified.
   No camera-source change is justified from the supplied one-second samples.

Hypothesis (a)'s live camera mismatch or wall collision remains unresolved.
The exact retained Kilika script is available, but `mapout.vpa` for the fire
chamber was absent from both checked retained-evidence roots. There is no
decoded chamber floor fixture establishing walls at x ±5–7, z 132–134.
Hypothesis (c)'s heading gate is absent: `nav_zones.cpp::interactionEndpoint`
and `pedestal_input.cpp::pedestalWalkLeg` use position, height, radius and
push-side alignment; they do not require the player's facing angle for staging.

The repair changes only push-mode 1/2 input along the validated axial staging
suffix and contact leg. Detour steering retains its existing behavior. Near
the waypoint, the walker releases input, drains ten native input events
(eight ramp steps plus a conservative two-frame publication allowance), then
issues short pulses separated by another drain. Pulse length uses remaining
distance and observed stride; an unmeasured cold start probes with two frames.
An initial one-step braking region covers inherited movement before the first
stride observation. A new waypoint or changed octant cancels an old pulse and
drains before replacement. The existing geometric staging handoff still
releases in its transition update. These are nonblocking frame states, with
no sleeps, native waits, locks, actor writes or new speech. Push legs remain
north 2.3, east 4.5, north 2.5.

Temporary `pedestal input:` diagnostics run through the existing copied log
sink, only while a push approach/contact walk is active. Keyboard and pad
emitters share a 50 ms throttle, at most 20 records per second. Each records
monotonic time, native update interval (zero when initially unknown), command
age at emission, live player X/Z, effective target X/Z, desired world error,
actual emitted axes or configured scan codes, both camera-vector yaws and
direction bits. A neutral record retains the desired world error but emits
zero keys/axes. This is an emission trace, not confirmation that the game has
already consumed the device state. Worker logging formats the owned packet.

`pedestal_walk_fixture` now models native ramps, deadzone and one/two-frame
keyboard/controller publication delay. Its 96 cases combine cold/warm starts,
six fixed camera headings, 17/33 ms frames and 10/50 units per second. Warm
starts continue the outgoing lateral stride at the positive-X loop position.
They require centred south-side contact within 30 seconds and exactly one
existing push announcement. Separate checks cover an interrupted camera turn,
manual cancellation, rapid restart, trace fields/throttle and prior supplied
pose replays. These are synthetic movement/latency models, not native locomotion
replay or live collision evidence. The corrected fixture fails against the
unchanged HEAD input driver on missing initial braking
(`.local/r17-baseline-red-final.log`); the current focused fixture passes
(`.local/r17-focused-final3.log`). The existing 192-case production route/input
matrix also passes (`.local/r17-matrix-final.log`).

The requested no-deploy build exits 0 (`.local/r17-build-final.log`), retaining
the existing C4244 warning in `puzzle_guide_owned.cpp`. The full native suite
exits 0 with 85 PASS groups and three existing missing-evidence SKIP groups
(`.local/r17-tests-final.log`): readers lack `hudTextureClipBody.bin`,
reachability lacks `collisionReplayBody.bin`, and utilities lack `maps_v3.json`.
Existing test-harness C4456/D9026 warnings remain. The updated standalone
walker assertions require neutral input on all four movement keys, then the
first correctly directed pulse within 12 native updates; those assertions
pass in the final full run. Changed files are `auto_walk.cpp`, fixtures
`auto_walk_fixture.cpp`, `pedestal_fixture.cpp`, `pedestal_walk_fixture.cpp`,
this record, `CODEX_MEMORY.md` and `CHANGELOG.md`.
An independent Sol review identified stale pulse directions, eager counter
updates, a warm-pipeline fixture gap and incomplete neutral-key assertions;
those findings are repaired and tested. Runtime review found no further concrete
defect. Gameplay, actual input consumption and audible speech remain unverified.
No commit, deployment, packaging or game access is authorized.

Current prioritized nonvisual procedure, only after separately authorized
deployment:

1. At Kilika step 15, select Pedestal and start Auto walk from the northeast,
   then from either former lateral-loop position. Expect short pauses and
   controlled corrections at the axial entry, then south-side contact and
   the unchanged three push legs. Do not hold Confirm; it opens the sphere
   prompt. Require one existing announcement per push leg, no repeated
   approach announcement and no no-progress deadline. Continue steps 16–22
   only after the pedestal reaches its depression.
2. If hunting or wrong-side contact recurs, retain that attempt's bounded
   `pedestal input:`, approach/contact and stop records. Compare commanded
   bits with observed motion and compare the two yaw fields. Record whether
   the loop coincides with large frame gaps, old command ages or a camera cut.
   Offline checks cannot prove actual floor collision, consumption delay,
   visual turn rate, audible speech or trial completion.
3. During a pulse and during a neutral pause, test Backslash and physical
   movement cancellation. Restart immediately once; old input must not return.
   When progression permits, verify recess walks at steps 8, 10, 11 and 13
   still avoid pushing. Those ordinary detour/recess paths retain their input
   policy.

## Kilika step-15 native staging handoff — November16

This record supersedes the three unsuccessful live repairs (`ffd6fc9`,
`04d23b0`, `77043b9`) and their acceptance procedures below. The supplied
`.local/r16-step15-live.log` records the player passing the clearance entry
sideways, then standing at `(-7.682, -10.008, 132.661)` until timeout.
The preceding oscillation is retained in `.local/r15-step15-live.log`.

The missed waypoint was the radial entry, not the final outside-contact stage.
At `77043b9`, `auto_walk.cpp:346` held every reached intermediate push waypoint
until the worker published a different one. `nav_cues.cpp:127` advanced only
from worker observations. A crossing between observations therefore left the
native walker holding forever. `pedestal_input.cpp:328` separately measured
progress toward an ideal contact point, explaining the logged distance mismatch.
The previous convergence record's claim that this mismatch was harmless is
superseded by the shared-target requirement and implementation here.

The route now ends with at least four world units along the push axis. For
the reset north push, it first reaches the centre line farther south, then
walks north to the outside stage. Floor connectivity and pedestal clearance
still validate that suffix on the preparation worker. The native walker can
advance that validated radial suffix without waiting for another observation.
Other reached intermediate waypoints release movement and allow at most 250 ms
for worker advancement; a missed waypoint silently requests a fresh route.
A player stopped before handoff also requests a fresh route after 1.8 seconds.
The existing eight-request limit and whole-approach deadline remain.

`pedestalWalkLeg` receives the exact pose already read by the per-frame walker.
It tests staging and changes the owned session to contact in that same call,
releasing movement for the transition tick. The next tick uses the owned axial
contact leg even if the worker still publishes the staging leg. Native contact
ancestry, script/state/side guards and original north/east/north push plans remain
in place. No new speech string or announcement is introduced. Progress,
no-progress diagnostics and reported waypoint now share the effective steering
target, including lateral alignment, rather than an unrelated contact point.
When a line-rejoin steering point moves forward, improvement toward the previous
point preserves the progress clock; changing targets without movement does not.
All new native work uses fixed owned facts and bounded arithmetic; pathfinding
stays on the preparation worker, with no native waits or locks.

The new combined `pedestal_walk_fixture` links the real pedestal session and
walker. It interpolates both live traces at 17 ms ticks with one-second recorded
observations, freezes the copied worker leg, then lets actual input drive a
10-units/second synthetic plant. It checks same-tick release/contact handoff,
next-tick north steering, matching progress diagnostics, one push announcement,
native north contact, and bounded stationary/missed-waypoint recovery. Existing
route/input matrices retain six camera headings, four strides and delayed
worker observations. These are offline synthetic movement/API/speech boundaries;
they do not establish native inertia, collision timing or audible speech.
The axial-route and indefinite-hold regressions failed before the repair. A
long moving-target detour also failed its added regression, then passed after
the progress-clock correction (`.local/r16-moving-target-red.log` and
`.local/r16-moving-target-green.log`).

Final validation: `cmd /c "src\proxy\build_dinput8.bat --no-deploy"` exits 0
(`.local/r16-build-final.log`). The full `src\native\tests\run_tests.bat`
exits 0: 85 PASS groups, 3 SKIP groups, no failures
(`.local/r16-tests-final.log`). Skips retain the existing absent
`native/hudTextureClipBody.bin`, `native/collisionReplayBody.bin` and
`offline-output/FFX/20261003_audit/maps_v3.json`; no installed-game read was
attempted to supply them. Existing C4244 build and C4456/D9026 harness warnings
remain; no new warning is reported for the changed runtime or combined fixture.
`git diff --check` passes. The matrix's contact handoff now refreshes its local
leg before evaluating clearance and recognizes native consumption of the
validated radial suffix; the full 192-case matrix passes with native contact
side/clearance and whole-approach limits retained.

Changed runtime files: `src/native/auto_walk.cpp`, `auto_walk.hpp`,
`pedestal_input.cpp`, `pedestal.hpp`, and `nav_zones.cpp`. Changed test files:
`src/native/tests/auto_walk_fixture.cpp`, `navigation_cancel_fixture.cpp`,
`pedestal_fixture.cpp`, new `pedestal_walk_fixture.cpp`, and `run_tests.bat`.
Documentation changes are this record, `CODEX_MEMORY.md` and `CHANGELOG.md`.
No commit, deployment, packaging or game access is authorized for this task.

Current nonvisual gameplay procedure, only after separately authorized deployment:

1. At step 15 with the reset pedestal loaded, select Pedestal and start Auto walk
   from the northeast, then from either former lateral loop position. Expect a
   centre-line approach from the south, a brief release at staging, one push
   announcement per leg, and north 2.3, east 4.5, north 2.5. Leave Confirm alone;
   it opens the sphere prompt. Continue to steps 16–22 only after the pedestal
   reaches the depression. Failure is renewed lateral hunting, an idle staging
   hold, wrong-side contact, a repeated announcement, or premature no-progress.
2. Verify Backslash and physical movement cancel immediately without restart.
   A blocked/missed approach must silently replan, then give at most one existing
   failure message within the retained request/deadline limits. Retain only the
   failing attempt's timestamps and approach/contact/stop diagnostics.
3. When progression permits, check sphere/recess walks at steps 8, 10, 11 and 13
   still avoid pushing. Offline fixtures cannot prove actual gameplay completion,
   floor collision response, hardware cadence or audible output.

## Kilika step-15 staging convergence — November16

Historical implementation and offline results; superseded by the native handoff
record above after the next live attempt failed.

The user's live test of `04d23b0` remained in staging until the 15-second
contact-progress stop. The authorized excerpt is `.local/r15-step15-live.log`.
The pedestal was at `(0.222, -10.005, 144.104)` with radius 7. The player crossed
between approximately X -5.7 and +5.6 near Z 133.4 while the intermediate
clearance waypoint stayed at `(0.222, -10.008, 133.398)`.

Two feedback problems reproduce in the production route/input fixture:

- `nav_cues.cpp::advance` advances intermediate waypoints from worker observations,
  using a one-unit reach/pass test. `auto_walk.cpp::updateAutoWalk` previously kept
  steering through the same point between observations. A crossing missed by
  the worker caused the native input owner to turn back through it repeatedly.
  The native input owner now uses the same reach/pass test and releases synthetic
  movement at a reached intermediate push waypoint. It requests a fresh scene
  once and retains that hold until the waypoint changes. Native processing
  continues immediately; cancellation, invalidation and progress limits still run.
- Final staging accepted only a 0.7-unit radial band and 0.65-unit lateral error.
  A 1.65-unit digital stride can skip both, creating a reversal cycle even with
  immediate worker observations. Staging now accepts radius plus 0.1 through
  radius plus 2, with lateral error at most 1 unit on the required face. The
  final input correction uses that same one-unit alignment. The original inner
  collision exclusion remains. The stricter contact-time claim remains below
  1.5 units across for Kilika and 1.8 for Besaid; native script predicates,
  contact radius, talk zones and converted contact endpoint checks are unchanged.
  A floor-projected staging goal within the new alignment band also selects this
  centring branch; a separate 0.9-unit goal-offset regression fails otherwise.

The logged `distance` is deliberately measured to final contact, rather than to
the displayed intermediate waypoint. It drives the approach timeout, not steering
or staging arrival. Its different value is not a mismatched steering target.
At radius 7, the displayed intermediate point is the route's outside ring:
`(7 + 3.5) / cos(pi / 16) = 10.706` units from the pedestal. Its Z coordinate
therefore agrees with the supplied live centre.

Regression coverage includes the exact reported northeast start and both sides
of the lateral loop, plus the previous wider-clearance case. The combined host
links production routing, waypoint publication and input steering. It exercises
192 combinations: four starts, six camera headings, four movement strides through
1.65 units, and observations every 33 or 198 milliseconds. Each case must hand
off staging before contact, retain detour clearance, touch only below the
1.5-unit lateral claim limit, and finish the approach within 30 simulated seconds.
The native pedestal fixture separately exercises seven-unit live collision
bounds, the enlarged outside staging handoff and northward native input.
Existing native fixtures retain the north/east/north plan and reject corner
contact. Speech strings, announcements and duplicate suppression are unchanged.
Focused input assertions also cover holding through delayed observations,
restarting toward the same waypoint and releasing the hold on the next leg.

The initial reproduction failed before repair. Delayed-observation cases also
failed without the native waypoint hold. Widening lateral alignment alone left a
radial reversal; outward-only correction and bounded prediction experiments also
left counterexamples. Those experiments were removed. The retained repair changes
only waypoint holding and synthetic outside-staging eligibility/alignment.
Focused navigation/audio checks pass in `.local/r15-staging-band-probe.log`.
The final `src\native\tests\run_tests.bat` run exits 0: 84 PASS groups, 3 SKIP
groups and no failures (`.local/r15-tests-final.log`). The unchanged missing
evidence is `hudTextureClipBody.bin`, `collisionReplayBody.bin` and `maps_v3.json`.
The documented installer suite also exits 0: 667 assertions and warnings-as-errors
UI compilation pass; PRISM-dependent cases skip because the locked runtime is
absent (`.local/r15-installer-tests.log`).
`cmd /c "src\proxy\build_dinput8.bat --no-deploy"` exits 0 for the final source
(`.local/r15-build-final.log`). Existing build C4244 and test-harness C4456/D9026
warnings remain; `git diff --check` passes. The 0.9-unit projected-goal test failed
before its matching steering predicate repair (`.local/r15-projected-goal-red.log`)
and passes in the final full suite.
Sol independently reviewed the final runtime and fixture changes without an
actionable correctness finding; the primary agent owns build/test verification.

This is offline evidence with synthetic movement on a free plane. The one-second
live samples do not establish exact native stride, worker cadence, inertia or
camera changes. Actual collision response, audible speech and gameplay completion
remain unverified. No game access, deployment, commit or packaging is authorized.

This procedure supersedes the earlier step-15 procedures below:

1. After separately authorized deployment, start step 15 with the loaded reset
   pedestal. Select Pedestal and start Auto walk from the reported northeast
   position, then from either side of the former staging loop. Expect one start,
   a south-side approach, an outside staging handoff and the native north 2.3,
   east 4.5, north 2.5 sequence. Leave Confirm alone; it opens the sphere prompt.
   Proceed to steps 16–22 only after the pedestal reaches the depression.
2. Check Backslash and physical movement cancel immediately, without restart.
   A blocked approach must release input and speak one no-progress reason within
   30 seconds, or after 15 seconds without contact-distance improvement. Retain
   only the failing attempt's timestamps and approach/contact/stop diagnostics.
3. When earlier progression is available, verify ordinary sphere/recess walks at
   steps 8, 10, 11 and 13 still avoid pushing the pedestal. Offline tests cannot
   establish native frame timing, real floor collisions or audible output.

## Kilika approach deadline and live contact follow-up — November16

The user reports an orbit and a south-facing stop at step 15; another in-game
attempt costs about 15 minutes of replay. This follow-up supersedes the earlier
step-15 acceptance procedure below. Source work and offline verification only:
no game access, deployment, commit or packaging is authorized.

Confirmed mechanisms and repairs:

- `pedestal_input.cpp` refreshed its progress clock on any player movement and
  exempted active approaches from its 15-second stop. Auto walk waypoint progress
  could keep an orbit alive without exhausting the eight-request retry limit.
  Kilika/Besaid approaches now retain a 30-second deadline across route requests,
  staging/contact changes, off-side touches and pending shapes. A separate
  15-second clock requires at least one unit of improvement in best distance to
  the final live contact point. Changing waypoints or moving sideways cannot
  refresh it. A new segment or a native push of at least 1.5 units starts a new
  approach budget; failed/short contacts do not. Accepted native contact starts
  the existing native-push progress clock afresh.
- `nav_interactions.cpp::refreshTargetZone` read the actor's live collision
  centre while retaining the target's old position. Subsequent `freshTarget`
  translated that already-current centre again; contact conversion also used
  the old target position. The refresh now pairs both positions from one live
  actor read. The owned scene builder already pairs them; it is unchanged.
- `updatePedestalOwnedTarget` accepted a delayed staging shape after the Native
  owner entered contact. That could replace the inward target with the outside
  staging ring while the session still believed it was walking into contact.
  Phase-mismatched shapes are now rejected and a fresh copied scene requested.
  Cancellation, input revisions, script identity and native ownership still gate
  movement. Native collision radius, 0.65-unit route alignment and the stricter
  native push-side guard are unchanged; no tolerance was widened.

Once per second during an approach, two bounded log packets copy elapsed/idle
time, segment, staging/contact/pending phase, request count, contact distance/best,
route availability, player/live-pedestal/waypoint positions, along/across offsets
and contact radius. With no active route, the waypoint field is the contact point
and `route 0` identifies that fallback. Timeout captures a final sample and one
spoken no-progress reason, releases synthetic input and retires the session.
Formatting and I/O remain on the existing logging worker; hooks do not wait.

Regression evidence: the orbit, shifted live reset centre and delayed staging
handback each failed before their repair. The synthetic pedestal fixture now
passes moving orbit/replans within 30 seconds, an improving approach stopping at
exactly 30 seconds, sideways oscillation stopping at 15 seconds, one final stop,
no restart, shifted south-side staging/inward route/native contact, delayed shape
rejection and one-second diagnostic cadence/content. The shifted case deliberately
uses a two-unit X/one-unit Z displacement from the nominal reset point to expose
mixed-centre handling. It does not establish that displacement in the user's game.
Final verification: `src\native\tests\run_tests.bat` exited 0 with 84 PASS groups
and 3 SKIP groups (two additional passing groups over the previous 82-group
baseline). `cmd /c "src\proxy\build_dinput8.bat --no-deploy"` exited 0.
Receipts: `.contact-tests-final.log`, `.contact-build-final.log`; focused pedestal
receipt `.contact-focused.log`. The existing evidence root is
`%TEMP%\ffx-r12-evidence`. Skips remain readers (`hudTextureClipBody.bin`),
reachability (`collisionReplayBody.bin`) and utilities (`maps_v3.json`). Existing
C4244 (`puzzle_guide_owned.cpp:67`), C4456 and D9026 warnings remain. Initial
focused/full-suite attempts hit C1088/C1085 (disk full); generated local build
objects and stale compiler scratch were cleared before the successful rerun.
No source assertion failed in that interrupted full run. `git diff --check`
passes. No game/process/probe, tester logs, deployment, commit or package access.

Changed runtime files: `pedestal_input.cpp`, `nav_interactions.cpp`; fixture:
`tests/pedestal_fixture.cpp`. `CHANGELOG.md`, `CODEX_MEMORY.md` and this record
retain the handoff. Review was local; no independent agent was used. These
fixtures prove the mechanisms above, not which one occurred in the user's replay.

Current prioritized nonvisual gameplay procedure, after separately authorized deployment:

1. With a Kilika Sphere loaded and the pedestal reset at step 15, start Auto walk
   from the reported northeast position. Expect one start, a detour to the south
   centre line, inward contact and the native north/east/north sequence. Also
   start directly south at the reset spot. Leave Confirm alone: it opens the
   sphere prompt. Complete steps 16–22 only after the pedestal reaches its goal.
2. A looping/blocked approach must release input and give one no-progress reason
   within 30 seconds, possibly at 15 seconds without contact-distance improvement.
   Expect no automatic restart after that stop. Backslash, physical movement and
   focus loss still cancel. Retain this attempt's timestamp and `pedestal approach`
   diagnostics, plus contact/off-side/stop lines; a failure needs only that attempt.
3. Check ordinary sphere/recess walks at steps 8, 10, 11 and 13 still avoid the
   live pedestal and do not push it. Offline fixtures cannot prove actual native
   collision timing, frame responsiveness, audible speech or the user's exact
   stopping coordinate. These remain gameplay checks, not claims of completion.

## Kilika step-15 stall follow-up — November16

The user reports four spoken no-progress stops on `0d7166e`, from the northeast
of the reset pedestal. That report supersedes the earlier offline-only Round12
status below. This follow-up remains local and uncommitted; no game access or
deployment is authorized.

The route builder already allows the final radial push-side leg inside its
wider keep-out. The input driver applied the detour's line-rejoin steering to
that leg, however, without first centring on the planned push axis. It also
judged progress by direct distance to the final destination, which can increase
through a valid detour. Stops were announced before the pedestal session retried
the approach, so silent starts still repeated the failure speech.

The final leg now centres within the existing 0.65-unit route tolerance before
moving inward. Other legs retain their clearance, immediate turns and avoidance
of unchecked sidesteps. Native push directions and the existing side/contact
ownership guards remain authoritative. Progress uses the current waypoint;
reaching a short waypoint while moving counts, but changing waypoints alone
cannot reset the 15-second timer. The pedestal session
owns approach retries, retaining its eight-request bound, and emits one final
no-progress reason after exhaustion. Silent Auto walk starts preserve their
stop-speech latch. Both movement owners capture player, pedestal and current
waypoint positions on a no-progress stop, using the existing bounded log packets.

The regression fixtures first failed for premature inward steering, repeated
stop speech, premature stall reporting and detour progress. A combined production
geometry/waypoint/input fixture also exposed lateral correction oscillation at
22.5 degrees with a 1.3-unit stride; using the endpoint's 0.65-unit tolerance
avoids that synthetic limit cycle. The fixture covers six camera headings,
three movement rates, a northeast start, retained clearance, aligned native
contact and cancellation. The pedestal fixture exercises eight stalled attempts,
one final reason and no further requests. These are synthetic input/collision
hosts, not native game movement, actual floor replay or audible-output proof.

Final verification: `cmd /c "src\proxy\build_dinput8.bat --no-deploy"` exited 0;
`src\native\tests\run_tests.bat` exited 0 with 82 PASS lines / 3 SKIP lines,
one additional PASS group over baseline. Receipts: `.kilika-build-final.log`
and `.kilika-tests-final.log`; test outputs use the established
`%TEMP%\ffx-r12-evidence\test-build` host with external evidence junctions.
Skips remain readers (`hudTextureClipBody.bin`), reachability
(`collisionReplayBody.bin`) and utilities (`maps_v3.json`). The unchanged
build warning is C4244 at `puzzle_guide_owned.cpp:67`; the unchanged test harness
also reports C4456 and D9026. Focused pedestal, combined route/input and steering/
speech fixtures pass. The added short-waypoint regression also failed before
its repair; stationary waypoint churn still cannot reset the stall timer.
No game bytes were added to source; no game process or tester logs were accessed.
The exact in-game stopping coordinate was not supplied, so this work cannot
establish the complete live cause.

Changed runtime files: `auto_walk.cpp`, `pedestal_input.cpp`, `pedestal.hpp`.
Changed fixtures: `auto_walk_fixture.cpp`, `pedestal_fixture.cpp`,
`navigation_cancel_fixture.cpp`. Updated documentation: `CHANGELOG.md`,
`CODEX_MEMORY.md` and this record. No commit, deployment or package was created.

Prioritized nonvisual gameplay procedure, only after separately authorized deployment:

1. At step 15, leave the pedestal at its reset position and start from the
   northeast, about 5.6 steps away. Expect one start announcement, a clear detour
   to the south centre line, contact, then the native north and east pushes.
   Follow the remaining north push and depression progression described below.
2. If an approach stalls, expect silent retries, then one final no-progress
   reason and released input. No retry or speech should follow that final stop.
   Backslash, manual movement and focus loss must cancel the session. Retain the
   timestamp and new player/pedestal/waypoint diagnostic if this fails.
3. On guide steps 8, 10, 11 and 13, walk to the sphere/recess targets. Expect
   clearance around the live pedestal, without pushing it. Any native touch
   must still release directions and recover within the existing touch bound.

## Kilika Round12 — November16

This worktree contains uncommitted repairs against deployed `cd26e6b`. No game
attachment, input, deployment, commit or cherry-pick occurred. Offline checks do
not establish successful gameplay or audible output.

Contact conversion previously discarded the planned push side and changed native
horizontal contact into a three-dimensional sphere test. The native session,
copied scene and direct refresh now retain the centre line and native vertical
bounds. The keep-out fallback removed in `052a5bd` stays removed. Round12 logs
contain relative offsets but no absolute positions, floor IDs or captured mesh;
the precise connected-floor refusal cannot be reproduced from those logs alone.

Route failures now have a separate revision from user cancellation. Ordinary
routes refresh their obstacle from the live pedestal snapshot. Pedestal detours
disable the 250 ms octant hold and unchecked recovery sidesteps. Automatic
restarts suppress destination announcements; a push leg announces only once.
The guide recognizes Kilika's reset pose and verified three-leg corridor, holds
its state during messages and names Enter to close them. The Glyph Sphere wall
placement is a checkpoint. Actual route data for steps 16–22 retains the native
depression, carried-sphere, socket, chest-bit and cleared-door dependencies.

The requested `cmd /c "src\proxy\build_dinput8.bat --no-deploy"` exited 0.
`src\native\tests\run_tests.bat` exited 0: 81 PASS lines and 3 SKIP lines,
matching baseline. Skips are readers (`hudTextureClipBody.bin`), reachability
(`collisionReplayBody.bin`) and utilities (`maps_v3.json`). The build retains
the existing C4244 warning in the unchanged chained guide-state initialization.
Test outputs used `%TEMP%\ffx-r12-evidence\test-build`, with read-only evidence
located through junctions to the established evidence directories. Receipts in
this worktree are `.r12-build-final.log`, `.r12-tests.log` and `.r12-focused.log`.
Focused fixtures cover centre-line retention after
refresh, slope bounds, radial escape, refreshed obstacles, immediate turns,
bounded recovery, speech repetition, cancellation and the actual Kilika route's
steps 14–22. Native resource/script/geometry tests that require unavailable
private evidence remain explicit skips; no game bytes were added to source.
The bounded Luna delegation read only the two named Round12 logs. Sol compared
the extracted offsets and failure order with the source; no delegated edits or
unresolved delegation remain.

Changed runtime files under `src/native`: `auto_walk.cpp`, `auto_walk.hpp`,
`butterfly_navigation.cpp`, `nav_zones.cpp`, `navigation.cpp`, `navigation.hpp`,
`navigation_intents.cpp`, `navigation_intents.hpp`, `navigation_observations.hpp`,
`navigation_scene.cpp`, `pedestal_input.cpp`, `pedestal_state.cpp`,
`puzzle_guide.hpp`, `puzzle_guide_data.hpp`, `puzzle_guide_owned.cpp` and
`puzzle_guide_progress.hpp`.

Changed fixtures under `src/native/tests`: `auto_walk_fixture.cpp`,
`navigation_service_fixture.cpp`, `navigation_zone_followups.inc`,
`pedestal_fixture.cpp`, `puzzle_guide_progress_fixture.cpp`,
`puzzle_guide_route_fixture.inc`, `reader_fixture.cpp` and `utility_fixture.cpp`.
Documentation changes are `CHANGELOG.md`, `CODEX_MEMORY.md` and this record.

Prioritized nonvisual gameplay procedure, after a separately authorized deployment:

1. At the current post-leg-two pedestal, request step detection. Expect step 15.
   Start the assisted push from the east pocket or south side. Expect a centred
   south approach, north push, native plunge and step 16. Repeat from the reset
   pose when available: north to Z170, east to X50.011, then north across
   Z196.276. Stop on any off-side-contact loop, refusal, unexpected stop or short
   push; retain the exact speech and log time. No failed route may blame input
   unless input occurred.
2. Follow steps 16–22 in order: take the lower Kilika Sphere, insert it in the
   antechamber door, take the Destruction Sphere, insert it in the emptied lower
   recess, open the treasure, remove the door sphere, then enter the antechamber.
   Confirm each expected step after closing messages. The depression must finish
   before the lower sphere, the lower recess must be empty before the Destruction
   Sphere, and the door must be extinguished before exiting. Any missing target,
   wrong sphere action, duplicated speech, route refusal or control loss fails
   this procedure. Offline flag fixtures do not prove these live paths, native
   animation timing, detour clearance under load or audible speech.
3. When replaying a pre-step13 save, check ordinary walks from beside the pedestal and
   past it, especially storage recess 1 to the fire chamber wall. Expect an
   outward first leg near the actor, clearance while passing, one destination
   announcement, no pedestal touch and no loss of player control. Interrupt one
   walk manually; expect it to remain stopped.
4. After the Glyph Sphere wall insertion, close its message with Enter and ask
   for the expected step: 14. After the reset tile, expect 15. With a message
   open, expect “Press Enter to close the message”; native scenes without a
   message keep the wait instruction. Re-approaches must not repeat either the
   destination headline or “Pushing pedestal” for the same leg.

[November1 Mushroom Rock/shared route repair](mushroom_rock_lag_20261101.md) is
offline verified and guarded-deployed:433 map assets/327 native floors audited,
20928 visibility/654 route comparisons and both reader profiles pass. Private
visibility acceleration retains exact routes while reducing sampled long-route
cost substantially. Actual game/frame/audible response remains unconfirmed.

Airship Cid identification and offline verification are **complete**; guarded deployment is complete; gameplay remains unconfirmed. Both reader profiles pass actual capture, selection, movement and worker transfer. [Airship Cid identity and tracking](airship_cid_20261020.md) records the native/model identity, live actor handling and test limits.

The Baaj report is closed by user gameplay confirmation: Besaid correctly remains inactive without its treasure, and every eligible seal works. Final door opening with all six prerequisites was not tested in this save. See [Baaj evidence and confirmation](baaj_seals_20261020.md).
## Identification and observations

`navigation.cpp::installNavigation` connects query input, native movement/actor/
interaction events, owned observations, reachability, pedestal and Auto walk.
`navigation_observations.cpp` captures the native scene; `navigation_scene.cpp`
prepares entities from owned parts/actors/worker interaction facts. Scene bounds
are500 workers,128 actors,64 physical parts and32767 vertices/triangles each.
These are transport limits, not claims that every room reaches them. Allocation,
worker/header, physical-part extent, epoch, scene and player/control identity
must match before ready facts are admitted.

`atel.cpp`, `atel_memory.cpp`, `nav_exits.cpp`, `exit_labels.cpp`,
`navigation_targets.cpp`, `navigation_labels.cpp` and generated/hash-qualified
catalogs identify exits, NPCs, events, objects, save spheres, shops and chests.
Native active channels/useful entries, actor binding/hidden/readiness and actual
interaction geometry determine whether a candidate exists. A nested warp in an
event is not automatically the event's category. Save spheres were audited across
all retained maps; same-room lift/door interactions are not assumed absent.
Stable worker/entry identities preserve numbering as other targets disappear.

The user's October18 exception permits reliable map/code/developer/HUD names
for **pathfinding identification** before a visible name reveal. It does not
permit invalid routes, invented entities or treasure contents. Treasure labels
remain type-only. New Yuna sphere and Dome staircase labels use exact retained
script length/checksum and entry2; native activation/collection remains in charge.

See [catalog and save-sphere audit](../../save_sphere_audit_20261009.md),
[exit/arrival identities](../../exit_status_swimming_20261009.md),
[map asset audits](../../navigation_asset_audit_20261007.md),
[native field activation](../../open_issue_repairs_20261012.md), and
[October18 missing events](../../dylan_reports_20261018.md).
`publishFieldArrival`/`announceMapArrival` require completed native load plus a
valid initialized controlled actor. Saved room numbers alone do not trigger an
arrival. Repeated genuine arrivals are preserved.

## Routes and validity

October23 clarification accepted by the user: list distance/bearing points to
the stored exit reference position; the route instead chooses a reachable
crossing of its native trigger area. These endpoints can produce different
directions near a wide exit. Both use the shared camera-relative direction
conversion. The user explicitly requested leaving exit behavior unchanged;
the two reported Baaj routes were not independently measured at that position.

The [Sea of Sorrow repair record](sin_sorrow_routes_20261020.md) documents the
implemented directed-event route constraints and bend-preserving speech change.
Source-free checks and the warning-free final developer build pass; guarded
deployment is complete; the user confirms Sea of Sorrow works on October20.
The later Sin gate repair and optional Nucleus collection module pass their
offline route/input/native-reader checks and warning-free build, and are
guarded-deployed. Approved exact City/Nucleus floors now pass original native
collision replay, including192 onward/return routes across all64 gate states.
Four missing platform boarding stops pass both reader profiles and are deployed;
full ride/gameplay confirmation remains pending.
See [later Sin obstacle audit](sin_late_obstacles_20261020.md) and
[Nucleus collection](../puzzles/sin_collection_20261020.md) for their separate status.

`nav_memory.cpp`/`nav_geometry.cpp`/`nav_zones.cpp` decode copied collision and
target shapes. `navigation_intents.cpp` and owned computation connect player
position to the actual usable interaction zone. The same committed route feeds
spoken path/distance, beacon, reachability and Auto walk. Avoid an independent
straight-line reachability shortcut in one consumer.

Native collision83E4E0 plus both switch tables establish dynamic group rules,
including aliases48..63 and the strict64 boundary. Captured remaps and actor
flags are evaluated against the real geometry; unsupported/unready data stays
unresolved instead of becoming passable. `reachability_capture.cpp` and
`reachability_worker.cpp` keep context/target identities with each result. The
default-Off Hide unreachable filter removes only a proved unreachable target;
pending/unknown data remains listed. Matching committed results survive a
compatible refreshed scene, while changed geometry, pose, gate or epoch retires
them. An empty/no-path result must propagate to every route consumer.

Water/depth routing is a three-dimensional transition problem. `swimming_route.cpp`,
`swimming_navigation.cpp` and generated Gagazet cross-water links join verified
water sections to their dry cave crossings; the path is not a direct swim
between disconnected pools. Native party preparation and story/puzzle gate
initializers still open the physical passage. Spoken direction/distance is a
presentation transform; Partial steps only changes rounding, never route
geometry or cue precision. Preserve native world/camera axes and interaction
endpoint height rather than flattening every floor onto X/Z.

Evidence: [reachability pipeline](../../reachability_filter_20261012.md),
[Besaid/no-path correction](../../help_navigation_quit_20261015.md),
[swimming progression](../../swimming_progression_20261009.md),
[Via swimming](../../via_swimming_blocker_20261012.md),
[Gagazet return/cross-water bake](../../gagazet_cave_20261016.md).
`navigation_asset_audit_20261006.md` and retained Offline scripts explain baked
data provenance. Regenerate/check the affected real map, including dry/wet and
gate states, rather than adding an unverified global passage.

## Audio

`nav_cues.cpp` owns a bounded POD route leg and camera-relative cue facts.
`wall_cues.cpp` computes walls from owned geometry. `nav_audio.cpp` receives
bounded commands and alone synthesizes/mixes the SDL stream after the physical
SDL owner establishes audio readiness. Native actor/movement callbacks do not
open devices or wait for computation/audio. Generation/context and independent
preview, warning, arrival, timing and position cancellation retire stale sounds.
Settings, focus, pause and route loss gate each relevant sound.

The current implementation uses stereo left/right, distinct rear pitch and
proximity/rate. Beacon and wall volume/range are separate; Arrival requires an
enabled beacon and actual final destination. Blue butterfly/race pickup cues,
red hazards and manual timing tones have independent gates. October18 Gagazet
launch tones add their own cancellation and reject commands older than100ms;
this is an obsolete-cue cutoff, not a hardware latency guarantee.

Full HRTF3D playback and a separate global sound/HRTF UI are not present in this
existing implementation; do not claim the maintained new-mod baseline is already
implemented. `audio_sample.hpp`, embedded resources and SDL license/runtime are
deployment dependencies. [Audio/route ownership](../../navigation_blitz_autowalk_20261007.md)
and [SDL owner evidence](../../sdl_owner_20261012.md) cover the current pipeline.
The real dummy stream fixture tests production mixed samples/gates; fake SDK
ownership tests and component timings remain separate from audible gameplay.

## Movement

`auto_walk.cpp`, `movement_keys.cpp`, `game_binding_input.cpp` and input assist
adapters use approved native input routes. Auto walk defaults Off; speaking its
path is a separate default-Off setting. Ordinary movement cancels immediately;
menus, dialogue, focus loss, combat, stale context, route loss and stall cancel.
Arrival does not automatically interact. Native input handback must survive
worker/SDK failure without waiting for a mod result.

`pedestal_input.cpp`, `pedestal_keys.cpp`, `pedestal_state.cpp` and
`pedestal_rules.cpp` track the native held-pedestal mode/interaction geometry.
Push/crossing/recess targets differ; a held pedestal cannot be treated as
ordinary free-walk input. [Pedestal input](../../pedestal_input_re_20261007.md),
[movement evidence](../../pedestal_movement_re_20261007.md) and
[current trial rules](../../trial_entry_followups_20261016.md) retain the exact
native bodies, transformations and validation. Do not bypass the Bevelle rail
pedestal requirement when adding a solver trigger.

## Extension and checks

Add identification through the qualified catalog/native name path, actual useful
entry and live zone. Preserve native active/hidden/collision and treasure rules.
Add route types through shared geometry/owned scene facts so filter, speech,
audio and walking agree. Worker jobs carry copied data, cancellation/version and
bounded computation; native callbacks perform bounded observation only.

Run `run_tests.bat --readers`, `--reachability`, `--navigation-geometry`,
`--navigation-audio`, `--pedestal`, `--native-actions` as affected. Tests include
original collision and script predicates, stale ownership, disabled/re-enabled
events, strict endpoints, disconnected floors and water portals. Controlled
poses, native API adapters and fake SDK callbacks are documented dependencies.
User confirmation after a known game build is still required for actual movement,
audio reaction and unusual maps. Installer/archive preservation is independent.

The October20 [Free Search audio repair](../puzzles/airship_audio_20261020.md)
supersedes release0.5.8's temporary blanket airship mute. The native permission
uses room382 or M's active search flag, with transition cancellation; the mixer
also checks each pickup's current generation and original category toggle.
Invalid field/race/butterfly loops are refused in search, while explicit
previews and separately gated events work again. This is offline verified;
the user's actual audible source and both menu routes await gameplay confirmation.
The [deferred Y question](../puzzles/airship_password_20261019.md#deferred-y-coordinate-question)
is unchanged.

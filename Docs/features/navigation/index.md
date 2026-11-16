# Navigation, route validity, audio and movement

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

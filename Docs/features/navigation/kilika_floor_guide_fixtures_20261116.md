# Kilika floor, guide fallback, and fixture startup - November 16

Scope: detached `0ba4a49`. The user confirms that this deployed revision's homing fix and single push cue worked in today's game session. This follow-up does not deploy, commit, package, control the game, or access Tester Logs.

## A. The stairs are required by the retained floor

The user's 20:32:53 and 20:34:53 diagnostics identify a player on the Y -10 floor and worker 35's recess on the Y +10 floor. The second route starts on polygon 273 at (83.890, -9.999, 175.936). Its straight line leaves the floor near (80.797, -7.415, 180.283), at fraction 0.129274. This is outside the settled pedestal footprint. Both effective and copied pedestal keep-outs are absent.

Read-only extraction was limited to `ffx_ps2/ffx/master/jppc/map/klyt/klyt10/bin/mapout.vpa`. SHA256: `18fc75824309143fe487532fd0477a250cf059e9d036afd9cce30a3e7159c97d`. The original map has 360 triangles, 270 vertices, and native float scale 91.41224670410156. It reproduces the four-point, 273.708-unit route exactly, through the stairs near X -43.495. Both logged starts have floor-continuous routes; both reject a direct shortcut.

The settled-state collision rows in the log open group 53 and block groups 54, 55, and 58. Even allowing every other mutable group cannot produce a shorter connection across this gap. Original worker 6 completion code pushes (53, 0) at 0x2E81 and calls Common4017 at 0x2E87, then pushes (55, 1) at 0x2E8A and calls it at 0x2E90. The setter's native remap semantics are established in [the Sin collision analysis](sin_late_obstacles_20261020.md). Group 55 contains polygons 0 and 1, the Y -10 extension toward the depression; the completion deliberately blocks them. No retained collision or script evidence establishes a walkable pedestal surface joining the two levels.

The user clarified that nobody manually crossed directly over the pedestal: only the routed walk reached the recess. Routing, height checks, native collision remaps, and drop protection remain unchanged. Long approaches whose endpoints differ by more than the existing eight-unit visibility height tolerance now use the neutral `interaction-route-height-change` classification. The request header is `route-diagnostic`. Full waypoint, component, obstacle, and first-gap evidence remains available. The label describes a height change; it does not grant a new crossing or claim that every long route is optimal. Same-height unexplained detours retain their original classification.

The retained-floor fixture fingerprints both binary inputs, reproduces the exact second route and first gap, checks both starts and the closed group-55 polygons, proves every returned leg has floor coverage, and checks the informational diagnostic. Existing synthetic disconnected-floor, narrow-gap, event, and pedestal tests remain relevant.

Evidence stays outside Git at `%TEMP%/ffx-0ba4a49-kilika`, including `mapout.vpa`, `triangles.bin`, `vertices.bin`, `facts.txt`, and `manifest.json`. The suite resolves those inputs through `%TEMP%/ffx-0ba4a49-tests/kilika-floor`. An absent retained floor is an explicit SKIP, never a substitute synthetic proof. Preparation uses the existing VBF reader and ChMap layout already used by the Sin floor fixtures: root at file+0x18; scale at root+12 divided by 10 and rounded to float; group table at root+u32(root+28); triangle count at group+8; triangles at root+u32(group+12); vertex count at root+10; vertices at root+u32(root+24). Triangles and vertices have strides 16 and 8.

## B. A guide request outside a trial must answer

The native F11 availability predicate handed ordinary-field input back to the game, including Shift+F11. The owned guide's existing fallback was therefore never reached. A separate shifted chord now owns that explicit field guide request. It says `No puzzle guide for this area.` on every independent press. Plain F11 remains native in ordinary fields; supported trial/Via/Gagazet guide actions, naming, menus, battle repeat, and Sphere Grid ownership keep their existing behavior.

Owned no-context manual guide actions use the same response. Automatic completion refreshes remain silent. The audit also found an early-startup submission path returning false without feedback; manual requests now say `Navigation is not ready. Try again shortly.` Automatic guide refreshes remain silent there. Existing unavailable collision, absent target, busy queue, missing observation, and thrown guide-data paths already produce explanations. The specific Zanarkand story/encounter prerequisite message remains intact.

Regression checks cover repeated field Shift+F11 output, supported inferred-step routing, contextual repeat behavior, five no-context owned actions, two silent completion actions, and startup refusal. The test dispatcher follows production chord precedence, falling back to the ordinary binding when a chord is unavailable.

## C. Standalone fixtures need their own crash policy

`navigation_scene_fixture.cpp`, included by the service fixture, did not map native message-mode getter RVA 0x46C840. Its fallback returned the numeric RVA itself. Production scene capture then called that number as a function, executing an unrelated address in the fixture process. A rebuilt baseline using the old adapter reproduces unhandled access violation 0xC0000005 at 0x0046C840, exit -1073741819. The older executable that produced the user's exact 0x0043E378/0x0046C8B0 write fault is unavailable; its exact instruction layout is not claimed as replayed.

The adapter now supplies a real controlled message-mode callback and owned empty message-window pointers. Unknown callable addresses fail with an explicit fixture assertion; absent optional globals resolve to null. The loader address remains an explicitly documented comparison token. Service coverage verifies both message modes through actual scene capture, then runs the existing route, query, guide, native-action, pressure, and reset checks. The fixture's semantic Auto walk/audio/utility endpoints were brought up to date with current production signatures. Actual walking remains covered by the dedicated walker fixtures.

The old suppression depended on executing an external binary-load helper or launching through the suite wrapper. This service fixture used its own stream-based EV loader and was absent from the default suite, so standalone execution could miss both protections. The service fixture is now part of the full suite.

Every executable entry source includes `fixture_runtime.hpp` at line 1. Its retained `.CRT$XCT` initializer runs before ordinary C++ static constructors, after CRT setup. It enables `SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX`, disables CRT abort UI/report-fault handling, directs CRT assertions to stderr, and sets process-local WER no-UI flags. The linker retention directive prevents an unreferenced COMDAT initializer from disappearing. Existing fixture calls reuse the same policy instead of overwriting its flags. The wrapper also inherits all three error-mode flags for loader-time failures. No system-wide WER or registry setting changes.

The runtime fixture checks policy before main and launches five hidden child failures with the parent's error mode cleared: read-only-page write violation, abort, uncaught exception, pre-main abort, and a CRT assertion. Each must terminate within five seconds with the expected nonzero status. The source census covers 92 executable entry sources. It verifies code coverage, not that historical binaries elsewhere were rewritten.

## Verification and limits

The requested `cmd /c "src\proxy\build_dinput8.bat --no-deploy"` build exited 0. The final `src/native/tests/run_tests.bat` run also exited 0: 96 PASS groups plus the proxy forwarding check. It includes both retained Kilika starts, guide fallback/startup checks, service integration, and all five deliberate failure subprocesses. Three existing evidence-dependent checks skipped because `hudTextureClipBody.bin`, `collisionReplayBody.bin`, and `maps_v3.json` are absent. Tests used `FFX_EVIDENCE_ROOT=%TEMP%\ffx-0ba4a49-tests`. Existing C4244, C4456, and D9026 warnings remain; the production build reports C4244.

The WER header's `small` macro collision with the Sphere Grid fixture was repaired. Final review added explicit CRT assertion stderr routing; its regression failed before that protection and the full suite passed after the fix. The height classification, field-key fallback, and startup feedback regressions also failed before their respective fixes. The final DLL SHA256 is `582162669b17fb4f6c6834018a97f23621bf0e7517750ec9f606c8683f55cce6`. `git diff --check` passes; HEAD remains detached at `0ba4a49`.

Receipts: `.local/three-verification.json`, `.local/three-build-final.log`, `.local/three-tests-final.log`, `.local/kilika-floor-red.log`, `.local/kilika-floor-green.log`, `.local/three-b-accessibility-red.log`, `.local/observations-red.log`, `.local/service-red-result.log`, `.local/service-green.log`, `.local/crash-baseline-final.log`, `.local/runtime-test.log`, `.local/runtime-assert-red.log`, and `.local/fixture-startup-census.json`.

Offline checks do not prove current in-game audio or native movement timing. No game deployment occurred. Original native movement, pointer ownership, and hook work are unchanged; new route classification runs in the existing owned diagnostic path. Startup suppression affects test executables only.

Luna performed read-only evidence location and bounded guide-key/test edits, followed by mechanical startup includes. Sol reviewed and corrected dispatcher precedence/counters, preserved the Zanarkand prerequisite message, fixed startup feedback and the fixture crash, diagnosed the floor, integrated all changes, and owns verification. No Astra or independent reviewer was used.

## Prioritized nonvisual gameplay procedure

1. After a separately authorized deployment, stand in Kilika Antechamber outside the trial. Press and release Shift+F11 twice. Each press must say `No puzzle guide for this area.` Silence, repeated unrelated history, or two replies per press is a failure.
2. In a supported trial, verify F11 still reads the selected step and Shift+F11 reads the inferred step. In a native menu or battle, F11 and Shift+F11 retain repeat behavior; naming readback retains priority. These checks require the corresponding game context, not a new puzzle progression.
3. On a suitable Kilika save after pedestal settlement, request the lower recess route. Expect the existing stair route and a normal interaction on arrival. Do not attempt an unsupported direct drop. The log may retain informational height-change details. This check depends on the existing pedestal/sphere progression; it is not required to repeat the user's already confirmed homing/push-speech test.
4. Fixture regression testing is developer-owned. No user desktop or game input is needed for the deliberate failure tests. Their child processes must exit with failure status without opening a modal error box.

## Changed source locations

The behavior and integration edits are:

- `src/native/nav_zones.cpp:349`: informational height-change classification.
- `src/native/navigation.cpp:220`: neutral diagnostic request label.
- `src/native/mod_menu.cpp:327`: field guide chord and fallback; registration near line 420.
- `src/native/puzzle_guide_owned.cpp:252`: owned no-context wording.
- `src/native/navigation_observations.cpp:57`: early-startup manual feedback.
- `src/native/tests/kilika_floor_fixture.cpp:1`: retained original-floor regression.
- `src/native/tests/fixture_runtime.hpp:1`: shared pre-main error policy and linker retention.
- `src/native/tests/fixture_runtime_test.cpp:1`: startup assertions and deliberate child failures.
- `src/native/tests/navigation_scene_fixture.cpp:32`: message-mode/window adapters and safe address fallback.
- `src/native/tests/navigation_service_fixture.cpp:39`: current semantic endpoints; message-mode regression near line 72.
- `src/native/tests/accessibility_fixture.cpp:136`: key dispatch helper and contextual guide checks.
- `src/native/tests/puzzle_guide_message_fixture.cpp:47`: manual unavailable-guide and automatic-refresh checks.
- `src/native/tests/navigation_observations_fixture.cpp:62`: startup refusal checks.
- `src/native/tests/external_fixture.hpp:2`: shared policy for external evidence consumers.
- `src/native/tests/run_without_dialogs.ps1:14`: inherited loader-time error mode.
- `src/native/tests/run_tests.bat:28`: runtime regression; service and retained-floor fixture integration.

Documentation changes also update `CODEX_MEMORY.md:3`, `Docs/controls.md:119`, `Docs/features/runtime/index.md:149`, `Docs/features/navigation/index.md:3`, and this record at line 1.

The remaining fixture edits are mechanical startup includes at line 1, with ten old error-mode calls replaced by the shared initializer. Complete entry-source list follows; unchanged fake DLLs and helper-only translation units are excluded.

- `src/native/tests/accessibility_fixture.cpp:1`
- `src/native/tests/atel_resources_fixture.cpp:1`
- `src/native/tests/auto_walk_fixture.cpp:1`
- `src/native/tests/battle_announcement_worker_fixture.cpp:1`
- `src/native/tests/battle_status_worker_fixture.cpp:1`
- `src/native/tests/battle_target_text_fixture.cpp:1`
- `src/native/tests/blitz_review_fixture.cpp:1`
- `src/native/tests/butterfly_fixture.cpp:1`
- `src/native/tests/butterfly_paths_fixture.cpp:1`
- `src/native/tests/butterfly_real_queue_fixture.cpp:1`
- `src/native/tests/context_fixture.cpp:1`
- `src/native/tests/controller_bridge_fixture.cpp:1`
- `src/native/tests/copied_help_worker_fixture.cpp:1`
- `src/native/tests/cross_water_fixture.cpp:1`
- `src/native/tests/dialogue_voice_fixture.cpp:1`
- `src/native/tests/direct_input_adapter_fixture.cpp:1`
- `src/native/tests/direct_keyboard_fixture.cpp:1`
- `src/native/tests/esc_worker_fixture.cpp:1`
- `src/native/tests/external_fixture_test.cpp:1`
- `src/native/tests/fixture_runtime_test.cpp:1`
- `src/native/tests/hook_abi.cpp:1`
- `src/native/tests/hook_batch_fixture.cpp:1`
- `src/native/tests/hook_calls_fixture.cpp:1`
- `src/native/tests/hook_native_actions_fixture.cpp:1`
- `src/native/tests/hook_overflow_fixture.cpp:1`
- `src/native/tests/hook_tail_fixture.cpp:1`
- `src/native/tests/hook_threading.cpp:1`
- `src/native/tests/input_dialog_focus_fixture.cpp:1`
- `src/native/tests/input_fixture.cpp:1`
- `src/native/tests/jecht_fixture.cpp:1`
- `src/native/tests/kilika_floor_fixture.cpp:1`
- `src/native/tests/lightning_fixture.cpp:1`
- `src/native/tests/log_capture_fixture.cpp:1`
- `src/native/tests/log_history_fixture.cpp:1`
- `src/native/tests/menu_speech_fixture.cpp:1`
- `src/native/tests/message_buffers_worker_fixture.cpp:1`
- `src/native/tests/minigame_completion_fixture.cpp:1`
- `src/native/tests/minigame_fixture.cpp:1`
- `src/native/tests/minigame_real_queue_fixture.cpp:1`
- `src/native/tests/movie_dialogue_fixture.cpp:1`
- `src/native/tests/native_action_storage_fixture.cpp:1`
- `src/native/tests/native_worker_index_fixture.cpp:1`
- `src/native/tests/navigation_cancel_fixture.cpp:1`
- `src/native/tests/navigation_fixture.cpp:1`
- `src/native/tests/navigation_observations_fixture.cpp:1`
- `src/native/tests/navigation_order_fixture.cpp:1`
- `src/native/tests/navigation_real_queue_fixture.cpp:1`
- `src/native/tests/navigation_scene_fixture.cpp:1`
- `src/native/tests/navigation_service_fixture.cpp:1`
- `src/native/tests/nav_audio_fixture.cpp:1`
- `src/native/tests/nav_cues_fixture.cpp:1`
- `src/native/tests/owned_atel_decoder_fixture.cpp:1`
- `src/native/tests/owned_mesh_decoder_fixture.cpp:1`
- `src/native/tests/owned_nav_target_fixture.cpp:1`
- `src/native/tests/owned_retirement_fixture.cpp:1`
- `src/native/tests/pedestal_fixture.cpp:1`
- `src/native/tests/pedestal_walk_fixture.cpp:1`
- `src/native/tests/prism_adapter_fixture.cpp:1`
- `src/native/tests/process_storage_fixture.cpp:1`
- `src/native/tests/profile_extensions_fixture.cpp:1`
- `src/native/tests/profile_guard.cpp:1`
- `src/native/tests/proxy_forwarding.cpp:1`
- `src/native/tests/proxy_startup_fixture.cpp:1`
- `src/native/tests/puzzle_guide_message_fixture.cpp:1`
- `src/native/tests/puzzle_guide_progress_fixture.cpp:1`
- `src/native/tests/reachability_cache_fixture.cpp:1`
- `src/native/tests/reachability_fixture.cpp:1`
- `src/native/tests/reader_fixture.cpp:1`
- `src/native/tests/route_format_fixture.cpp:1`
- `src/native/tests/runtime_tests.cpp:1`
- `src/native/tests/sdl_owner_fixture.cpp:1`
- `src/native/tests/selection_detail_worker_fixture.cpp:1`
- `src/native/tests/sensor_message_worker_fixture.cpp:1`
- `src/native/tests/settings_defaults_fixture.cpp:1`
- `src/native/tests/settings_worker_fixture.cpp:1`
- `src/native/tests/sin_collection_fixture.cpp:1`
- `src/native/tests/sin_collection_hook_fixture.cpp:1`
- `src/native/tests/sin_later_maps_fixture.cpp:1`
- `src/native/tests/sin_route_fixture.cpp:1`
- `src/native/tests/speech_capture_fixture.cpp:1`
- `src/native/tests/sphere_fixture.cpp:1`
- `src/native/tests/sphere_jump_ui_fixture.cpp:1`
- `src/native/tests/status_buffer_worker_fixture.cpp:1`
- `src/native/tests/system_menu_worker_fixture.cpp:1`
- `src/native/tests/text_plan_worker_fixture.cpp:1`
- `src/native/tests/utility_console_fixture.cpp:1`
- `src/native/tests/utility_feedback_ready_fixture.cpp:1`
- `src/native/tests/utility_fixture.cpp:1`
- `src/native/tests/utility_resource_fixture.cpp:1`
- `src/native/tests/wakka_fixture.cpp:1`
- `src/native/tests/wall_cues_fixture.cpp:1`
- `src/native/tests/warp_flag_fixture.cpp:1`

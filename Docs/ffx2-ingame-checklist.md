# FFX-2 main-story in-game checklist

Gameplay testing is pending. Run this later, when the current FFX test has
finished and the owner has installed the intended test build. This work did not
launch, attach to, control or deploy either game. Use the normal main story,
English game text and the existing save's normal progression. Last Mission,
minigames, warps and save edits are outside this checklist.

Expected speech below is the exact queued text. A screen reader may pronounce
punctuation differently. Baseline: Speech On, Speak punctuation names Off,
Menu indexing Off. Preserve existing preferences: if punctuation was already On,
reverse the two toggle expectations and restore On afterward. If saved Menu
indexing is On, the F8 rows additionally end with `, N of T`: main-page rows
1–3 of 3; Speech-options rows 1–2 of 2. Game top-menu speech has no such suffix.

Record each item's result, action, actual words and approximate delay to the
first usable word. Report missed/repeated/stale words and native input delays.
Do not infer a pass from a log or a normal game sound.

1. Phase 2a, startup. Start the intended FFX-2 test build normally. Expected
   speech once: `FFX-2 screen reader loaded. Only the main menu is read so far. F8 opens speech settings.`

2. Press F11 before any other mod speech. Expected speech: exactly the same
   startup line from item 1. A held F11 must not create uncontrolled repeats.

3. Press and release F8. Expected speech:
   `Accessibility mod menu. Speech: On`. Native game selection must not move
   while the mod menu owns its arrow/confirm/cancel keys.

4. Press Down once. Expected speech: `Speech options, submenu`.
   Press Enter. Expected speech:
   `Speech options. Speak punctuation names: Off`.

5. Press Right once on that option. Expected speech:
   `Speak punctuation names: On`. Press Left once to restore the baseline.
   Expected speech: `Speak punctuation names: Off`.

6. Press Down once. Expected speech: `Back`. Press Enter. Expected speech:
   `Accessibility mod menu. Speech options, submenu`.
   Press Down once. Expected speech: `Close mod menu`.
   Press Enter. Expected speech: `Mod menu closed`.

7. Press F8 again, then F8 again. Expected speech, in order:
   `Accessibility mod menu. Speech: On`, then `Mod menu closed`.
   Release all keys. The game must regain normal input immediately.

8. Use the game's configured menu, movement, Confirm and Cancel keys with F8
   closed. Expected mod speech: none in unsupported field screens; top-menu
   movement follows items 9–16. The original game actions must still occur.
   Repeat one hold/release and one quick press/release; no stuck key is acceptable.

9. Phase 2b, top-menu entry. Open the normal field menu, then move onto Items.
   Expected speech: `Items. Use items.` Opening the list without moving is not
   itself an announcement trigger in this build.

10. If White Magic is normally available, move onto it. Expected speech:
    `White Magic. Access recovery magic using the White Mage dressphere.`
    Its availability depends on the native party/equipment state. Do not change
    story progress solely to expose it.

11. If Equip is present, move onto it. Expected speech: `Equip. Change equipment.`

12. Move onto Garment Grids when normally available. Expected speech:
    `Garment Grids. Customize Garment Grids.`

13. Move onto Abilities when normally available. Expected speech:
    `Abilities. View current abilities and select new abilities to learn.`

14. If Accessories is present, move onto it. Expected speech:
    `Accessories. View accessories.`

15. Move onto Dresspheres when normally available. Expected speech:
    `Dresspheres. View dresspheres.`

16. Move onto Config. Expected speech:
    `Config. Configure gameplay settings.`
    These help lines come from the English archive records, not guessed wording.

17. Charset/reload check. Return to Items, then Config, then Items again.
    Expected speech in order: `Items. Use items.`,
    `Config. Configure gameplay settings.`, `Items. Use items.`
    The second Items line must not be suppressed as a duplicate. Leave the
    menu normally, change maps through normal play when convenient, reopen the
    menu and repeat. Expected speech is the same three lines. Labels alone
    indicate missing/unreadable charset/help data and do not pass the help check.

18. Idle and repeat. Move onto Config, then stop without further cursor moves.
    Expected speech once: `Config. Configure gameplay settings.` After that,
    expected new speech while idle: none.
    Press F11. Expected speech: `Config. Configure gameplay settings.`
    Move away and back. The same line must be spoken again for that new move.

19. Rapid actions. Move between available top rows quickly, then stop on Items.
    Expected final usable speech: `Items. Use items.` There must be no stuck
    native input or later speech from an old map/menu generation. Check once
    after a cold startup and again after repeated warm menu use. Also check a
    normally busy area when normal story progression reaches one. Record the
    observed first-word delay; multi-second assembly is a failure. Hardware
    latency budgets still need measured gameplay/backend evidence.

20. Items parser coverage boundary. Enter Items and move between two native
    rows, then cancel normally. Expected new mod speech inside the submenu:
    none. This build has only offline item cursor/list/help parser coverage.
    Do not mark submenu reading as working because the top-level Items row spoke.

21. Abilities parser coverage boundary. Select a party member if the native
    menu requires one, enter Abilities, move a row and cancel normally.
    Expected new mod speech inside the submenu: none. Its addresses remain
    UNVERIFIED and have no live hook.

22. Garment Grid/Dressphere boundary. Enter each normally available submenu,
    move once and cancel without changing equipment. Expected new mod speech
    inside either submenu: none. Node and parent-list bindings have static
    evidence and offline tests; native thread/reveal gates remain UNVERIFIED.
    Do not test unavailable or story-locked entries by forcing them open.

23. Config boundary. Enter native Config, move between options and cancel
    without changing settings. Expected new mod speech inside native Config:
    none. Its nine main-story label/value records have offline selection tests.
    The 16-byte per-row arrays are animation state. Static evidence says the
    help window shows the selected value's help (for Subtitles ON,
    `Display subtitles.`); a sighted helper confirms that text once.

24. Battle-ready boundary. Reach an ordinary story battle through normal play.
    Expected character-ready mod speech in this build: none. No ready hook or
    sound is installed. After future address verification and integration, the
    acceptance strings are exactly `Yuna`, `Rikku` or `Paine` for the displayed
    ready character, once per ready edge, with no sound or suffix. A held ready
    state and a mere party-slot reorder must remain silent. This future behavior
    is tested only by the worker fixture today; do not mark it gameplay-confirmed.

25. Final handback. After the menu and battle checks, open and close F8 once.
    Expected speech: `Accessibility mod menu. Speech: On`, then
    `Mod menu closed`. Native game keys must continue working after release.

## Experimental Config build (`FFX2_EXPERIMENTAL_CONFIG=1`)

Run these only with a test build compiled with that flag; the ordinary build
must keep item 23 (no Config speech). Build it by adding
`/DFFX2_EXPERIMENTAL_CONFIG=1` to the DLL compile line. Use the main story, English text, and
restore every setting you change before leaving Config. Speech interrupts the
previous menu line. Entering Config speaks nothing until the first move; that
is a known gap, not a failure.

26. Config row moves. Open Config from the top menu (top-menu speech
    `Config. Configure gameplay settings.`, as item 16). Press Down one row
    at a time from Spherechanges to Guide Map, then Up back to the top. Each
    move speaks exactly `Label: Value. Help` for the value shown on screen.
    Rows in order, with every value and its exact help:
    - Spherechanges: `Normal. Display full spherechange sequences.`,
      `Short. Display shortened spherechange sequences.`, `OFF. Skip spherechange sequences.`
    - ATB Mode: `Active. Set Active Time Battle mode to Active.`,
      `Wait. Set Active Time Battle mode to Wait.`
    - ATB Speed: `Slow. Set Active Time Battle gauge to charge slowly.`,
      `Normal. Set Active Time Battle gauge to charge normally.`,
      `Fast. Set Active Time Battle gauge to charge quickly.`
    - Cursor: `Memory. Remember previous cursor position in battle menus.`,
      `Default. Keep default cursor position in battle menus.`
    - Battle Help: `ON. Display help window during battle.`, `OFF. Hide help window during battle.`
    - Vibration: `ON. Turn vibration function on.`, `OFF. Turn vibration function off.`
    - Subtitles: `ON. Display subtitles.`, `OFF. Hide subtitles.`
    - Subtitle Names: `ON. Display speaker’s name in subtitles.`,
      `OFF. Hide speaker’s name in subtitles.`
    - Guide Map: `ON. Display on-screen map.`, `OFF. Hide on-screen map.`
    Example with Subtitles ON: `Subtitles: ON. Display subtitles.` Record the
    first row's value. The value word must match the one on screen.

27. Config value changes. On Subtitles, press Right once. Expected:
    `OFF. Hide subtitles.` Press Left once. Expected: `ON. Display subtitles.`
    On Spherechanges press Right twice: `Short. Display shortened spherechange sequences.`,
    then `OFF. Skip spherechange sequences.`; press Left twice back to Normal.
    Pressing Left at the first value or Right at the last value must stay
    silent (the game does not change the value).

28. Config idle and edges. Hold no key for five seconds: no speech. Press Up
    on Spherechanges and Down on Guide Map: silent (cursor does not move).
    Cancel out of Config without changing settings and return to the top
    menu. Cancel and confirm themselves are silent in this reader. Top-menu
    speech resumes on the next top-menu move, with no Config line repeated.

29. Config help mirrors the screen. For three rows, a sighted helper compares
    the spoken help with the help window. Any mismatch (for example the help
    still showing the previous row) is a failure to report with the row name.
    Report whether the game shows help for the label row itself.

Offline limits and static instruction evidence:
[addresses](ffx2-addresses.md). Battle design and missing integration evidence:
[ready cue](ffx2-battle-ready.md). The code owner resolves pointers, callbacks,
thread ownership and remaining bindings; gameplay feedback requires no code review.

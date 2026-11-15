# FFX-2 address evidence

This file records which FFX function addresses have a verified FFX-2.exe
equivalent, and why. `profileRva()` in `src/native/profile.hpp` reads the
`ffx2Addresses` table under `NativeProfile::Ffx2`. That table holds only the
verified pairs below. Every other FFX RVA throws `Unverified FFX-2 RVA`.

## Binaries

- FFX-2.exe: Steam, 12281600 bytes, SHA-256
  6ea7f142f129a43bdaea7eaabda4477a9057e60d12fdae367530c63e9a14cd69.
- FFX.exe: the installed Steam build (`NativeProfile::Installed`, SHA-256
  starting 0537b2a1). The mod's hook ids are reference RVAs. The installed
  RVA comes from `nativeAddresses` or from the signature extensions in
  `profile_extensions_data.hpp`.

## Method

The scope report (`.local/audit/ffx2-scope.md`) found candidates by matching
the first bytes of each function. A prologue match is not proof, so each
candidate was checked again with a whole-function comparison:

1. Load both executables at their preferred bases and apply relocations.
2. Disassemble the installed FFX function from its entry to its last `ret`
   with Capstone (x86, 32-bit).
3. Mask every operand that the relocation table marks as an absolute
   address, and the rel32 of every `call` and `jmp`.
4. Search the whole FFX-2 `.text` section for the masked byte pattern.
5. Accept only a single match. Then check that every masked global and
   every call target maps to a consistent FFX-2 counterpart, and compare the
   callers and the vtable slots that reference the function.

The ESC, Blitzball and other reader addresses were not checked in this phase.
Ghidra was not running during this work, so the comparison used the PE
images directly. The Ghidra FFX2 project can confirm the same facts later.

## Verified

### 0x225930: PInput::Update (game-thread probe)

- Installed FFX: 0x225770 (signature extension `signature_225930`, owner
  0x22e4f0, installed 0x22e340).
- FFX-2: 0x6b50a0.
- Body: 103 bytes, single masked match in FFX-2 `.text`.
- First 16 bytes. FFX: 80 3d d0 9c cc 00 00 74 27 a1 d4 9c cc 00 6a 01.
  FFX-2: 80 3d a0 31 a6 01 00 74 27 a1 a4 31 a6 01 6a 01. Only the
  relocated globals differ.
- Globals: 0x8c9cd0 maps to 0x16631a0, 0x8c9cd4 to 0x16631a4, 0x830fa8 to
  0x9b25d0. Call 0x224d40 maps to 0x6b4670.
- Callers: FFX 0x22e340 and 0x241800; FFX-2 0x6bdc20 and 0x52810.
- The owner function also matches as a whole: installed 0x22e340 and FFX-2
  0x6bdc20, 86 bytes, single match. It calls the probe first in both games.

### 0x22d770: DirectInput keyboard updater

- Installed FFX: 0x22d5c0. FFX-2: 0x6bcea0.
- Body: 279 bytes, single masked match.
- Calls: 0x54964c maps to 0x47dc24 (four calls), 0x54923a to 0x47d890,
  0x22d550 to 0x6bce30.
- Globals: 0x8613d8 maps to 0x96fab8 (security cookie), 0x8c9cd8 to
  0x16631a8, 0x830e90 to 0x9b24b8.
- Only reference: a vtable slot, FFX 0x73fc44 and FFX-2 0x88a5fc. The
  neighbouring slots also match as whole functions: 0x225ca0 to 0x6b55d0,
  0x2289a0 to 0x6b8280, 0x221ef0 to 0x6b1850.
- The hook reads the DirectInput device at `[ecx+0x14]`. That offset is
  inside the matched body, so it is the same in FFX-2.

### 0x22db80: DirectInput pad updater

- Installed FFX: 0x22d9d0. FFX-2: 0x6bd2b0.
- Body: 450 bytes, single masked match.
- Vtable slot: FFX 0x73ff94, FFX-2 0x88a94c. Neighbouring slots match:
  0x225dd0 to 0x6b5700, 0x2289f0 to 0x6b82d0, 0x228ad0 to 0x6b83b0.
- Calls: 0x228170 maps to 0x6b7a80, 0x352f0 to 0x48cb70.
- Same `[ecx+0x14]` device read as the keyboard updater.

### 0x471d10: per-frame input event

- Installed FFX: 0x471d70. FFX-2: 0x32ce30.
- Body: 211 bytes, single masked match.
- Call at +0xe: 0x487dc0 maps to 0x347290. Globals: 0x48e600 maps to
  0x34c130, 0xf270c0 to 0xf270d4 map to 0xd93a90 to 0xd93aa4.
- One caller in each game: FFX 0x4688fe, FFX-2 0x32304e.
- Verified and in the table, but not hooked yet. The FFX handler on this
  address reads battle and scene state that FFX-2 does not have verified.

## Verified by call graph

FFX-2 grew these two functions, so a whole masked body cannot match. They are
verified by a second method instead:

1. The prologue matches up to the cookie global and argument registers.
2. A log string or flag write ties the body to the FFX function.
3. The callers match two levels up. Each caller pair has identical masked
   bodies, and each function has the same number of callers in each game.

The mod hooks these two only as resource scopes: on entry it starts a new
speech generation, and while the call runs it suppresses other readers. It
reads no argument or structure, so the changed bodies do not matter to it.

### 0x5093b0: map load (load-finished boundary)

- Installed FFX: 0x5093b0. FFX-2: 0x3ba3b0.
- Prologue: `push ebp; mov ebp,esp; sub esp,0x34`, then the stack cookie.
  Only the cookie global (0xc613d8 in FFX, 0xd6fab8 in FFX-2) and the
  argument registers differ.
- Strings: FFX refers to "RAM Inuse : %d u:%d" at 0x50947f. FFX-2 refers to
  "RAM Inuse : %d" at 0x3ba4a2, and also to "FFX2 Loading: loading map %s"
  and "load yn guidemap".
- Tail: both set bits 0x11 in a status global (FFX 0x18ded08, FFX-2
  0x172508c) and call the same-shaped finisher (FFX 0x514250, FFX-2
  0x3bfdb0).
- Body: FFX 217 instructions, FFX-2 256. Instruction similarity 0.66.
- Callers: FFX 0x508370 and 0x5085d0 only; FFX-2 0x3b9060 and 0x3b93d0
  only. Both pairs have identical masked bodies (15 instructions).
- Their callers: FFX 0x46dca0, 0x46dcd0, 0x46dd00, 0x46dd30; FFX-2 0x328670,
  0x3286a0, 0x3286d0, 0x328700. All four pairs have identical masked bodies,
  and each pair calls the same wrapper.

### 0x46fed0: map request (event map jump)

- Installed FFX: 0x46ff40. FFX-2: 0x32a970.
- Prologue: both test bit 0x80 of a flag byte (FFX 0x1326b28, FFX-2
  0x11924d4), call a gate and stop if it returns 0. Both then push
  `[ebp+0xc]` and `[ebp+8]` and call the map jump (FFX 0x48ea60, FFX-2
  0x34c5b0).
- Body: FFX 195 bytes, FFX-2 237. Instruction similarity 0.72. FFX-2 adds a
  log line "svp->LastEventJumpId: %d" and a branch on `[ebp+8]` against
  0x164. FFX compares the word at `[eax+2]` with 0x109; FFX-2 compares it with
  0x17. These are game-specific map numbers.
- Callers: FFX 0x4581c0, 0x458330, 0x458460, 0x458610 (event opcode
  handlers). FFX-2 0x314050, 0x3141b0, 0x314350, 0x314560, and a wrapper
  0x32a950 that FFX-2 adds. Three handler pairs have identical masked bodies.
  The 0x458460 and 0x314350 pair differs (similarity 0.42).

## Not verified

These scope-report candidates are related functions, not the same function.
They stay out of the table.

- 0x4b6260 (installed 0x4b62b0) and FFX-2 0x392490. No masked match.
  Similarity 0.69. Structure offsets differ (+0xc becomes +0x150, +2 becomes
  +0x148) and one FFX block is missing. This is numeric entry, not the ordinary
  dialogue/choice producer; see "Dialogue and field text" below.
- 0x4e2e20 and FFX-2 0x389f20. No masked match, but both are 277 bytes and
  89 instructions with the same control flow. Only structure field offsets
  move: +0x48 to +0x4c, +0x45 to +0x48, +0x58 to +0x5c, +0x90 to +0x94. It is
  probably the same function on a changed structure. The FFX handlers read
  the old offsets, so the address is not usable until those reads are ported.
- 0x376220 (help-page input). No FFX-2 match. FFX-2 does not hook it.
- 0x48dfe0 (installed 0x48e070, field initialize, the third FFX resource
  scope). No masked prefix or whole-body match in this investigation. A second
  instruction-shape search of the field cluster did not establish a match.
  Under FFX-2 `resourceScopeHook()` still returns false for it.

## Text encoding

`src/native/ffx2_text.cpp` decodes FFX-2 game text to UTF-8. It follows the
English path of the game's own converter. Absolute operands below are virtual
addresses at the preferred base 0x400000; function addresses are RVAs.

- 0x111120 loads `%s/ffx2sjistbl_us.bin` (string at VA 0xc34214) from the
  game archive. For English it stores the buffer in the global at VA
  0xdeade0. Japanese, Korean and Chinese use the global at VA 0xdeaddc.
- The table is UTF-8 text. Entry i is its i-th code point. The lookup at
  0x1109f0 turns a character index into a byte offset with fixed ranges,
  because some entries are wider than one byte.
- The converter at 0x110760 dispatches bytes below 0x30 through a byte table
  at VA 0x510860:
  - 0x00 ends the string.
  - 0x01, 0x02 and 0x05 are one-byte controls with no output.
  - 0x03 writes CR LF. The decoder writes "\n".
  - 0x04 is an escape to the Japanese table. It is three bytes when the next
    byte is 0x2f or less, else two. The decoder skips it.
  - 0x06 to 0x2b take one parameter byte and produce no output.
  - 0x2c to 0x2f select a page: index = (byte - 0x2b) * 0xd0 + next - 0x30.
  - 0x30 and above: index = byte - 0x30.
- The runtime reads the table from the game through the pointer at VA
  0xdeade0 (FFX-2-only address `Charset`, see below). The decoder and its
  fixtures use synthetic bytes only.
- Not handled: the meaning of the two-byte controls (FFX uses them for
  button glyphs, colours and name macros). The decoder drops them.

## Menus

FFX-2 has two kinds of menu:

- Iggy (Flash) overlays, as in FFX: `flash/escmenu.swf` (PC settings, with
  the Iggy `EscMenu*` callbacks), `flash/pausemenu.swf`, the save and load
  screens (`saveloadx2_*.swf`, `saveloadlm_*.swf`) and the speed and
  auto-battle toasts.
- The native field menu (Items, Dresspheres, Garment Grid and so on). It
  uses the same module manager and window objects as FFX, with changed
  structures. No FFX menu function matches whole (0x4e0340, 0x4b4460,
  0x4a9ab0, 0x38fd40 and 0x38fc00 have no masked match).

All addresses below come from static analysis. The ones the runtime uses are
in the FFX-2-only table (next section).

### Module manager

- FFX 0x4aaab0 registers a menu module: it clamps the slot to 0..0x18,
  stores the module in the table at VA 0x1840888 and calls the module's
  `+8` init. FFX-2 has the same code at 0x3657b0 (slot 0..0x14, table at VA
  0x16bc590), but FFX-2 inlines it in the dispatcher 0x365280.
- A module is a callback table: `+0` update, `+4` draw, `+8` init, `+0xc`,
  `+0x10` exit, `+0x14` release.
- FFX registers the main menu (table at VA 0xc5b2b0, update 0x4e0350, the
  installed address of the hooked 0x4e0340) in slot 1. FFX-2 registers the
  module at VA 0xd62e20 in slot 2, on the dispatcher's default branch.

### Top-level menu (module VA 0xd62e20)

- Callbacks: update 0x373170, draw 0x373650 (a bare `ret`), init 0x373660,
  0x373690, exit 0x373710, release 0x373730.
- 0x375900 builds the item list. It sets availability bits for item ids 0
  to 11, then copies each available item to the array at VA 0x16befe0 (8
  bytes each: dword id, word available flag at `+4`, word at `+6`). It
  stores the count at VA 0x16bf040 and resets the cursor at VA 0x16bef0c to
  0.
- 0x373170, on up (input bit 0x1000) and down (0x4000): plays the cursor
  sound (0x344710 with 0x80000001), moves the cursor at VA 0x16bef0c with
  wrap-around, then sets the help line from `0x379050(id)`.
- 0x379050 maps the id to a menu-text id (0x1047 + id; ids 7, 8 and 9 have
  special cases) and calls the text getter 0x21d7c0 with space 1.
- The item labels are not text. The draw code at 0x3760d9 uses `id * 18`
  as a row in a 0x70 by 0x12 texture. So there is no game string for an
  item name, only for its help line.

### Item names

The labels are textures, so the item name comes from a fixed English list in
the mod (`src/native/ffx2_top_menu.cpp`, one table indexed by menu id). The
names are the game's own text, not guesses: the help setter 0x379050 maps
each id to a record in `battle/kernel/menu_txt.bin`, and each record holds a
name string and a help string. With the flag at VA 0x16ef654 equal to zero:

- ids 0 to 6: records 0x1047 to 0x104d: Items, White Magic, Equip, Garment
  Grids, Abilities, Accessories, Dresspheres.
- id 7: record 0x104f, Develop.
- id 8: record 0x1050, Team Members.
- id 9: record 0x104e, Config.

When the flag is not zero, 0x379050 maps id 7 to Config and id 8 to Develop.
0x395620 sets the flag from a mode switch: case 0 clears it and the
companion flag at VA 0x16ef6a0, every other case sets both. Zero is taken to
be the main story (inference, to confirm in game). The reader speaks only
when the flag is zero and logs the value once otherwise.

The record table (`menu_txt.bin`): ranges of record ids at `+0xc` (20 bytes
each), 8-byte records from `+0x20` (word name offset, word, word help offset,
word), strings at `+0x20 + [+0x18]`. 0x21d7c0 (space, id) finds the text
space at VA 0xe103b0 + 12 * space, 0x21e3a0 finds the range, and the
getter returns base + word `[record + 0]` (name) or `[record + 4]` (help),
chosen by the space through the jump table at VA 0x61d828: space 0 takes
the name, spaces 1, 3 and 4 take the help. 0x379050 asks for space 1, so the
pointer stored at VA 0x16bd97c is the record's help string. Decoded offline,
record 0x1047 holds "Items" and "Use items.". The reader speaks the help
through that pointer, not by record number.

## FFX-2-only addresses

`ffx2OnlyAddresses` in `src/native/profile.hpp` holds FFX-2-specific RVAs and
layouts, including experimental text sites that cannot use FFX handlers.
`ffx2Rva(name)` throws outside the FFX-2 profile. Hook ids
for these functions are `0xfe000000 | rva`, outside the FFX RVA space that
handlers and resource scopes compare against. Each entry was checked by
reading every reference to it in FFX-2.exe.

- `TopMenuUpdate`, 0x373170: update callback at `+0` of the module at VA
  0xd62e20, which the dispatcher 0x365280 registers in slot 2 (table at VA
  0x16bc590). Prologue `55 8b ec 51 53 e8` (push ebp; mov ebp, esp; push
  ecx; push ebx; call 0x37b250); the reader refuses to hook if these bytes
  differ. A jump table at VA 0x7735e4 dispatches on the module state at
  `[module + 0x1c]`; state 5 is the top-level list.
- `TopMenuCursor`, RVA 0x12bef0c (VA 0x16bef0c): dword. Written only by
  0x373170 (up and down, with wrap) and by 0x375900 (reset to 0). Read by
  the draw code (0x375c50, 0x375cc0, 0x376020), which highlights the row
  whose index equals it.
- `TopMenuItems`, RVA 0x12befe0 (VA 0x16befe0): 8-byte entries, dword id
  at `+0`, word available at `+4` (confirm buzzes when zero), word at `+6`
  (non-zero: the item asks for a party member). Built by 0x375900.
- `TopMenuCount`, RVA 0x12bf040 (VA 0x16bf040): dword count, the wrap
  bound in 0x373170.
- `HelpText`, RVA 0x12bd97c (VA 0x16bd97c): the help setter 0x365a50 stores
  its argument here when it differs from the current value (previous value
  at VA 0x16bd980), then redraws the help window via 0x35fcd0(0x1a).
  0x373170 calls it right after each cursor move, so the after-hook sees
  the new line.
- `Charset`, RVA 0x9eade0 (VA 0xdeade0): written only by the loader
  0x111120, for every language except Japanese, Korean and Chinese (its
  second switch at 0x1112b0 sends those to VA 0xdeaddc). The loader sizes
  the buffer as the file size rounded up to 16 bytes (`add eax, 0xf; and
  esi, 0xfffffff0`), allocates that plus one (0x34c180), reads the file and
  writes a NUL at the end. The previous buffer is freed (0x34b3e0) before
  the store. All 56 other references are reads in the lookup 0x1109f0.
  `parseLoadedCharset` stops at the NUL or at the first malformed padding
  byte.
- `StoryMode`, RVA 0x12ef654 (VA 0x16ef654): dword returned by 0x392ee0;
  written only by the mode switch 0x395620.

### In game, not yet confirmed

- That 0x373170 runs on the game thread that the 0x225930 probe marks
  (the hook dispatcher drops calls from other threads).
- That the flag at VA 0x16ef654 is zero in the main story.
- The text of each help line on screen.

## What runs under FFX-2

- MinHook starts.
- Window message hooks (PeekMessageA/W, GetMessageA/W) for key capture.
- 0x225930 probe: marks the game thread on entry, drains queued commands on
  return.
- 0x22d770 and 0x22db80: DirectInput keyboard capture. The keyboard is
  swallowed while the mod menu is open. The pad is only neutralized while a
  mod input dialog has focus, because the controller owner is not installed.
- Hotkeys: F8 opens a speech-settings mod menu (Speech, Speech options with
  punctuation names, Close). F11 repeats the last speech.
- 0x46fed0 and 0x5093b0: resource scopes. Each call logs "native reader
  resource scope begin" and "end" and invalidates pending speech. The return
  logs "native load return".
- 0x373170 (FFX-2-only): the top-level menu. On each cursor move it speaks
  "<item name>. <help line>" and logs "FFX-2 top menu cursor=<n> id=<id>".
- No other reader, menu, battle, field, controller, beacon or minigame hook. The
  input code skips the ESC menu, Blitzball, field, battle-sensor and minigame
  predicates under FFX-2, because they read FFX memory.

## Dialogue and field text

November 15, 2026. **Static only; every X-2 item in this section is unverified
in game.** No game launch, debugger attachment, deployment or game-directory
write took place. The executable hashes are the ones above; the full FFX hash
is `0537b2a1047f3266e73495cd4e35f63f0777f4231d417699f979954686da686d`.
Executable copies, disassembly and analysis scripts remain outside Git in the
task scratch directory `ffx2-text`. Ghidra had no running instance; analysis used
PE relocations and Capstone 5.0.7 on the copies.

### FFX capture path and search results

Addresses below are RVAs, not virtual addresses. "FFX" means the reference hook
id, followed by the installed Steam RVA where established. Confidence describes
static function identity or explicitly stated functional correspondence. It
does not establish thread affinity, live visibility, speech or voice policy.

- **Text setter: FFX 0x4b8980, installed 0x4b89d0; X-2 0x396fc0.
  High static confidence.** Three cdecl stack arguments: slot, primary text,
  alternate text. FFX starts `55 8b ec 81 ec 20 04 00 00`; X-2 starts
  `55 8b ec 83 3d`. There is no whole masked-body match. Both select the
  alternate-language pointer, write two start/current pointer pairs, then
  select a pair into the displayed start/current pair. FFX offsets
  `+0x5c/+0x60`, `+0x74/+0x78`, `+0x90/+0x94` become X-2
  `+0xbc/+0xc0`, `+0xd4/+0xd8`, `+0xf0/+0xf4`. X-2 calls 0x3962e0 to
  measure this text. Its six direct callsites are 0x314dd2, 0x35e7cd,
  0x35e820, 0x35e880, 0x35e8e0 and 0x35e940. In the wrapper at 0x35e7b0,
  the same pointer is pushed twice before the slot, exactly as the FFX wrapper
  does at 0x49b196..0x49b19c. The script-event caller also calls the X-2 window
  opener at 0x314e79. The experiment observes this setter only to invalidate
  its previous page identity; it does not speak text while the window is hidden.

- **Page changes: FFX 0x4de350/0x4de5b0, installed
  0x4de370/0x4de5d0; X-2 0x397a40 plus 0x39b9f0.
  High confidence in the page-advance mechanism; no one-to-one body match.**
  FFX signatures begin `55 8b ec 83 ec 0c d9 ee 56 57` and
  `55 8b ec 83 ec 0c d9 ee 53 56`. Both advance their current text pointer
  from the previous page end, then scan the next page. X-2 separates advance
  from preparation. 0x397a40 begins `55 8b ec 57 8b 7d 08`: slot is its
  sole cdecl argument. At 0x397a7f..0x397aa3 it loads `window+0xf8`, adds one,
  writes `window+0xf4`, calls 0x394330, and stores the new end at `+0xf8`.
  The primary and alternate pairs advance similarly. Its two direct callers
  are 0x390fd2 and 0x3911f2. At 0x397b1e it tail-calls the window's `+0x128`
  callback with the window pointer as a cdecl stack argument. 0x397090 and
  0x397920 install 0x39b9f0 into that slot. 0x39b9f0 begins
  `55 8b ec 81 ec 14 01 00 00`; at 0x39ba19 it reads `+0xf4`, computes
  current-page measurements/reveal bounds, and chooses the input callback.
  The experiment copies the page on this preparation return, not from the
  raw renderer or from the next-page scanner.

- **Ordinary choice input: FFX 0x4b6490, installed 0x4b64e0;
  X-2 0x3926e0. High static confidence.** FFX starts
  `55 8b ec 8b 4d 08 66 a1`; X-2 starts
  `55 8b ec 8b 55 08 66 a1`. Both accept a slot, save/restore the default
  focus on cancel, accept a row on confirm, and move the focus on directional
  bits before sounding the cursor. X-2 uses a 32-byte directional-link row
  structure at `window+0x180`, rather than FFX's simpler wrap path. At
  0x39bc7d the current-page preparer installs this exact function into
  `window+0x130` when `+0x147==1`. The callback is indirect, so a search for
  direct calls alone finds none. It is cdecl, with slot at `[ebp+8]` and a
  plain `ret`/tail jump. The hook reads the focused row after native processing;
  it does not emulate keys or compute selection from physical input.

- **Numeric input: FFX 0x4b6260, installed 0x4b62b0;
  X-2 0x392490. Medium static identity confidence; not installed.** The
  matching masked prefix includes `55 8b ec 83 ec 14`, the cookie and local
  zeroing. No whole-body match. Both decompose the numeric value with a
  decimal-power table and handle digit movement/value edits. X-2 uses
  `+0x148` for the digit and `+0x150` for the value, replacing FFX's `+2`
  and `+0xc`. X-2 0x396d60 installs it at `+0x130`, with a pointer operand
  at 0x396dfc; 0x39b9f0 selects it for kind 2 at 0x39bc63. This corrects the
  scope report's description of 0x392490 as a general dialogue window.

- **Choice drawing: FFX 0x4b5f60, installed 0x4b5fb0;
  no exact X-2 match. Confidence unknown.** FFX prefix
  `55 8b ec 83 ec 38` leads into reset cursor-draw globals and separate TK
  cursor objects. Masked prefix/body searches fail. X-2's larger draw loop
  at 0x391c20 tests each active window and `+0x144==1`, but is not established
  as the same function. The experiment uses the ordinary native input
  callback instead; initial focus is observed on its first visible call.

- **Review/lifetime reset: FFX 0x4b8510, installed 0x4b8560;
  no verified X-2 identity. Confidence unknown.** FFX prefix
  `55 8b ec 56 8b 75 08 b9 01 00 00 00`; it writes the native state and
  invokes callbacks. `messages.cpp` calls its observer `closed`; that name
  alone does not identify an X-2 operation. The experiment instead observes
  the independently traced X-2 opener 0x396280, beginning
  `55 8b ec 56 8b 75 08`. It sets active byte `+0` to 1, state `+4` to 0,
  calls 0x390490 and 0x38e130, and is called after text installation in
  0x35e7b0 and the event handler. FFX's analogous wrapper opens through
  0x4ade10, not the 0x4b8510 hook. This is a new X-2 lifetime observation,
  not a claimed RVA substitution for the FFX review hook.

- **Event end: FFX 0x471a50, installed 0x471ab0;
  no verified X-2 match. Confidence unknown.** FFX prefix
  `55 8b ec 51 53 56 57 8b 3d`; no masked prefix/body match. A second
  instruction-shape search of X-2's field cluster suggested 0x32ca40 at only
  0.33 similarity; that is insufficient evidence. No X-2 dialogue-review
  lifetime or T-key review implementation is inferred from it.

- **Decoded event/tutorial output: FFX 0x376a50, installed 0x376940;
  no verified X-2 match. Confidence unknown.** FFX prefix
  `55 8b ec 83 ec 7c`; caller RVAs 0x36f618 and 0x36fcbd.
  `subtitleAfter` observes the output buffer in argument 0, while the Help
  observer associates it with the selected SPS2 entry. Masked prefix search
  fails. Instruction-shape searches in the X-2 menu/script region did not
  establish a candidate. Native window messages can include tutorial text,
  but this does not cover dedicated tutorial pages or event subtitles.

`messages.cpp` installs all nine direct sites above: one subtitle site, one text
setter, two page sites, three TK sites, one review reset and one event-end site.
Its FFX-only helpers must not be installed wholesale under the X-2 profile.

### Decoder, location names and remaining text hooks

- FFX's `text.cpp` / `owned_text_decoder.hpp` use a built-in glyph table.
  `message_text.cpp` first walks the current native page until 00/01/02,
  resolves name/value controls through FFX 0x4b6610 (installed 0x4b6660),
  resolves button labels, and sends owned instructions to the worker. It
  reads window pointers at reference RVA 0x14676b0, separate TK cursors at
  0x1468a50, window current text at `+0x94`, and cursor visible/row/count at
  `+1/+2/+5`. These offsets cannot be used in X-2.
- X-2's macro getter candidate is **0x399fd0, medium static confidence**,
  compared with FFX 0x4b6610. Both accept `(slot, token)` and dispatch on
  the token byte minus 0x10 and parameter minus 0x30. FFX starts
  `55 8b ec 51 53 56 8b 75 0c`; X-2 starts
  `55 8b ec 83 ec 0c 53 8b 5d 0c`. X-2 adds controls and uses different
  backing tables. Calls include 0x39a54f, 0x39a5cb, 0x39bc25 and 0x39bd2e.
  The return can point into a reused formatting buffer at VA 0x16ef6b0.
  The experiment neither calls this getter nor retains its return pointer;
  resolved macro capture remains unimplemented.
- **Location getter: FFX 0x46c3c0, installed 0x46c430;
  X-2 0x326b20. High static confidence.** A unique whole masked match:
  60 bytes, 26 instructions. Prefix `55 8b ec 56 57 e8 ?? ?? ?? ?? ff 75 0c`.
  It takes `(index, text-bank)`, calls the language selector (FFX 0x3851f0,
  X-2 0x20c680), bounds the index by the table's first word shifted right
  two, reads a language-selected 16-bit offset and returns table+offset.
  The bank lookup changes from FFX 0x4b7f60 to X-2 0x3944b0; X-2 reads
  `[VA 0x16d6200 + bank*4]+0x2c`. Callers include 0x36a16e, 0x36b184,
  0x39efbc and 0x3a04ec. This getter is not a display event. No automatic
  location announcement is added: the field-initialization/arrival identity
  and area-selection structures remain unmatched.
- X-2 0x394330 scans to the next 00/01/02 boundary, using the token-width
  tables at VA 0x794388/0x794398. 0x397dd0 advances one token, using
  VA 0x797e10/0x797e20. 0x39a7b0 interprets window text and its row markers.
  Its byte dispatch is at VA 0x79b244, jump table VA 0x79b230; category
  dispatch VA 0x79b26c. The 0x10 branch at 0x39ae50..0x39afe7 subtracts
  0x30 from the parameter, handles 0xff as a row end, stores row geometry
  at `+0x180 + row*32`, and enables cursor byte `+0x146`.
- The existing X-2 charset/converter evidence above remains authoritative.
  The field decoder adds page termination and row separation; unlike menu
  decoding, it refuses incomplete or unsupported text. It accepts glyphs,
  03 line breaks, 05, and verified presentation controls 09/0c/0e with one
  parameter. Native 0x39acac..0x39ae50 changes presentation for these controls
  without adding text. It refuses Japanese escapes, unresolved buttons,
  name/value macros and other unknown controls rather than deleting them.

No X-2 counterpart is established for the remaining FFX Help hooks:
0x375df0, 0x375ef0 (initialization), 0x376220 (input), 0x36f960 (draw scope),
0x4e52e0, 0x4e75c0, 0x4e52b0, 0x4ee210 (accepted text draws), and 0x374630
(control diagrams). Existing signatures did not establish an X-2 match;
separate tutorial state, SPS2 layout and image labels remain unverified.

FMV captions are a separate FFX path: 0x2dad80 selects a timeline row and
0x505d30 observes its selected text. No X-2 matches are established here.
The supporting FFX movie sites (0x6273b0, 0x548cf4, 0x628330, 0x627e90,
0x67c90a, 0x628350, 0x2d9db0, 0x2d72e0, 0x2d90f0, 0x2d77b0) and dialogue
voice sites (0x30b6b0, 0x30b250, 0x548c9a, 0x548ca6, 0x30aec0, 0x30a680,
0x307b40, 0x30b8e0, 0x30b420, 0x30b3a0, 0x30b460) are inventoried, not
ported or individually matched in this task. Confidence in their X-2 mapping
is unknown. In particular, field text is not proof of opening-movie subtitles
or of audible native voice. Do not install the FFX voice readers in X-2.

### X-2 layout and experimental implementation

All offsets below are independently visible in X-2 instructions, not copied
from FFX. The initializer 0x3968f0 points each of eight slots at one window;
the pointers are at VA 0x1cceb20 (RVA 0x18ceb20). The initializer
0x395790 clears exactly 0x5a0 bytes per window.

- `+0`: active byte; set to 1 by 0x396280, tested by the draw loop 0x391c20.
- `+1`: signed byte slot, used by 0x39b9f0 and the font calls.
- `+4`: dword window lifecycle state; reset by the opener. It is not FFX's
  state word at `+0x2c`.
- `+0xf0/+0xf4/+0xf8`: selected text start/current/end.
- `+0x128`: current-page preparation callback; 0x397a40 calls it after advance.
- `+0x130`: cursor input callback, selected by the page preparer.
- `+0x144`: word cursor state. The draw loop tests equality with 1; confirm
  in 0x3926e0 changes it to 2. It is not FFX cursor byte `+1`.
- `+0x146/+0x147`: cursor enabled/kind bytes; kind 1 is ordinary choices,
  kind 2 numeric entry.
- `+0x148`: signed focused row/digit byte. The reader bounds it to 0..31.
- `+0x180 + row*32`: per-row native geometry/directional links.

`FFX2_EXPERIMENTAL_FIELD_TEXT` defaults to **0**. The ordinary build does not
register this speech domain or install these hooks. Defining it to 1 explicitly
enables only the four `Ffx2Only` sites: setter 0x396fc0, opener 0x396280,
page preparer 0x39b9f0 and ordinary-choice updater 0x3926e0. All signature
prefixes are checked before registration, in addition to executable/profile
validation. The short prefixes are sanity guards, not independently unique
function signatures. The general FFX-to-X-2 RVA whitelist is unchanged.

Native hooks run only on the hook framework's PInput owner. They copy an owned,
bounded current-page/charset snapshot, attempt the existing nonblocking speech
capture once, and return. They do not decode, format, log, invoke text getters,
wait or retry for workers. Workers never dereference native pointers. Native
identity/focus prevents idle repetition; an explicit new text/open lifetime
allows identical words to be spoken again. A rejected queue submission leaves
the identity available for a later observation. Generation changes reject old
packets. Per-slot atomic versions also retire captures superseded by a newer
page, focus, text/open lifetime or observed selector acceptance before speech
preparation. Worker-owned page identities preserve the prompt when the first
capture is overtaken by a rapid focus move. These guards do not retract audio
already submitted to the backend. Only story-mode flag 0 is accepted, as in the
existing menu reader.

Limits per observation: 0x14c window bytes, up to 4096 current-page bytes, and
up to 1024 charset bytes. A missing page terminator or incomplete charset copy
is refused. The parser never reads the page after 00/01/02. The worker speaks
the prompt and current native choice, not every choice or a guessed label.
The default-off and enabled synthetic adapter hosts cover copies surviving
complete source removal, idle silence, repeated equal-text lifetimes, selected
row changes, closed selector state, rejected queues, stale generations,
unsupported macros, inactive windows, invalid slots and unsupported mode.
Rapid-focus and same-generation lifetime/acceptance regressions also pass.

**Remaining limitations:** no FMV/event-subtitle path, dedicated tutorials,
numeric entry, location-arrival announcement, resolved names/values/buttons,
native-voice qualification or T-key dialogue review. The experiment treats its
accepted pages as unvoiced, so it can duplicate native voiced dialogue and
does not implement the saved voiced-dialogue policy. Keep it disabled for
distribution. F11 still repeats the last speech through the existing system.
Real callback affinity, first visible focus, reveal timing, pointer lifetimes,
NVDA speech, queue pressure and latency remain live-check requirements.
The ordinary text-window close/event-end path remains unmatched; immediate
cancellation at that boundary is not claimed by the experimental capture guard.
Worker diagnostics report capture duration and preparation age; they do not
prove time to audible speech. No latency budget is claimed from these static
or synthetic results.

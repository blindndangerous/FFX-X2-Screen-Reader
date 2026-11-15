# FFX-2 address evidence

## Gameplay qualification (November 15)

Every FFX-2 address and structure offset recorded here remains **UNVERIFIED
in game**. Historical headings such as "Verified" describe static binary
correspondence, not gameplay confirmation. No game was launched, attached to,
controlled or deployed during this work. New submenu and battle code has no
live hook. The already-existing top-menu hook remains the only menu reader.

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
  +0x148) and one FFX block is missing.
- 0x4e2e20 and FFX-2 0x389f20. No masked match, but both are 277 bytes and
  89 instructions with the same control flow. Only structure field offsets
  move: +0x48 to +0x4c, +0x45 to +0x48, +0x58 to +0x5c, +0x90 to +0x94. It is
  probably the same function on a changed structure. The FFX handlers read
  the old offsets, so the address is not usable until those reads are ported.
- 0x376220 (help-page input). No FFX-2 match. FFX-2 does not hook it.
- 0x48dfe0 (field initialize, the third FFX resource scope). Not searched
  yet. Under FFX-2 `resourceScopeHook()` returns false for it.

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
when the flag is zero. Other values suppress top-menu capture.

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

`ffx2OnlyAddresses` in `src/native/profile.hpp` holds FFX-2.exe RVAs that have
no FFX counterpart. `ffx2Rva(name)` throws outside the FFX-2 profile. Hook ids
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

Additional static instruction checks for the **UNVERIFIED** existing globals:

- Cursor: RVA `0x3731f4`, `a1 0c ef 6b 01`, reads VA `0x16bef0c`;
  RVA `0x37332b`, `a3 0c ef 6b 01`, writes it after a cursor move.
- Item list: RVA `0x3761f7`, `8b 34 c5 e0 ef 6b 01`, reads the dword
  ID from VA `0x16befe0 + 8*cursor`.
- Count: RVA `0x373370`, `3b 05 40 f0 6b 01`, compares against VA
  `0x16bf040` for wrap-around.
- Story flag: RVA `0x392ee0`, `a1 54 f6 6e 01`, reads VA `0x16ef654`.
- Help and charset pointer store bytes are recorded in the submenu and
  charset-capture sections below. Their final gameplay meaning remains unverified.

## Native submenu investigation. UNVERIFIED (November 15)

These findings use only the pinned executable and read-only archive entries.
No submenu address was added to `ffx2OnlyAddresses`; no new submenu hook exists.
The parsers in `ffx2_submenus.*` consume supplied, owned bytes only. A passing
synthetic parser test does not verify which live controller owns those bytes.

### Items. UNVERIFIED

- Route: top ID 0 dispatches slot 5. RVA `0x37621a`, `6a 05`, followed by
  `e8 8f e8 fe ff`, calls the module activator. Registry slot 5 contains
  module VA `0xd631e0`, whose update is RVA `0x37b6f0`.
- Window pointer: RVA `0x12bf514`, VA `0x16bf514`. RVA `0x37b715`,
  `8b 15 14 f5 6b 01`, loads it into EDX. RVA `0x37b78e`,
  `0f bf 51 4c`, reads its signed word cursor at `+0x4c`.
- The effective column is the signed activity/column byte at `+0x48`,
  clamped against byte `+0x45`. Negative activity suppresses help. RVA
  `0x37eb50`, `0f bf 4e 4c`, reads the row; `0x37eb54`,
  `8b 44 96 5c`, reads the column base at `+0x5c+4*column`;
  `0x37eb58`, `8d 0c 48`, computes `base+2*row`. The `+0x98` dword
  must be nonzero (`0x37eb5d`, `39 86 98 00 00 00`).
- Main item rows: RVA `0xdb8e00`, VA `0x11b8e00`, stride 4. RVA
  `0x37eb82`, `0f b7 04 8d 00 8e 1b 01`, loads the word ID.
  RVA `0x37eb79`, `38 04 8d 03 8e 1b 01`, tests byte `+3` against zero.
  Other list modes use other arrays; this is not permission to reuse the
  main item layout for every inventory pane or target/quantity dialog.
- Help: RVA `0x37eb8b`, `e8 40 58 01 00`, calls getter RVA `0x3943d0`
  with the ID. RVA `0x37ebf7`, `e9 54 6e fe ff`, forwards the resulting
  pointer to RVA `0x365a50`. The selected help pointer is the existing
  `HelpText` RVA `0x12bd97c`.

### Abilities. UNVERIFIED

- Route: top ID 4 dispatches slot 4. RVA `0x376298`, `6a 04`, followed by
  `e8 11 e8 fe ff`. Registry slot 4 contains module VA `0xd62eb8`,
  update RVA `0x376870`.
- Window pointer: RVA `0x12bf268`, VA `0x16bf268`. RVA `0x376a07`,
  `a3 68 f2 6b 01`, stores the result of the constructor at `0x378d00`.
  The selected help helper is `0x3777e0`. Constructor RVA `0x378d17`,
  `c7 46 0c 90 71 77 00`, installs update callback `0x377190` at `+0xc`.
  RVA `0x378d25`, `c7 46 14 10 8f 77 00`, installs `0x378f10` at
  `+0x14`; callback roles must be traced rather than assumed by offset.
- Cursor/list indexing: help callback RVA `0x37780a`, `0f bf 4e 4c`,
  reads the signed row. `0x37780e`, `8b 44 96 5c`, reads its column base;
  `0x377812`, `8d 04 48`, computes `base+2*row`. Activity/column
  handling at `+0x48` and `+0x45` matches the Items calculation.
- Rows: RVA `0xdbc400`, VA `0x11bc400`, stride 16. RVA `0x377815`,
  `c1 e0 04`, multiplies by 16; `0x377818`, `05 00 c4 1b 01`, adds
  the table. Byte `+0` gates availability (`80 38 00` at `0x37781d`);
  the dword at `+4` supplies the ID (`ff 70 04` at `0x377822`).
- Help: RVA `0x377825`, `e8 a6 cb 01 00`, calls `0x3943d0`;
  `0x37783c`, `e9 0f e2 fe ff`, forwards to `0x365a50`. The final
  pointer is again `HelpText` RVA `0x12bd97c`.

### Garment Grids. UNVERIFIED

- Route: top ID 3 dispatches slot 6 (`6a 06` at RVA `0x376276`).
  Module VA `0xd631bc`, update `0x37b550`, enters the legacy controller:
  RVA `0x37b586`, `e8 c5 75 e7 ff`, calls `0x1f2b50`; update RVA
  `0x37b59c`, `e8 3f 73 e7 ff`, calls `0x1f28e0`.
- There are several panels. The node cursor is signed word RVA
  `0x16c9a68` (VA `0x1ac9a68`); a separate list/action cursor is
  signed word RVA `0x16c9b88` (VA `0x1ac9b88`). RVA `0x1f2961`,
  `0f bf 05 68 9a ac 01`, reads the first; RVA `0x1f2942`,
  `0f bf 05 88 9b ac 01`, reads the second. The node-edit callback reads
  panel `+8` (`0f b7 47 08` at `0x1f34fa`) and bounds it below 32
  (`66 83 f8 20` at `0x1f3507`). Do not confuse this node cursor with
  a selected Garment Grid ID or the available-dressphere list cursor.
- The selected grid is signed word `+0xe` in the node panel at VA
  `0x1ac9a60`, hence RVA `0x16c9a6e`. Constructor RVA `0x1e53ef`,
  `0f b7 05 78 5d df 00`, reads the current grid at VA `0xdf5d78`;
  `0x1e53f6`, `66 89 46 0e`, stores it in that field. Constructor
  `0x1e53d2`, `c7 46 1c 20 00 20 00`, initializes a 32-node bound.
  Update `0x1f095e`, `0f bf 43 0e`, reads the grid, and `0x1f0962`,
  `a3 78 5d df 00`, synchronizes the current-grid global after transition.
- A sorted byte ID list is at RVA `0x9f4fc4`, VA `0xdf4fc4`: getter
  `0x1ea7a6`, `0f b6 80 c4 4f df 00`. Its count is stored at RVA
  `0x9f502c` (`89 3d 2c 50 df 00` at `0x1ea7fa`). The builder at
  `0x1ea7b0` filters records through `0x20c6e0` before sorting. This is
  an available-dressphere list used by the controller, not proof of a
  complete owned-Garment-Grid catalog.
- Grid node data comes through pointer RVA `0x9f4fc0`, VA `0xdf4fc0`.
  Getter `0x1eb9e6`, `8b 15 c0 4f df 00`, reads it; subsequent shifts
  compute `(grid*32+node)*8` bits and return an eight-bit node value.
  It is not a string list.
- Help routes include `0x1f30a0`: signed panel `+8` (`0f bf 40 08`
  at `0x1f30a8`), plus `0x1088` (`05 88 10 00 00`), text getter
  `0x21d7c0` with space 1, then help setter `0x3638a0`.
  The list callback at `0x1f30d0` handles the final three action rows
  separately; other rows reach record-based helper `0x1e5920`.
  RVA `0x1e5970`, `0f b7 40 04`, takes the help offset from a resolved
  record; `0x1e597b`, `e8 20 df 17 00`, calls the same help setter.
  Setter RVA `0x3638b6`, `a3 7c d9 6b 01`, stores the shared help pointer.
- The node parser selects byte `grid*32+node` from an owned node bank,
  matching getter `0x1eb9e0`. Zero is empty: the native clearing helper
  `0x1ebf30` writes zero, and the clear action at `0x1f38d1` tests for a
  positive getter result. Main-story grid/node parsing has separate tests
  from the available-dressphere list. Whether node-only focus refreshes the
  shared help pointer remains UNVERIFIED; another pane's stale help is not usable.
  It requires panel kind 2 (`66 89 46 18` at `0x1e5401`, following
  `mov eax,2`) and the constructor's 32-node count, excluding other pane layouts.

### Dresspheres. UNVERIFIED

- Route: top ID 6 dispatches slot `0x11` (`6a 11` at RVA `0x3762d2`).
  Module VA `0xd636d0`, update `0x385650`, delegates to controller
  `0x75e480`: RVA `0x385671`, `e8 0a 8e 3d 00`.
- The parent selection panel is VA `0x1ac9a00`: RVA `0x75e554`,
  `68 00 9a ac 01`, passes it to constructor `0x1e5470`. The signed
  cursor is `+8`, RVA `0x16c9a08`; constructor `0x1e54b6`,
  `66 89 47 08`, resets it, and `0x1e54c6`, `66 89 77 1e`, stores
  its count at `+0x1e`.
  The parser requires parent kind 0 (`66 89 47 18` at `0x1e54f9`,
  following `xor eax,eax`), excluding the node and text-display panes.
- Selection callback `0x75e6b0` proves the list binding: `0x75e6b7`,
  `0f bf 46 08`, reads the cursor; `0x75e6bc`, `e8 df c0 a8 ff`,
  calls byte-ID getter `0x1ea7a0` over VA `0xdf4fc4`. Its return is the
  selected dressphere ID, stored at RVA `0x1665ba0` (VA `0x1a65ba0`)
  by `0x75e6c8`, `a3 a0 5b a6 01`.
- Help: `0x75e6d9`, `e9 42 72 a8 ff`, forwards that ID to `0x1e5920`,
  then to help setter `0x3638a0`. The help line is the shared pointer at
  RVA `0x12bd97c`.
- The child display at VA `0x1ac9a60` mirrors the parent cursor:
  `0x75e6cd`, `66 89 0d 68 9a ac 01`. It is not the primary input owner.
- Globals VA `0x1a65b94` and `0x1a65b98` are **not cursor addresses**.
  For example `0x75e8e5`, `89 0d 94 5b a6 01`, and
  `0x75eb3f`, `ff 0d 94 5b a6 01`, change the first inside rendering.
  These control display animation. The selected ID at VA `0x1a65ba0`
  is also distinct from the parent cursor.

### Config. UNVERIFIED

- Route: top ID 9 dispatches slot 8 (`6a 08` at RVA `0x376320`). Module
  VA `0xd63924` has update RVA `0x388880`; its window pointer is RVA
  `0x12d5f44`, VA `0x16d5f44` (`a3 44 5f 6d 01` at `0x388a6c`).
- Cursor: dword RVA `0x12d5f38`, VA `0x16d5f38`. Up reads/decrements it
  at `0x388d30`, `8b 15 38 5f 6d 01`, and stores at `0x388d40`,
  `89 15 38 5f 6d 01`; Down stores at `0x388d97` with the same bytes.
  The scroll origin at VA `0x16d5f30` is separate from this cursor.
- Item list: pointer RVA `0x12d5f40` (VA `0x16d5f40`) selects a table of
  setting-record pointers. Main-story mode zero takes the default branch:
  `0x3889fd`, `c7 05 40 5f 6d 01 b4 38 d6 00`, selects table RVA
  `0x9638b4` (VA `0xd638b4`). `0x388a07`, `c6 05 48 5f 6d 01 09`,
  sets count RVA `0x12d5f48` to nine. The mode lookup uses the same
  `0x392ee0` getter as the top-menu story guard. Other mode branches are
  outside this main-story parser.
- Record prefix: word `+2` is the setting label ID (`0f bf 40 02` at
  `0x389b29`); word `+4` is the choice count; words `+6/+8/+0xa` are
  choice label IDs (`0f bf 04 13` at `0x3897a2`, with EBX initially 6).
  Signed word `+0xc` is the current choice (`0f bf 41 0c` at `0x3891bc`).
  The parser copies only this 16-byte prefix and never follows record callbacks.
- Label/value strings use resolver `0x379170`, which calls `0x21d840`
  with space 1 (`e8 c1 46 ea ff` at `0x37917a`). Unlike `0x21d7c0`,
  this resolver returns the name offset, word `+0` of the kernel record:
  `0x21d881`, `0f b7 00`. Selection speech must use that distinction.
- Help line: no separate ready help-string producer is confirmed for native
  Config. The nine main-story setting records name Spherechanges, ATB Mode,
  ATB Speed, Cursor, Battle Help, Vibration, Subtitles, Subtitle Names and
  Guide Map. Their `menu_txt.bin` help fields contain the literal `UNUSED`.
  That placeholder and the previous top-menu Config help are not submenu help.
  Visible-help absence still needs gameplay confirmation; no address is invented.
- Correction to the initial descriptor hypothesis: the 16-byte arrays at
  RVAs `0x12d5f50`, `0x12d6000`, `0x12d60b0` are animation state,
  not label/value/help text objects. Their consumer `0x372eb0` decrements
  a delay (`fe c8` at `0x372ebd`) and advances an interpolation frame
  (`66 89 47 08` at `0x372f35`). The old descriptor-copy parser was
  removed after tracing this consumer; it did not identify text.

## What runs under FFX-2

### Charset capture and preparation (November 15)

The top-menu hook copies the loaded charset and selected help bytes into one
owned `ffx2_menu_snapshot.hpp` packet. Only the existing speech preparation
worker parses UTF-8, decodes game text, assembles speech and logs it. Native
capture performs no allocation, decode, logging, wait or worker-memory read.
Queue refusal drops the observation; there is no synchronous fallback.
The packet retains its originating speech generation. Retired packets are silent.

There is no pointer-keyed cache: an allocator can reuse the same charset address
after a language/resource reload. Every move captures the current bytes. Missing
or malformed charset data produces the item label alone. Read-only archive
inspection found the pinned English charset file is exactly 310 bytes. The
loader therefore allocates 321 bytes, including padding and its final NUL.
Capture reads only those 310 file bytes; the old 1024/512/336-byte attempts
could all exceed the allocation. This extent remains **UNVERIFIED in game**.
Preparation requires a complete, NUL-free UTF-8 file with 192 glyphs. The
padding-tolerant `parseLoadedCharset` remains a pure helper; the runtime uses
strict `parseCharset` because no loader padding is captured now. A malformed
suffix after a valid prefix must not be accepted as padding.
Help capture copies through the encoded terminator, at most 256 bytes, without
reading beyond it. NUL control parameters are copied as parameters. An unreadable
or unterminated line produces the label alone. Japanese/Korean/Chinese escape
text and parameterized controls remain unsupported. Do not report those as
localized speech support.

Static evidence for the **UNVERIFIED** charset pointer: RVA `0x11124c`,
`a1 e0 ad de 00`, reads VA `0xdeade0`; RVA `0x11125e`,
`89 3d e0 ad de 00`, replaces it after freeing the preceding buffer.
RVA `0x11121a`, `c6 04 37 00`, writes the trailing NUL at `[edi+esi]`.
The existing **UNVERIFIED** module phase offset `+0x1c` comes from
RVA `0x3731c4`, `8b 47 1c` (`mov eax,[edi+0x1c]`);
only phase 5 is eligible for top-menu capture. The synthetic boundary fixture
checks source-buffer mutation, same-address reload, profile and phase guards,
missing/malformed charset, queue pressure and generation retirement.

Archive provenance (read-only; no extracted asset saved):
`data/FFX2_Data.vbf`, entry
`ffx_ps2/ffx2/master/jppc/ffx2_encoding/ffx2sjistbl_us.bin`, 310 bytes,
192 UTF-8 code points, SHA-256
`9f96e3b904c0ed332bb6859ec74f36b5a440c9c10c00fad24099afd42bd39af8`.

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
- 0x373170 (FFX-2-only): the top-level menu. On each eligible cursor move it
  captures owned text. Preparation speaks "<item name>. <help line>" and logs
  "FFX-2 top menu id=<id> charset glyphs=<n>".
- No other reader, menu, battle, field, controller, beacon or minigame hook. The
  input code skips the ESC menu, Blitzball, field, battle-sensor and minigame
  predicates under FFX-2, because they read FFX memory.

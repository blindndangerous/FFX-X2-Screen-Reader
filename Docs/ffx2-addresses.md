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
- Not done yet: the runtime does not read the table from the game. The
  0xdeade0 pointer is not in the verified table, and no FFX-2 text source is
  hooked. The decoder and its fixture use synthetic bytes only.
- Not handled: the meaning of the two-byte controls (FFX uses them for
  button glyphs, colours and name macros). The decoder drops them.

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
- No reader, menu, battle, field, controller, beacon or minigame hook. The
  input code skips the ESC menu, Blitzball, field, battle-sensor and minigame
  predicates under FFX-2, because they read FFX memory.

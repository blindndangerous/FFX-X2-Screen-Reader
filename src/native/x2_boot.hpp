#pragma once
namespace ffx {
// FFX-2 boot: the hash matched Steam FFX-2.exe. Logs and queues one startup
// line on the bootstrap worker. Installs no hook and reads no game memory.
void announceFfx2Boot();
// MinHook plus the keyboard input hooks, F8/F11, the map load boundaries and
// the top-level menu reader.
// Throws if an address is unverified.
bool startFfx2Input(bool& initializedHooks);
// reader.cpp: hooks the verified FFX-2 map request and map load as resource
// scopes that invalidate pending speech.
void installFfx2LoadBoundaries();
// ffx2_menu_reader.cpp: hooks the top-level menu update and speaks the
// focused item on cursor moves. False when the hook was not installed.
bool installFfx2TopMenu();
}

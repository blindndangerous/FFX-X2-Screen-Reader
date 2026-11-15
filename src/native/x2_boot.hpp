#pragma once
namespace ffx {
// FFX-2 boot: the hash matched Steam FFX-2.exe. Logs and queues one startup
// line on the bootstrap worker. Installs no hook and reads no game memory.
void announceFfx2Boot();
// MinHook plus the keyboard input hooks, F8/F11 and the map load boundaries.
// Throws if an address is unverified.
bool startFfx2Input(bool& initializedHooks);
// reader.cpp: hooks the verified FFX-2 map request and map load as resource
// scopes that invalidate pending speech.
void installFfx2LoadBoundaries();
}

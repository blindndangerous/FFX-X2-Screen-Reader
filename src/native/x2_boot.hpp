#pragma once
namespace ffx {
// FFX-2 boot: the hash matched Steam FFX-2.exe. Logs and queues one startup
// line on the bootstrap worker. Installs no hook and reads no game memory.
void announceFfx2Boot();
// MinHook plus the keyboard input hooks and F8/F11. Throws if an address is unverified.
bool startFfx2Input(bool& initializedHooks);
}

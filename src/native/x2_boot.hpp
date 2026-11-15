#pragma once
namespace ffx {
// FFX-2 boot only: the hash matched Steam FFX-2.exe. Logs and queues one
// startup line on the bootstrap worker. Installs no hook and reads no game memory.
void announceFfx2Boot();
}

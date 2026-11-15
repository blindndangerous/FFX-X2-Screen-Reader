// FFX-2 boot announcement. FFX-2.exe has no verified addresses yet, so the
// reader stops after this; see Docs/gameArchitecture.md (FFX-2 status).
#include "x2_boot.hpp"
#include "runtime.hpp"
#include "log_capture.hpp"
namespace ffx {
void announceFfx2Boot() {
    recordLog("FFX-2 boot: speech only; no game hooks installed, menus are not read yet");
    if(!output(L"FFX-2 screen reader loaded. Menus are not read yet.",false,"x2-boot"))
        recordLog("FFX-2 boot: startup line was not queued");
}
}

// FFX-2 startup. FFX-2.exe has only the verified input and map load addresses
// in profile.hpp (ffx2Addresses), so it gets keyboard hotkeys, the speech
// settings menu and map load scopes; see Docs/ffx2-addresses.md and
// Docs/gameArchitecture.md (FFX-2 status).
#include "x2_boot.hpp"
#include "runtime.hpp"
#include "events.hpp"
#include "log_capture.hpp"
#include "mod_menu.hpp"
#include "text_plan.hpp"
#include <MinHook.h>
namespace ffx {
void announceFfx2Boot() {
    recordLog("FFX-2 boot: keyboard input and map load scopes; menus are not read yet");
    if(!output(L"FFX-2 screen reader loaded. Menus are not read yet. F8 opens speech settings.",false,"x2-boot"))
        recordLog("FFX-2 boot: startup line was not queued");
}
bool startFfx2Input(bool& initializedHooks) {
    if(MH_Initialize()!=MH_OK) { recordLog("FFX-2 input refused: MinHook unavailable"); return false; }
    initializedHooks=true;
    installTextPlans();
    installFfx2Input();
    installFfx2ModMenu();
    installFfx2LoadBoundaries();
    if(!enableHooks()) { recordLog("FFX-2 input hooks failed to enable"); return false; }
    recordLog("FFX-2 input enabled: PInput::Update probe, DirectInput keyboard/pad, window messages; hotkeys F8 and F11; map load scopes");
    return true;
}
}

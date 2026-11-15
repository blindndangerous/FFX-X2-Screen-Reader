// FFX-2 startup. FFX-2.exe has only the verified input and map load addresses
// in profile.hpp (ffx2Addresses) plus the FFX-2-only top menu addresses
// (ffx2OnlyAddresses), so it gets keyboard hotkeys, the speech settings menu,
// map load scopes and the top-level menu; see Docs/ffx2-addresses.md and
// Docs/gameArchitecture.md (FFX-2 status).
// Field page/choice capture additionally requires FFX2_EXPERIMENTAL_FIELD_TEXT=1.
#include "x2_boot.hpp"
#include "runtime.hpp"
#include "events.hpp"
#include "log_capture.hpp"
#include "mod_menu.hpp"
#include "text_plan.hpp"
#include "ffx2_field_text.hpp"
#include <MinHook.h>
namespace ffx {
void announceFfx2Boot() {
    recordLog("FFX-2 boot: keyboard input, map load scopes and the top-level menu; other menus are not read yet");
    if(!output(L"FFX-2 screen reader loaded. Only the main menu is read so far. F8 opens speech settings.",false,"x2-boot"))
        recordLog("FFX-2 boot: startup line was not queued");
}
bool startFfx2Input(bool& initializedHooks) {
    if(MH_Initialize()!=MH_OK) { recordLog("FFX-2 input refused: MinHook unavailable"); return false; }
    initializedHooks=true;
    installTextPlans();
    installFfx2Input();
    installFfx2ModMenu();
    installFfx2LoadBoundaries();
    const bool topMenu=installFfx2TopMenu();
    const bool fieldText=installFfx2FieldText();
    if(!enableHooks()) { recordLog("FFX-2 input hooks failed to enable"); return false; }
    recordLog("FFX-2 input enabled: PInput::Update probe, DirectInput keyboard/pad, window messages; hotkeys F8 and F11; map load scopes; top menu %s",topMenu?"read":"not hooked");
    recordLog("FFX-2 experimental field text %s",fieldText?"enabled; static-only, unverified in game":"disabled");
    return true;
}
}

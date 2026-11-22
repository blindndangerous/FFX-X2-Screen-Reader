#pragma once
#include "message_text.hpp"
namespace ffx {
// Native owner only. TK window phases 1/2 are open (as in dialogue review);
// message mode alone can remain set throughout a script with no text window.
inline bool nativeVisibleMessageText(unsigned* visibleSlots=nullptr) {
    if(visibleSlots) *visibleSlots=0;
    for(unsigned slot=0;slot<8;++slot) {
        const auto window=read<uintptr_t>(address(0x14676b0)+slot*4);
        if(!window) continue;
        const auto state=read<uint16_t>(window+0x2c);
        if(state==0 || state>=3) continue;
        const auto text=read<uintptr_t>(window+0x94);
        if(text && read<uint8_t>(text)>2) {
            if(!visibleSlots) return true;
            *visibleSlots|=1u<<slot;
        }
    }
    return visibleSlots && *visibleSlots!=0;
}
// Native owner only: the same dialogue and controlled-actor facts guard field assistance.
inline const wchar_t* nativeFieldControlReason() {
    const auto controller=read<uintptr_t>(address(0xf26ae8));
    if(!controller || !(read<uint8_t>(controller)&2) || !read<uintptr_t>(address(0xf00740))) return L"player control disabled";
    using Mode=int(__cdecl*)();
    return reinterpret_cast<Mode>(address(0x46c840))()!=0?L"dialogue":nullptr;
}
struct FieldControlsSnapshot {
    unsigned epoch=0,revision=0,slot=8,serial=0;
    uintptr_t window=0,source=0;
    bool candidate=false;
};
FieldControlsSnapshot captureFieldControls(unsigned slot,uintptr_t source);
bool observeFieldControlsOwned(const MessagePage&,const FieldControlsSnapshot&);
void installFieldControls();
void clearDialogueReviewOwned(); // Preparation owner only; no native view mutation.
enum class FieldControlsClassification {Pending,Nonblocking,Blocking};
FieldControlsSnapshot fieldControlsForSource(unsigned slot,uintptr_t source);
FieldControlsClassification classifyFieldControls(const FieldControlsSnapshot&);
void invalidateFieldControlsSlot(unsigned slot);
void rejectFieldControlsCapture(const FieldControlsSnapshot&);
void resetFieldControls();
bool fieldControlsCapturesKey(unsigned vk, bool control);
bool dispatchFieldControlsKey(unsigned vk, bool control);
}

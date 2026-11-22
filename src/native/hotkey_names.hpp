#pragma once
#include "controller_model.hpp"
#include "input_device.hpp"
#include <atomic>
#include <cwctype>
#include <string>
namespace ffx {
// Spoken names for mod keys that follow the last used input device. Text is
// resolved when it is spoken, so a device switch applies to the next message.
// The controller bindings array belongs to the native owner. This copy is safe
// to read on any thread; the owner publishes it after load and every rebind.
inline std::atomic<int> spokenControllerBindings[pad::CountCommands]{};
inline std::atomic<bool> spokenControllerBindingsReady{false};
template<class Bindings> void publishSpokenControllerBindings(const Bindings& values) {
    for(int i=0;i<pad::CountCommands;++i) spokenControllerBindings[i].store(values[i],std::memory_order_relaxed);
    spokenControllerBindingsReady.store(true,std::memory_order_release);
}
inline int spokenControllerBinding(int command) {
    if(command<0 || command>=pad::CountCommands) return -1;
    return spokenControllerBindingsReady.load(std::memory_order_acquire)?
        spokenControllerBindings[command].load(std::memory_order_relaxed):pad::defaults[command];
}
enum class ModKey {Route,PuzzleStep,PuzzlePath,ModMenu};
// Keyboard: backslash, F9, F10, F8. Controller: the saved binding; mod-layer
// commands start with View (controller.cpp: View toggles the mod layer).
inline std::wstring modKeyName(ModKey key,bool onPad=nativeInputUsesController()) {
    static constexpr const wchar_t* keyNames[]={L"backslash",L"F9",L"F10",L"F8"};
    const auto index=static_cast<unsigned>(key);
    if(!onPad) return keyNames[index];
    if(key==ModKey::ModMenu) return std::wstring(L"View, then ")+pad::controlNames[pad::Menu];
    const int command=key==ModKey::Route?pad::Route:key==ModKey::PuzzleStep?pad::PuzzleStep:pad::PuzzlePath;
    const int control=spokenControllerBinding(command);
    if(control<0 || control>=pad::Count) return std::wstring(keyNames[index])+L" on the keyboard";
    return (key==ModKey::Route?std::wstring():std::wstring(L"View, then "))+pad::controlNames[control];
}
// Static text written with keyboard names (guide steps, setting descriptions):
// on a controller, swap whole-word F9, F10 and backslash for the bindings.
// Shift chords have no controller equivalent and stay as written.
inline std::wstring deviceKeyText(std::wstring text,bool onPad=nativeInputUsesController()) {
    if(!onPad) return text;
    struct Token {const wchar_t* key;ModKey mod;};
    static constexpr Token tokens[]={{L"F10",ModKey::PuzzlePath},{L"F9",ModKey::PuzzleStep},{L"backslash",ModKey::Route},{L"Backslash",ModKey::Route}};
    for(const auto& token:tokens) {
        const std::wstring key=token.key;
        for(size_t at=text.find(key);at!=std::wstring::npos;at=text.find(key,at)) {
            const auto end=at+key.size();
            const bool shifted=at>=6 && (!text.compare(at-6,6,L"Shift ") || !text.compare(at-6,6,L"Shift+"));
            if(shifted || (at && std::iswalnum(text[at-1])) || (end<text.size() && std::iswalnum(text[end]))) {at=end;continue;}
            const auto name=modKeyName(token.mod,true);text.replace(at,key.size(),name);at+=name.size();
        }
    }
    return text;
}
}

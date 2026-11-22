#pragma once
// Development test pipe (\\.\pipe\ffx-sr-dev-<pid>). Release builds contain
// none of it: every declaration is FFX_DEV_UTILITIES only.
// Protocol, one UTF-8 line each way: see dev_pipe.cpp.
#include <string>
namespace ffx {
#ifdef FFX_DEV_UTILITIES
// Structured event for scripts. Any thread; the native game thread never
// takes a lock (its events cross a lock-free queue). channel: step, target,
// action, key. Text is truncated to the event slot.
void devPipeEvent(const char* channel,const char* text) noexcept;
// Speech worker tee after the speech log line. The log format is unchanged.
void devSpeechTap(const char* producer,const char* utf8) noexcept;
// Last event sequence number given out (0 before the first event).
unsigned devPipeSequence() noexcept;
// Bootstrap only: starts the pipe thread once.
void startDevPipe();
// Native input callback, after pumpUtilities. Runs at most one pipe command.
void pumpDevPipe();
// Pipe-thread owner: handles one request line and returns one reply line
// without the newline. Tests call it directly.
std::string devPipeHandleLine(const std::string& utf8);
// Pipe-thread owner: called when a client disconnects (clears focus hold).
void devPipeClientGone();
#endif
}

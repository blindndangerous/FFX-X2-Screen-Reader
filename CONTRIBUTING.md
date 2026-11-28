# Contributing

This guide explains how to build and test the FFX Screen Reader, and which rules a change must follow. Players should read the [player guide](Docs/controls.md) instead.

## Repository layout

- `src/proxy`: the x86 `dinput8.dll` proxy, the build entry point and the guarded deployment scripts.
- `src/native`: the screen reader itself, in C++. Its fixtures are in `src/native/tests`.
- `src/launcher`: speech for the original game launcher, through a C++/CLI adapter.
- `src/installer`: the C# Windows installer that players run.
- `src/speech`: PRISM dependency scripts.
- `src/navigation`, `src/minigames`, `src/puzzles`: offline data tools. They do not ship.
- `Docs/Scripts`: reverse-engineering and offline analysis scripts. They do not ship.
- `Docs/adr`: architecture decision records.

## Set up and build

Use the [build-from-source instructions](README.md#building-from-source) to
install the Windows x86 toolchain and fetch the locked MinHook and SDL3 inputs.
The shared development environment can supply those dependencies too. See the
[developer build and CI procedure](Docs/release.md#developer-build-and-ci).

From Command Prompt at the repository root, stage the release build.

```bat
src\proxy\build_dinput8.bat --release
```

The build uses the shared resource limits and stages files in `install\native`.
Ordinary builds stage by default. `--no-deploy` also stages only. Developer
deployment requires the guarded deploy script or the explicit `--deploy` flag
after verification. `--release` and `--test-build` exclude the developer utilities.

## Test

Run the same public native checks as Windows CI from Command Prompt.

```bat
python tools\run-build.py -- cmd.exe /d /c call src\native\tests\run_tests.bat --public
```

Run the public installer checks from PowerShell.

```powershell
.\src\installer\tests\run_tests.ps1 -Public
```

Full local suites need external game-derived evidence or the locked PRISM
runtime. Omit the public flags for those suites. Native suites can also be
run individually with selectors such as `--readers`, `--abi` or `--threading`.
Keep game-derived evidence outside the Git tree. The fixtures use controlled
inputs. Gameplay confirmation is separate.

Standalone setup and the game payload are separate release artifacts. See
the [release procedure](Docs/release.md) for packaging and verification.

## Rules for changes

- Follow the [non-blocking hook contract](Docs/adr/0002-non-blocking-hook-contract.md). Code that runs on a game thread must not wait for locks, queues, file I/O, sleeps or other threads.
- Add production behavior to the native reader only. See [native-only runtime](Docs/adr/0001-native-only-runtime.md).
- Send speech and braille through the PRISM adapter. See [PRISM speech backend](Docs/adr/0003-prism-speech-backend.md).
- Do not commit game files, decompiled game code, tester logs, or PRISM or SDL binaries.

## Documentation

- `README.md`: player overview, installation and known issues.
- `Docs/controls.md`: the player guide.
- `CHANGELOG.md`: release history.
- `Docs/features/index.md`: the current feature map, with links to each subsystem.
- `Docs/gameArchitecture.md`: reverse-engineering reference for the game.
- [Debugging record](Docs/debug.md): prior failures, diagnosis and verification.
  Search it for the affected feature or symptom.
- `Docs/release.md`: the release procedure.
- `Docs/adr`: why the main design choices were made.

To record a new design decision, copy the format of an existing ADR in `Docs/adr`. Give it the next number.

## Report a problem

The [README](README.md) lists what a problem report needs.

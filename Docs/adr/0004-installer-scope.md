---
status: accepted
date: 2026-10-07
decision-makers: project maintainer
---

# Reusable Windows setup and GitHub releases

The Windows installer dates from 2026-10-07. This record incorporates the
later GitHub update and safe-list decisions.

## Context and Problem Statement

The 0.5-Test Build shipped a ZIP with a batch setup script, `InstallLauncher.bat` (`CHANGELOG.md`). Players had to run scripts, and the scripts had to find the game, back up files and remove the old Frida mod.

What should setup do, and what must it never do?

## Decision Drivers

- Players run one program and do not need a command prompt.
- Setup must not damage or redistribute original game files.
- Setup must be able to undo its changes if it fails.
- Old mods that also hook the game can conflict with this reader.

## Considered Options

- Keep the batch and PowerShell setup scripts.
- A Windows Forms installer with a bundled payload.
- A reusable Windows Forms installer that fetches a GitHub-hosted payload.

## Decision Outcome

Players keep `InstallFFXScreenReader.exe` and reuse it for supported installs
and updates. Setup verifies the downloaded payload before applying it.

The scope, from `Docs/windows_installer_20261007.md`, `Docs/features/distribution/index.md` and `src/installer`:

- Setup checks every original game DLL and EXE against a pinned official Steam inventory (`src/installer/MigrationPlan.cs`, `src/installer/BaseGameCatalog.cs`). If one is missing or changed, setup stops and asks the player to verify the game files in Steam. It never replaces or ships original game files.
- Setup checks GitHub for the latest stable release. Fresh installs and
  updates download the versioned ZIP, which supplies the mod, README and
  locked dependencies. A current installation checks release metadata only.
  Setup verifies the ZIP's digest, size and version before installing it.
- Setup installs or verifies the locked PRISM runtime (`src/installer/PrismInstall.cs`). See [ADR 0003](0003-prism-speech-backend.md).
- Setup backs up every file it will change or remove to `FFX_SR_Install_Backups` before it changes anything. If installation fails, it restores the old files.
- The per-installation mod safe list preserves selected DLL and ASI paths.
  Other added modules and known legacy files follow the migration policy
  (`src/installer/MigrationPlan.cs`, `src/installer/InstallerEngine.cs`).
- Setup keeps settings, saves, assets and unrelated launcher XML.
- Normal installation does not write to the registry. "Repair launch redirection" and "Restore launch backup" are separate actions that the player must confirm. They are limited to five documented Image File Execution Options names (`src/installer/LaunchRedirectRepair.cs`).
- Setup never starts the game.

Developers stage with `src/proxy/build_dinput8.bat` and use guarded deployment
after verification. `src/installer/build_installer.ps1` builds the standalone
setup. `src/installer/package_release.ps1` stages the separate release payload ZIP.

### Consequences

- Players reuse one program across compatible releases. Fresh installs and
  updates need a GitHub connection.
- Setup compatibility depends on its stock inventories and supported payload
  and PRISM versions.

### Confirmation

- `src/installer/tests/run_tests.ps1` covers backup, rollback, migration,
  registry actions and GitHub updates. The update fixtures use controlled
  release data.
- The [release procedure](../release.md) records setup, payload and integrity verification.

## More Information

- [Mod safe list](../windows_installer_20261007.md#mod-safe-list).
- [GitHub updates](../windows_installer_20261007.md#github-updates).
- [Distribution and installer ownership](../features/distribution/index.md).

# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What This Project Is

ApcMonitorCore is a Windows monitor enumeration library. It exposes monitor metadata — friendly name, serial number (extracted from EDID), geometry, work area, and a stable persistence key — through a native C++ static library and a C++/CLI managed wrapper for .NET.

## Build

Open `ApcMonitorCore.sln` in Visual Studio 2022 and build from the IDE, or use MSBuild:

```powershell
msbuild ApcMonitorCore.sln /p:Configuration=Release /p:Platform=x64
```

For the managed console app only:

```powershell
dotnet build ApcMonitorCore.Managed.Console
```

Target platforms are `Win32` and `x64`. Language standard is C++20 (x64) / C++17 (Win32).

## Running the Demo Apps

There is no automated test suite. Validation is done by running the two console applications and inspecting their output:

**Native demo** (after building):
```
x64\Release\ApcMonitorCore.Console.exe
```

**Managed (.NET) demo:**
```powershell
dotnet run --project ApcMonitorCore.Managed.Console
```

Both apps print all connected monitors — ID, primary flag, friendly name, device name, serial number, key, relative position, and monitor/work RECTs.

## Architecture

```
ApcMonitorCore (C++ static lib)
    MonitorService        — public API; calls Windows APIs to enumerate monitors
    MonitorData           — per-monitor result struct (geometry, names, serial, key, position)
    DisplayConfigData     — internal struct used to map QueryDisplayConfig results

ApcMonitorCore.Managed (C++/CLI DLL, net8.0)
    ManagedMonitorService — thin CLI wrapper around MonitorService
    ManagedMonitorData    — managed mirror of MonitorData

ApcMonitorCore.Console              — native C++ smoke test
ApcMonitorCore.Managed.Console      — C# smoke test
```

### Key data flow in `MonitorService::GetMonitorsData()`

1. `EnumDisplayMonitors` callback collects basic monitor handles and RECTs.
2. `QueryDisplayConfig` populates `DisplayConfigData` records with friendly names and device paths.
3. The two lists are merged by matching device names.
4. `TryGetMonitorSerialFromDevicePath` reads EDID from the Windows registry and parses the descriptor blocks to extract the serial number.
5. Each monitor gets a **Key**: `"SERIAL:<serial>"` when a serial is available, otherwise `"PATH:<device_path>"`. This key is stable across reboots and is intended for persisting monitor identity.

### Relative positioning

`MonitorData::RelativePosition` compares each monitor's RECT against the primary monitor's RECT to produce one of: `Primary`, `Left`, `Right`, `Above`, `Below`, `Overlap`.

### Display name formatting

`MonitorData::GetDisplayName(format)` accepts a format string with placeholders `{FriendlyName}`, `{Position}`, `{SerialNumber}`, `{Key}`, `{DeviceName}`, `{Rect}`, `{Size}`. Empty fields are cleaned up automatically.

## Dependencies

- **SetupAPI.lib**, **user32.lib**, **gdi32.lib**, **advapi32.lib** — all Windows SDK, no install needed.
- **System.Drawing.Common 8.0.0** — NuGet, used only in the managed console for `Rectangle`.
- **Microsoft.Windows.CppWinRT** — NuGet, used only in `ApcMonitorCore.Console`.

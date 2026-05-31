#include <windows.h>
#include <vector>
#include <stdexcept>
#include <SetupAPI.h>
#include <string>
#include <unordered_map>

#pragma comment(lib, "SetupAPI.lib")

#include "MonitorService.h"
#include "MonitorData.h"
#include "DisplayConfigData.h"
#include "EdidParser.h"

namespace
{
    BOOL CALLBACK EnumMonitorProc(const HMONITOR monitor, const HDC hdc, const LPRECT rect, const LPARAM lparam)
    {
        UNREFERENCED_PARAMETER(hdc);
        UNREFERENCED_PARAMETER(rect);

        // NOLINTNEXTLINE(performance-no-int-to-ptr)  // Win32 callback: LPARAM carries a pointer by API contract
        auto* monitors = reinterpret_cast<std::vector<MONITORINFOEX>*>(lparam);
        MONITORINFOEX info{};
        info.cbSize = sizeof(MONITORINFOEX);
        if (::GetMonitorInfo(monitor, &info))
        {
            monitors->push_back(info);
        }

        return TRUE;
    }

    /// <summary>
    /// Describes the position of a monitor RECT relative to the primary monitor's RECT
    /// </summary>
    /// <param name="r">Monitor RECT</param>
    /// <param name="primary">Primary monitor RECT</param>
    /// <returns>String describing the position</returns>
    std::wstring DescribePosition(const RECT& r, const RECT& primary)
    {
        // For primary itself
        if (EqualRect(&r, &primary))
        {
            return L"primary";
        }

        if (r.right <= primary.left)
        {
            return L"left";
        }

        if (r.left >= primary.right)
        {
            return L"right";
        }

        if (r.bottom <= primary.top)
        {
            return L"above";
        }

        if (r.top >= primary.bottom)
        {
            return L"below";
        }

        return L"overlap";
    }
}

std::vector<MonitorData> MonitorService::GetMonitorsData() const
{
    std::vector<MonitorData> returnData;

    auto monitorInfo = GetMonitorsInfo();
    auto displayInfo = GetDisplayConfigInfo();

    // First pass: create MonitorData objects matched by GDI device name.
    // EnumDisplayMonitors (GDI) and QueryDisplayConfig (CCD) enumerate in
    // independent orders, so pairing by index is not reliable.
    returnData.reserve(monitorInfo.size());
    for (const auto& info : monitorInfo)
    {
        const auto it = std::ranges::find_if(displayInfo,
            [&](const DisplayConfigData& d)
            {
                return _wcsicmp(d.GdiDeviceName.c_str(), info.szDevice) == 0;
            });

        if (it == displayInfo.end())
        {
            continue; // no matching display config entry — skip this monitor
        }

        const auto serial = TryGetMonitorSerialFromDevicePath(it->DevicePath);
        returnData.emplace_back(info, *it, serial);
    }

    // Second pass: determine relative positions
    RECT primaryRect{};
    for (const auto& md : returnData)
    {
        if (md.IsPrimary)
        {
            primaryRect = IsRectEmpty(&md.WorkRect) ? md.MonitorRect : md.WorkRect;
            break;
        }
    }

    if (primaryRect.right > 0)
    {
        for (auto& md : returnData)
        {
            // Explicitly check for the primary monitor first.
            if (md.IsPrimary)
            {
                md.RelativePosition = L"primary";
            }
            else
            {
                md.RelativePosition = DescribePosition(md.MonitorRect, primaryRect);
            }
        }
    }

    return returnData;
}

std::vector<MONITORINFOEX> MonitorService::GetMonitorsInfo()
{
    std::vector<MONITORINFOEX> monitors;

    if (!EnumDisplayMonitors(nullptr, nullptr, EnumMonitorProc, reinterpret_cast<LPARAM>(&monitors)))
    {
        throw std::runtime_error("Failed to enumerate monitors");
    }

    return monitors;
}

std::vector<DisplayConfigData> MonitorService::GetDisplayConfigInfo()
{
    std::vector<DisplayConfigData> returnData;

    std::vector<DISPLAYCONFIG_PATH_INFO> paths;
    std::vector<DISPLAYCONFIG_MODE_INFO> modes;
    constexpr UINT32 flags = QDC_ONLY_ACTIVE_PATHS | QDC_VIRTUAL_MODE_AWARE;

    long result;

    do
    {
        UINT32 pathCount;
        UINT32 modeCount;

        result = GetDisplayConfigBufferSizes(flags, &pathCount, &modeCount);
        if (result != ERROR_SUCCESS)
        {
            throw std::runtime_error("Failed to get display config buffer sizes");
        }

        paths.resize(pathCount);
        modes.resize(modeCount);

        result = QueryDisplayConfig(flags, &pathCount, paths.data(), &modeCount, modes.data(), nullptr);

        paths.resize(pathCount);
        modes.resize(modeCount);
    } while (result == ERROR_INSUFFICIENT_BUFFER);

    if (result != ERROR_SUCCESS)
    {
        throw std::runtime_error("Failed to query display config");
    }

    for (const auto& path : paths)
    {
        DISPLAYCONFIG_TARGET_DEVICE_NAME targetName{};
        targetName.header.adapterId = path.targetInfo.adapterId;
        targetName.header.id = path.targetInfo.id;
        targetName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
        targetName.header.size = sizeof(targetName);

        result = DisplayConfigGetDeviceInfo(&targetName.header);
        if (result != ERROR_SUCCESS)
        {
            throw std::runtime_error("Failed to get monitor friendly name");
        }

        // Retrieve the GDI source device name (e.g. "\\.\DISPLAY1") so we can
        // match this CCD path against the MONITORINFOEX.szDevice from GDI.
        DISPLAYCONFIG_SOURCE_DEVICE_NAME srcName{};
        srcName.header.adapterId = path.sourceInfo.adapterId;
        srcName.header.id = path.sourceInfo.id;
        srcName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
        srcName.header.size = sizeof(srcName);

        std::wstring gdiDeviceName;
        if (DisplayConfigGetDeviceInfo(&srcName.header) == ERROR_SUCCESS)
        {
            gdiDeviceName = srcName.viewGdiDeviceName;
        }

        returnData.emplace_back(
            path.targetInfo.id,
            targetName.flags.friendlyNameFromEdid ? targetName.monitorFriendlyDeviceName : L"Unknown",
            targetName.monitorDevicePath,
            gdiDeviceName);
    }

    return returnData;
}

std::wstring MonitorService::TryGetMonitorSerialFromDevicePath(const std::wstring& devicePath) const
{
    if (devicePath.empty())
    {
        return L"";
    }

    {
        std::lock_guard lock(m_cacheMutex);
        const auto it = m_serialCache.find(devicePath);
        if (it != m_serialCache.end())
        {
            return it->second;
        }
    }

    const HDEVINFO hSet = SetupDiCreateDeviceInfoList(nullptr, nullptr);
    if (hSet == INVALID_HANDLE_VALUE)
    {
        return L"";
    }

    std::wstring serial;

    SP_DEVICE_INTERFACE_DATA ifData{};
    ifData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    if (SetupDiOpenDeviceInterfaceW(hSet, devicePath.c_str(), 0, &ifData))
    {
        DWORD required = 0;
        SetupDiGetDeviceInterfaceDetailW(hSet, &ifData, nullptr, 0, &required, nullptr);

        if (required > 0)
        {
            std::vector<BYTE> buf(required);
            auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(buf.data());
            detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
            SP_DEVINFO_DATA devInfo{};
            devInfo.cbSize = sizeof(SP_DEVINFO_DATA);

            if (SetupDiGetDeviceInterfaceDetailW(hSet, &ifData, detail, required, nullptr, &devInfo))
            {
                // Device Parameters holds EDID for most monitors
                const HKEY hKey = SetupDiOpenDevRegKey(hSet, &devInfo, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
                if (hKey && hKey != INVALID_HANDLE_VALUE)
                {
                    DWORD type = 0;
                    DWORD size = 0;
                    if (RegQueryValueExW(hKey, L"EDID", nullptr, &type, nullptr, &size) == ERROR_SUCCESS &&
                        type == REG_BINARY && size > 0)
                    {
                        std::vector<BYTE> edid(size);
                        if (RegQueryValueExW(hKey, L"EDID", nullptr, &type, edid.data(), &size) == ERROR_SUCCESS)
                        {
                            const auto parsed = Edid::ParseSerial(edid.data(), size);
                            if (Edid::IsLikelyValidSerial(parsed))
                            {
                                serial = parsed;
                            }
                        }
                    }

                    RegCloseKey(hKey);
                }
            }
        }
    }

    SetupDiDestroyDeviceInfoList(hSet);

    // Cache even empty to avoid repeated attempts
    {
        std::lock_guard lock(m_cacheMutex);
        m_serialCache[devicePath] = serial;
    }
    return serial;
}

int MonitorService::FindMonitorIndex(const std::vector<MonitorData>& monitors, const std::wstring& key, const RECT& rect)
{
    if (!key.empty())
    {
        int index = 0;
        for (const auto& monitor : monitors)
        {
            if (_wcsicmp(monitor.Key.c_str(), key.c_str()) == 0)
            {
                return index;
            }
            ++index;
        }
    }

    // Legacy fallback using persisted RECT
    if (!IsRectEmpty(&rect))
    {
        int index = 0;
        for (const auto& monitor : monitors)
        {
            if (EqualRect(&monitor.MonitorRect, &rect))
            {
                return index;
            }
            ++index;
        }
    }

    return -1; // Not found
}

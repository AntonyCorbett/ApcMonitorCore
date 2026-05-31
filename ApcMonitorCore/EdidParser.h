#pragma once
#include <windows.h>
#include <string>

// EDID parsing utilities. Extracted from MonitorService for independent testability.
namespace Edid
{
    std::wstring Trim(const std::wstring& s);
    bool IsLikelyValidSerial(const std::wstring& s);

    // Parses a raw EDID byte buffer and returns the monitor serial number,
    // or an empty string if none can be determined.
    std::wstring ParseSerial(const BYTE* edid, DWORD size);
}

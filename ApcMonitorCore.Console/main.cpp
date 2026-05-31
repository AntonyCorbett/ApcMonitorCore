#include <iostream>
#include <vector>
#include "../ApcMonitorCore/MonitorService.h"
#include "../ApcMonitorCore/MonitorData.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib") 
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "SetupAPI.lib")

/// <summary>
/// A native C++ console application to test the MonitorService class
/// and display monitor information.
/// </summary>
int main()
{
    try 
    {
        // Create an instance of MonitorService
        const MonitorService service;

        // Get monitor data
        const auto monitors = service.GetMonitorsData();

        // Display information about each monitor
        std::wcout << L"Found " << monitors.size() << L" monitor(s):\n\n";

        for (size_t i = 0; i < monitors.size(); ++i) 
        {
            const auto& monitor = monitors[i];

            std::wcout << L"Monitor " << (i + 1) << L":\n";
            std::wcout << L"  ID: " << monitor.Id << L"\n";
            std::wcout << L"  Primary: " << (monitor.IsPrimary ? L"Yes" : L"No") << L"\n";
            std::wcout << L"  Friendly Name: " << monitor.FriendlyName << L"\n";
            std::wcout << L"  Device Name: " << monitor.DeviceName << L"\n";
            std::wcout << L"  Serial Number: " << monitor.SerialNumber << L"\n";
            std::wcout << L"  Key: " << monitor.Key << L"\n";
            std::wcout << L"  Position: " << monitor.RelativePosition << L"\n";
            std::wcout << L"  Monitor Rect: (" << monitor.MonitorRect.left << L", "
                << monitor.MonitorRect.top << L", " << monitor.MonitorRect.right
                << L", " << monitor.MonitorRect.bottom << L")\n";
            std::wcout << L"  Work Rect: (" << monitor.WorkRect.left << L", "
                << monitor.WorkRect.top << L", " << monitor.WorkRect.right
                << L", " << monitor.WorkRect.bottom << L")\n";
            std::wcout << L"\n";
        }

        // Example: Find a specific monitor
        if (!monitors.empty()) 
        {
            const RECT searchRect = monitors[0].MonitorRect;
            const int index = MonitorService::FindMonitorIndex(monitors, monitors[0].Key, searchRect);
            std::wcout << L"Successfully found monitor at index: " << index << L"\n";
        }

        // Example: Use custom display name format
        if (!monitors.empty())
        {
            const auto customDisplayName = monitors[0].GetDisplayName(L"{FriendlyName} - {SerialNumber}");
            std::wcout << L"Custom display name: " + customDisplayName;
        }

    }
    catch (const std::exception& e) 
    {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}

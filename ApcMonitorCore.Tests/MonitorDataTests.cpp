#include "pch.h"
#include "MonitorData.h"
#include "DisplayConfigData.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ApcMonitorCoreTests
{
    // Helper that constructs a MonitorData with all the fields set explicitly
    // (bypassing the constructor that calls Windows APIs indirectly).
    static MonitorData MakeMonitorData(
        const std::wstring& friendlyName = L"Acme Monitor",
        const std::wstring& serial = L"SN123",
        const std::wstring& position = L"primary",
        RECT monitorRect = { 0, 0, 1920, 1080 })
    {
        MONITORINFOEX info{};
        info.cbSize = sizeof(MONITORINFOEX);
        info.rcMonitor = monitorRect;
        info.rcWork = monitorRect;
        info.dwFlags = MONITORINFOF_PRIMARY;
        wcscpy_s(info.szDevice, L"\\\\.\\DISPLAY1");

        DisplayConfigData cfg{ 1u, friendlyName, L"\\\\?\\DISPLAY#MON001", L"\\\\.\\DISPLAY1" };

        MonitorData md{ info, cfg, serial };
        md.RelativePosition = position;
        return md;
    }

    TEST_CLASS(GetDisplayNameTests)
    {
    public:
        // -- Basic placeholder substitution --

        TEST_METHOD(DefaultFormat_ReturnsFriendlyNameAndPosition)
        {
            auto md = MakeMonitorData(L"Dell U2720Q", L"SN001", L"primary");
            Assert::AreEqual(std::wstring{ L"Dell U2720Q (primary)" }, md.GetDisplayName());
        }

        TEST_METHOD(FriendlyNamePlaceholder_Substituted)
        {
            auto md = MakeMonitorData(L"BenQ PD2720U", L"SN002", L"right");
            const auto result = md.GetDisplayName(L"{FriendlyName}");
            Assert::AreEqual(std::wstring{ L"BenQ PD2720U" }, result);
        }

        TEST_METHOD(PositionPlaceholder_Substituted)
        {
            auto md = MakeMonitorData(L"X", L"SN003", L"left");
            Assert::AreEqual(std::wstring{ L"left" }, md.GetDisplayName(L"{Position}"));
        }

        TEST_METHOD(SerialNumberPlaceholder_Substituted)
        {
            auto md = MakeMonitorData(L"X", L"MY-SERIAL", L"primary");
            Assert::AreEqual(std::wstring{ L"MY-SERIAL" }, md.GetDisplayName(L"{SerialNumber}"));
        }

        TEST_METHOD(KeyPlaceholder_Substituted)
        {
            auto md = MakeMonitorData(L"X", L"SN123", L"primary");
            Assert::AreEqual(std::wstring{ L"SERIAL:SN123" }, md.GetDisplayName(L"{Key}"));
        }

        // -- Rect and Size placeholders (Fix 5) --

        TEST_METHOD(RectPlaceholder_ExpandsToCoordinates)
        {
            auto md = MakeMonitorData(L"X", L"SN", L"primary", { 100, 200, 1920, 1080 });
            Assert::AreEqual(std::wstring{ L"100,200,1920,1080" }, md.GetDisplayName(L"{Rect}"));
        }

        TEST_METHOD(SizePlaceholder_ExpandsToWidthAndHeight)
        {
            auto md = MakeMonitorData(L"X", L"SN", L"primary", { 0, 0, 2560, 1440 });
            Assert::AreEqual(std::wstring{ L"2560x1440" }, md.GetDisplayName(L"{Size}"));
        }

        TEST_METHOD(SizePlaceholder_WithOffset_ComputedFromRect)
        {
            // Width = right - left, Height = bottom - top, even with non-zero origin
            auto md = MakeMonitorData(L"X", L"SN", L"right", { 1920, 0, 3840, 1080 });
            Assert::AreEqual(std::wstring{ L"1920x1080" }, md.GetDisplayName(L"{Size}"));
        }

        // -- Empty-value cleanup (Fix 5) --

        TEST_METHOD(EmptyPosition_NoDanglingParens)
        {
            auto md = MakeMonitorData(L"Dell U2720Q", L"SN", L"");
            // Default format: "{FriendlyName} ({Position})"
            // Position is empty → " ()" should be stripped
            Assert::AreEqual(std::wstring{ L"Dell U2720Q" }, md.GetDisplayName());
        }

        TEST_METHOD(EmptySerial_NoDanglingSquareBrackets)
        {
            auto md = MakeMonitorData(L"Dell U2720Q", L"", L"primary");
            const auto result = md.GetDisplayName(L"{FriendlyName} [{SerialNumber}]");
            Assert::AreEqual(std::wstring{ L"Dell U2720Q" }, result);
        }

        TEST_METHOD(EmptyField_NoDanglingAngleBrackets)
        {
            auto md = MakeMonitorData(L"Monitor", L"", L"primary");
            const auto result = md.GetDisplayName(L"{FriendlyName} <{SerialNumber}>");
            Assert::AreEqual(std::wstring{ L"Monitor" }, result);
        }

        // -- Trailing whitespace trim --

        TEST_METHOD(TrailingWhitespace_Trimmed)
        {
            auto md = MakeMonitorData(L"Monitor", L"", L"");
            // "{FriendlyName} " → after cleanup → "Monitor "  → trimmed → "Monitor"
            const auto result = md.GetDisplayName(L"{FriendlyName} ");
            Assert::AreEqual(std::wstring{ L"Monitor" }, result);
        }

        // -- Multiple placeholders --

        TEST_METHOD(MultiplePlaceholders_AllSubstituted)
        {
            auto md = MakeMonitorData(L"Acme", L"SN999", L"right");
            const auto result = md.GetDisplayName(L"{FriendlyName} | {Position} | {SerialNumber}");
            Assert::AreEqual(std::wstring{ L"Acme | right | SN999" }, result);
        }

        TEST_METHOD(CombinedRectAndSize)
        {
            auto md = MakeMonitorData(L"X", L"SN", L"primary", { 0, 0, 1920, 1080 });
            const auto result = md.GetDisplayName(L"{Size} at {Rect}");
            Assert::AreEqual(std::wstring{ L"1920x1080 at 0,0,1920,1080" }, result);
        }
    };
}

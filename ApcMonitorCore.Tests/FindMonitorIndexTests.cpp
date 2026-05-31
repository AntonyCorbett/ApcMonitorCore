#include "pch.h"
#include "MonitorService.h"
#include "MonitorData.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ApcMonitorCoreTests
{
    // Build a minimal MonitorData suitable for index-lookup tests.
    static MonitorData MakeEntry(const std::wstring& key, RECT rect)
    {
        MonitorData md{};
        md.Key = key;
        md.MonitorRect = rect;
        return md;
    }

    TEST_CLASS(FindMonitorIndexTests)
    {
    public:
        // -- Key-based lookup --

        TEST_METHOD(ExactKeyMatch_ReturnsCorrectIndex)
        {
            std::vector<MonitorData> monitors = {
                MakeEntry(L"SERIAL:AAA", { 0, 0, 1920, 1080 }),
                MakeEntry(L"SERIAL:BBB", { 1920, 0, 3840, 1080 }),
            };
            Assert::AreEqual(1, MonitorService::FindMonitorIndex(monitors, L"SERIAL:BBB", {}));
        }

        TEST_METHOD(KeyMatch_CaseInsensitive_ReturnsIndex)
        {
            std::vector<MonitorData> monitors = {
                MakeEntry(L"SERIAL:abc123", { 0, 0, 1920, 1080 }),
            };
            Assert::AreEqual(0, MonitorService::FindMonitorIndex(monitors, L"serial:ABC123", {}));
        }

        TEST_METHOD(FirstMonitor_ReturnsZero)
        {
            std::vector<MonitorData> monitors = {
                MakeEntry(L"SERIAL:FIRST", { 0, 0, 1920, 1080 }),
                MakeEntry(L"SERIAL:SECOND", { 1920, 0, 3840, 1080 }),
            };
            Assert::AreEqual(0, MonitorService::FindMonitorIndex(monitors, L"SERIAL:FIRST", {}));
        }

        TEST_METHOD(KeyNotFound_ReturnsMinus1)
        {
            std::vector<MonitorData> monitors = {
                MakeEntry(L"SERIAL:AAA", { 0, 0, 1920, 1080 }),
            };
            Assert::AreEqual(-1, MonitorService::FindMonitorIndex(monitors, L"SERIAL:ZZZ", {}));
        }

        // -- Rect-based fallback --

        TEST_METHOD(EmptyKey_FallsBackToRect)
        {
            std::vector<MonitorData> monitors = {
                MakeEntry(L"SERIAL:AAA", { 0, 0, 1920, 1080 }),
                MakeEntry(L"SERIAL:BBB", { 1920, 0, 3840, 1080 }),
            };
            RECT target = { 1920, 0, 3840, 1080 };
            Assert::AreEqual(1, MonitorService::FindMonitorIndex(monitors, L"", target));
        }

        TEST_METHOD(KeyMismatch_FallsBackToRect)
        {
            std::vector<MonitorData> monitors = {
                MakeEntry(L"SERIAL:AAA", { 0, 0, 1920, 1080 }),
            };
            RECT target = { 0, 0, 1920, 1080 };
            // Key doesn't match, rect does
            Assert::AreEqual(0, MonitorService::FindMonitorIndex(monitors, L"SERIAL:GONE", target));
        }

        TEST_METHOD(RectNotFound_ReturnsMinus1)
        {
            std::vector<MonitorData> monitors = {
                MakeEntry(L"SERIAL:AAA", { 0, 0, 1920, 1080 }),
            };
            RECT target = { 9999, 0, 11919, 1080 };
            Assert::AreEqual(-1, MonitorService::FindMonitorIndex(monitors, L"", target));
        }

        // -- Fix 3 regression: rect.right == 0 for left-of-primary monitors --

        TEST_METHOD(NegativeXMonitor_RightEdgeAtZero_IsFoundByRect)
        {
            // Secondary monitor fully to the left of primary.
            // MonitorRect = {-1920, 0, 0, 1080}: right == 0.
            // Old guard "rect.right > 0" would skip this; IsRectEmpty handles it correctly.
            std::vector<MonitorData> monitors = {
                MakeEntry(L"SERIAL:LEFT", { -1920, 0, 0, 1080 }),
            };
            RECT target = { -1920, 0, 0, 1080 };
            Assert::AreEqual(0, MonitorService::FindMonitorIndex(monitors, L"", target));
        }

        TEST_METHOD(NegativeXMonitor_FoundByKey_DoesNotNeedRect)
        {
            std::vector<MonitorData> monitors = {
                MakeEntry(L"SERIAL:LEFT", { -1920, 0, 0, 1080 }),
            };
            Assert::AreEqual(0, MonitorService::FindMonitorIndex(monitors, L"SERIAL:LEFT", {}));
        }

        // -- Edge cases --

        TEST_METHOD(EmptyList_ReturnsMinus1)
        {
            std::vector<MonitorData> monitors;
            Assert::AreEqual(-1, MonitorService::FindMonitorIndex(monitors, L"SERIAL:X", {}));
        }

        TEST_METHOD(BothKeyAndRectEmpty_ReturnsMinus1)
        {
            std::vector<MonitorData> monitors = {
                MakeEntry(L"SERIAL:AAA", { 0, 0, 1920, 1080 }),
            };
            Assert::AreEqual(-1, MonitorService::FindMonitorIndex(monitors, L"", {}));
        }

        TEST_METHOD(ZeroRect_IsRectEmpty_SkipsRectSearch)
        {
            // A default-constructed (all-zero) RECT is empty: left==right==0, top==bottom==0.
            // IsRectEmpty returns TRUE, so the rect fallback is skipped and -1 is returned.
            std::vector<MonitorData> monitors = {
                MakeEntry(L"SERIAL:AAA", { 0, 0, 0, 0 }),
            };
            RECT zeroRect = {};
            Assert::AreEqual(-1, MonitorService::FindMonitorIndex(monitors, L"", zeroRect));
        }
    };
}

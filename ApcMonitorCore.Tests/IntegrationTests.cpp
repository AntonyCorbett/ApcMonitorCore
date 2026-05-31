#include "pch.h"
#include "MonitorService.h"
#include "MonitorData.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ApcMonitorCoreTests
{
    // These tests run against the real display stack and require at least one
    // connected, active monitor. They validate end-to-end behaviour of
    // MonitorService::GetMonitorsData() on the test machine.
    TEST_CLASS(IntegrationTests)
    {
    public:
        TEST_METHOD(GetMonitorsData_ReturnsAtLeastOneMonitor)
        {
            MonitorService svc;
            const auto monitors = svc.GetMonitorsData();
            Assert::IsTrue(monitors.size() >= 1, L"Expected at least one connected monitor");
        }

        TEST_METHOD(GetMonitorsData_ExactlyOnePrimaryMonitor)
        {
            MonitorService svc;
            const auto monitors = svc.GetMonitorsData();
            const auto primaryCount = std::ranges::count_if(monitors,
                [](const MonitorData& m) { return m.IsPrimary; });
            Assert::AreEqual(1LL, primaryCount, L"Expected exactly one primary monitor");
        }

        TEST_METHOD(GetMonitorsData_AllMonitorsHaveNonEmptyKey)
        {
            MonitorService svc;
            const auto monitors = svc.GetMonitorsData();
            for (const auto& m : monitors)
            {
                Assert::IsFalse(m.Key.empty(),
                    (L"Monitor with device name " + m.DeviceName + L" has an empty Key").c_str());
            }
        }

        TEST_METHOD(GetMonitorsData_AllMonitorsHaveNonEmptyDeviceName)
        {
            MonitorService svc;
            const auto monitors = svc.GetMonitorsData();
            for (const auto& m : monitors)
            {
                Assert::IsFalse(m.DeviceName.empty(), L"Expected all monitors to have a GDI device name");
            }
        }

        TEST_METHOD(GetMonitorsData_AllMonitorsHaveNonEmptyPosition)
        {
            MonitorService svc;
            const auto monitors = svc.GetMonitorsData();
            for (const auto& m : monitors)
            {
                Assert::IsFalse(m.RelativePosition.empty(),
                    (L"Monitor " + m.DeviceName + L" has empty RelativePosition").c_str());
            }
        }

        TEST_METHOD(GetMonitorsData_PrimaryMonitorAtOrigin)
        {
            MonitorService svc;
            const auto monitors = svc.GetMonitorsData();
            for (const auto& m : monitors)
            {
                if (m.IsPrimary)
                {
                    Assert::AreEqual(0L, m.MonitorRect.left, L"Primary monitor left must be 0");
                    Assert::AreEqual(0L, m.MonitorRect.top, L"Primary monitor top must be 0");
                    break;
                }
            }
        }

        TEST_METHOD(GetMonitorsData_PrimaryMonitorPositionIsPrimary)
        {
            MonitorService svc;
            const auto monitors = svc.GetMonitorsData();
            for (const auto& m : monitors)
            {
                if (m.IsPrimary)
                {
                    Assert::AreEqual(std::wstring{ L"primary" }, m.RelativePosition);
                    break;
                }
            }
        }

        TEST_METHOD(GetMonitorsData_AllMonitorRectsNonEmpty)
        {
            MonitorService svc;
            const auto monitors = svc.GetMonitorsData();
            for (const auto& m : monitors)
            {
                Assert::IsFalse(IsRectEmpty(&m.MonitorRect) == TRUE,
                    (L"Monitor " + m.DeviceName + L" has an empty MonitorRect").c_str());
            }
        }

        TEST_METHOD(GetMonitorsData_FindMonitorIndex_FindsEachMonitorByKey)
        {
            MonitorService svc;
            const auto monitors = svc.GetMonitorsData();
            for (int i = 0; i < static_cast<int>(monitors.size()); ++i)
            {
                const int found = MonitorService::FindMonitorIndex(monitors, monitors[i].Key, {});
                Assert::AreEqual(i, found,
                    (L"FindMonitorIndex could not find monitor " + monitors[i].Key).c_str());
            }
        }

        TEST_METHOD(GetMonitorsData_FindMonitorIndex_FindsEachMonitorByRect)
        {
            MonitorService svc;
            const auto monitors = svc.GetMonitorsData();
            for (int i = 0; i < static_cast<int>(monitors.size()); ++i)
            {
                const int found = MonitorService::FindMonitorIndex(monitors, L"", monitors[i].MonitorRect);
                Assert::AreEqual(i, found,
                    (L"FindMonitorIndex by rect failed for " + monitors[i].DeviceName).c_str());
            }
        }

        TEST_METHOD(GetMonitorsData_KeyPrefixIsSerialOrPath)
        {
            MonitorService svc;
            const auto monitors = svc.GetMonitorsData();
            for (const auto& m : monitors)
            {
                const bool hasSeriPrefix = m.Key.rfind(L"SERIAL:", 0) == 0;
                const bool hasPathPrefix = m.Key.rfind(L"PATH:", 0) == 0;
                Assert::IsTrue(hasSeriPrefix || hasPathPrefix,
                    (L"Key '" + m.Key + L"' has unexpected prefix").c_str());
            }
        }
    };
}

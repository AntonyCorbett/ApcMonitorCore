#include "pch.h"
#include "ManagedMonitorService.h"
#include <msclr/marshal_cppstd.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib") 
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "SetupAPI.lib")

using namespace System;
using namespace System::Collections::Generic;
using namespace msclr::interop;

namespace ApcMonitorCore {
    namespace Managed {

        // Helper function to convert native MonitorData to managed
        ManagedMonitorData^ ConvertToManaged(const ::MonitorData& native) {
            auto managed = gcnew ManagedMonitorData();
            managed->Id = native.Id;
            managed->IsPrimary = native.IsPrimary;
            managed->MonitorRect = System::Drawing::Rectangle(
                native.MonitorRect.left,
                native.MonitorRect.top,
                native.MonitorRect.right - native.MonitorRect.left,
                native.MonitorRect.bottom - native.MonitorRect.top
            );
            managed->WorkRect = System::Drawing::Rectangle(
                native.WorkRect.left,
                native.WorkRect.top,
                native.WorkRect.right - native.WorkRect.left,
                native.WorkRect.bottom - native.WorkRect.top
            );
            managed->FriendlyName = marshal_as<String^>(native.FriendlyName);
            managed->DevicePath = marshal_as<String^>(native.DevicePath);
            managed->DeviceName = marshal_as<String^>(native.DeviceName);
            managed->SerialNumber = marshal_as<String^>(native.SerialNumber);
            managed->Key = marshal_as<String^>(native.Key);
            managed->RelativePosition = marshal_as<String^>(native.RelativePosition);
            return managed;
        }

        ManagedMonitorService::ManagedMonitorService() {
            m_pNative = new MonitorService();
        }

        ManagedMonitorService::~ManagedMonitorService() {
            this->!ManagedMonitorService();
        }

        ManagedMonitorService::!ManagedMonitorService() {
            delete m_pNative;
            m_pNative = nullptr;
        }

        List<ManagedMonitorData^>^ ManagedMonitorService::GetMonitorsData() {
            try {
                auto nativeData = m_pNative->GetMonitorsData();
                auto managedList = gcnew List<ManagedMonitorData^>();

                for (const auto& monitor : nativeData) {
                    managedList->Add(ConvertToManaged(monitor));
                }

                return managedList;
            }
            catch (const std::exception& e) {
                String^ message = marshal_as<String^>(e.what());
                throw gcnew System::Exception(message);
            }
        }

        int ManagedMonitorService::FindMonitorIndex(List<ManagedMonitorData^>^ monitors, String^ key, System::Drawing::Rectangle rect) {
            // Convert managed data back to native for the call
            std::vector<::MonitorData> nativeMonitors;
            std::wstring nativeKey = key ? marshal_as<std::wstring>(key) : L"";

            RECT nativeRect = {
                rect.Left,
                rect.Top,
                rect.Left + rect.Width,  // Fixed: was rect.Right
                rect.Top + rect.Height   // Fixed: was rect.Bottom
            };

            // Convert managed monitors to native (simplified conversion for the search)
            for each (auto monitor in monitors) {
                ::MonitorData native{};
                native.Key = marshal_as<std::wstring>(monitor->Key);
                native.MonitorRect = {
                    monitor->MonitorRect.Left,
                    monitor->MonitorRect.Top,
                    monitor->MonitorRect.Left + monitor->MonitorRect.Width,  // Fixed
                    monitor->MonitorRect.Top + monitor->MonitorRect.Height   // Fixed
                };
                nativeMonitors.push_back(native);
            }

            return MonitorService::FindMonitorIndex(nativeMonitors, nativeKey, nativeRect);
        }

        String^ ManagedMonitorData::GetDisplayName(String^ format) {
            if (String::IsNullOrEmpty(format)) {
                format = "{FriendlyName} ({Position})";
            }

            String^ result = format;
            result = result->Replace("{FriendlyName}", FriendlyName != nullptr ? FriendlyName : "");
            result = result->Replace("{Position}",     RelativePosition != nullptr ? RelativePosition : "");
            result = result->Replace("{SerialNumber}", SerialNumber != nullptr ? SerialNumber : "");
            result = result->Replace("{Key}",          Key != nullptr ? Key : "");
            result = result->Replace("{DeviceName}",   DeviceName != nullptr ? DeviceName : "");

            if (result->Contains("{Rect}")) {
                auto r = MonitorRect;
                result = result->Replace("{Rect}",
                    String::Format("{0},{1},{2},{3}", r.Left, r.Top, r.Right, r.Bottom));
            }

            if (result->Contains("{Size}")) {
                result = result->Replace("{Size}",
                    String::Format("{0}x{1}", MonitorRect.Width, MonitorRect.Height));
            }

            result = result->Replace(" ()", "")->Replace(" []", "")->Replace(" <>", "");

            return result->TrimEnd();
        }
    }
}
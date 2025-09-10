#pragma once
#include "../ApcMonitorCore/MonitorService.h"
#include "../ApcMonitorCore/MonitorData.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Runtime::InteropServices;

namespace ApcMonitorCore {
    namespace Managed {

        // Managed equivalent of MonitorData
        public ref class ManagedMonitorData {
        public:
            property int Id;
            property bool IsPrimary;
            property System::Drawing::Rectangle MonitorRect;
            property System::Drawing::Rectangle WorkRect;
            property String^ FriendlyName;
            property String^ DevicePath;
            property String^ DeviceName;
            property String^ SerialNumber;
            property String^ Key;
            property String^ RelativePosition;

            property String^ DisplayName {
                String^ get() { return GetDisplayName(L"{FriendlyName} ({Position})"); }
            }

            String^ GetDisplayName(String^ format);
        };

        public ref class ManagedMonitorService {
        private:
            MonitorService* m_pNative;

        public:
            ManagedMonitorService();
            ~ManagedMonitorService();
            !ManagedMonitorService();

            List<ManagedMonitorData^>^ GetMonitorsData();
            static int FindMonitorIndex(List<ManagedMonitorData^>^ monitors, String^ key, System::Drawing::Rectangle rect);
        };
    }
}
#pragma once
#include <windows.h>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "MonitorData.h"
#include "DisplayConfigData.h"

class MonitorService
{
public:
	[[nodiscard]] std::vector<MonitorData> GetMonitorsData() const;
	[[nodiscard]] static int FindMonitorIndex(const std::vector<MonitorData>& monitors, const std::wstring& key, const RECT& rect);

private:
	mutable std::mutex m_cacheMutex;
	mutable std::unordered_map<std::wstring, std::wstring> m_serialCache;

	[[nodiscard]] std::wstring TryGetMonitorSerialFromDevicePath(const std::wstring& devicePath) const;
	[[nodiscard]] static std::vector<MONITORINFOEX> GetMonitorsInfo();
	[[nodiscard]] static std::vector<DisplayConfigData> GetDisplayConfigInfo();
};
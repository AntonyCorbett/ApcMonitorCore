#pragma once
#include <windows.h>
#include <string>

struct DisplayConfigData
{
	DisplayConfigData()
		: Id(0)
	{
	}

	DisplayConfigData(const UINT id, const std::wstring& friendlyName, const std::wstring& devicePath, const std::wstring& gdiDeviceName)
		: Id(id)
		, FriendlyName(friendlyName)
		, DevicePath(devicePath)
		, GdiDeviceName(gdiDeviceName)
	{
	}

	UINT Id;
	std::wstring FriendlyName;
	std::wstring DevicePath;
	std::wstring GdiDeviceName; // GDI source device name, e.g. "\\.\DISPLAY1"
};


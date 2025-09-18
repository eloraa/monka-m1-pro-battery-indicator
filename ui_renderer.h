#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include <map>
#include "svg_renderer.h"
#include "mouse_item.h"
#include "mouse_list.h"
#include "settings_view.h"
#include "colors.h"
#include "device_discovery.h"

#pragma comment(lib, "gdiplus.lib")

enum class BatteryLevel {
    Empty,
    Low,
    Medium,
    Full,
    Charging,
    Critical,
    Updating,    
    Offline      
};

class UIRenderer {
private:
    static ULONG_PTR gdiplusToken;
    static bool gdiplusInitialized;
    
    static Gdiplus::Font* interFont;
    static Gdiplus::Font* geistMonoFont;
    
    static std::vector<MouseItem> mouseList;

    static bool animationTimerRunning;

    static int hoveredItemIndex;
    static DWORD lastHoverUpdate;
    static bool isTrackingMouse;

    static DWORD lastBatteryUpdate;
    static DWORD lastDeviceDiscovery;
    static std::map<std::string, DWORD> deviceLastResponse;  
    static std::vector<std::string> activeDevicePaths;   

    static SettingsView settingsView;
    static SettingsView& GetSettingsView() { return settingsView; }

    static HWND mainWindowHandle;

    static bool InitializeMouseList();
    
public:
    static bool Initialize();

    static void SetMainWindow(HWND hWnd);

    static void Cleanup();
    
    static void RenderUI(HDC hdc, const RECT& clientRect);
    
    static void UpdateMouseHover(HWND hWnd, int mouseX, int mouseY, int width, int height);
    static void ClearMouseHover(HWND hWnd);
    static bool UpdateHoverAnimation(HWND hWnd);

    static void StartAnimationTimer(HWND hWnd);
    static void StopAnimationTimer(HWND hWnd);

    static int HandleMouseClick(HWND hWnd, int mouseX, int mouseY, int width, int height);

    static void ShowDeviceSettings(const MouseItem* device);
    static bool IsSettingsViewVisible();
    static bool HandleSettingsClick(int mouseX, int mouseY, int width, int height);
    static void HandleSettingsHover(int mouseX, int mouseY, int width, int height);

    static void UpdateBatteryStatus(HWND hWnd);
    static void RefreshDeviceList(HWND hWnd);
    static void CheckDeviceHealth(HWND hWnd);
    static void PerformDeviceDiscovery(HWND hWnd);
    static void SwitchToAvailableDevice(HWND hWnd);
    static void UpdateDeviceResponseTime(const std::string& devicePath);

    static void OnBatteryDataReceived(const std::string& devicePath, const BatteryStatus& status);

    static void OnDeviceChange();

    static void UpdateSystemTrayIcon();
    
    static void UpdateThemeFromSystem();
    
    static bool LoadFonts();
    
    static Gdiplus::Font* GetInterFont();
    static Gdiplus::Font* GetGeistMonoFont();

    static const std::vector<MouseItem>& GetMouseList();
    
    static void RenderMockDataIndicator(Gdiplus::Graphics* g, int width, int height);
};

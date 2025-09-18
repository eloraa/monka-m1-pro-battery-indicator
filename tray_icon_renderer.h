#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <string>
#include "svg_renderer.h"
#include "colors.h"

#pragma comment(lib, "gdiplus.lib")

class TrayIconRenderer {
public:
    static HICON CreateBatteryIcon(int batteryLevel, bool isCharging, bool isOnline, bool isUpdating);

private:
    static Gdiplus::Bitmap* RenderMouseSVG(int iconSize, bool isDarkTheme);

    static Gdiplus::Bitmap* CreateBatteryLevelMask(int width, int height, int batteryLevel);

    static Gdiplus::Bitmap* CreateColoredMouseMask(Gdiplus::Bitmap* mouseMask, int iconSize, 
                                                   int batteryLevel, bool isOnline, bool isUpdating);

    static Gdiplus::Color GetBatteryColor(int batteryLevel);

    static void RenderBatteryText(Gdiplus::Graphics* g, int batteryLevel, bool isUpdating,
                                 int iconSize, bool isDarkTheme);

    static void ApplyDimmingEffect(Gdiplus::Graphics* g, int iconSize);

    static HICON BitmapToHIcon(Gdiplus::Bitmap* bitmap);

    static Gdiplus::Bitmap* LoadSVGAsBitmap(const std::string& svgPath, int width, int height,
                                           Gdiplus::Color fillColor);

    static Gdiplus::Bitmap* LoadPNGAsBitmap(const std::string& pngPath);

    static bool GetChargingBlinkState();

private:
    static DWORD s_lastBlinkTime;
    static bool s_blinkState;
};
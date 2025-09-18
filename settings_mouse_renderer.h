#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <string>
#include "colors.h"

#pragma comment(lib, "gdiplus.lib")

class SettingsMouseRenderer {
public:
    static void RenderMouseImage(Gdiplus::Graphics* g, int x, int y, int size, bool isColored, bool isDarkTheme);

private:
    static Gdiplus::Bitmap* LoadPNGAsBitmap(const std::string& pngPath);
    
    static Gdiplus::Bitmap* CreateColoredMouseMask(Gdiplus::Bitmap* mouseMask, int iconSize, 
                                                   int batteryLevel, bool isOnline, bool isUpdating, 
                                                   Gdiplus::Color customColor = Gdiplus::Color(0, 0, 0, 0));
    
    static Gdiplus::Color GetBatteryColor(int batteryLevel);
};

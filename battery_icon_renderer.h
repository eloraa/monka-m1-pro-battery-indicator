#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <string>
#include "svg_renderer.h"
#include "colors.h"

class BatteryIconRenderer {
public:
    static void RenderBatteryIcon(Gdiplus::Graphics* g, int x, int y, int size, int batteryLevel, bool isCharging, Gdiplus::Color color);
    
    static std::string GetBatteryIconPath(int batteryLevel, bool isCharging);
    
    static Gdiplus::Color GetBatteryColor(int batteryLevel);
};

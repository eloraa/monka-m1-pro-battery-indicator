#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include "mouse_item.h"

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

class MouseListRenderer {
public:
    static void RenderMouseList(Graphics* g, const std::vector<MouseItem>& mice, int x, int y, int containerWidth, int containerHeight);
    
    static void AddMouse(std::vector<MouseItem>& mice, const MouseItem& mouse);
    
    static void RemoveMouse(std::vector<MouseItem>& mice, int index);
    
    static void UpdateMouseBattery(std::vector<MouseItem>& mice, int index, int batteryLevel, bool isCharging = false);
    
private:
    static void CalculateLayout(const std::vector<MouseItem>& mice, int containerWidth, int containerHeight, 
                               int& itemsPerRow, int& itemWidth, int& itemHeight, int& spacingX, int& spacingY);
};

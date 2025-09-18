#include "mouse_list.h"

using namespace Gdiplus;

void MouseListRenderer::RenderMouseList(Graphics* g, const std::vector<MouseItem>& mice, int x, int y, int containerWidth, int containerHeight) {
    if (mice.empty()) return;
    
    int itemsPerRow, itemWidth, itemHeight, spacingX, spacingY;
    CalculateLayout(mice, containerWidth, containerHeight, itemsPerRow, itemWidth, itemHeight, spacingX, spacingY);
    
    for (size_t i = 0; i < mice.size(); i++) {
        int row = i / itemsPerRow;
        int col = i % itemsPerRow;
        
        int itemX = x + col * (itemWidth + spacingX);
        int itemY = y + row * (itemHeight + spacingY);
        
        MouseItemRenderer::RenderMouseItem(g, mice[i], itemX, itemY, itemWidth, itemHeight);
    }
}

void MouseListRenderer::AddMouse(std::vector<MouseItem>& mice, const MouseItem& mouse) {
    mice.push_back(mouse);
}

void MouseListRenderer::RemoveMouse(std::vector<MouseItem>& mice, int index) {
    if (index >= 0 && index < static_cast<int>(mice.size())) {
        mice.erase(mice.begin() + index);
    }
}

void MouseListRenderer::UpdateMouseBattery(std::vector<MouseItem>& mice, int index, int batteryLevel, bool isCharging) {
    if (index >= 0 && index < static_cast<int>(mice.size())) {
        mice[index].batteryLevel = batteryLevel;
        mice[index].isCharging = isCharging;
    }
}

void MouseListRenderer::CalculateLayout(const std::vector<MouseItem>& mice, int containerWidth, int containerHeight, 
                                       int& itemsPerRow, int& itemWidth, int& itemHeight, int& spacingX, int& spacingY) {
    itemsPerRow = 2;
    
    spacingX = 20;
    spacingY = 20;
    
    itemWidth = (containerWidth - spacingX) / 2;
    itemHeight = 220;
}

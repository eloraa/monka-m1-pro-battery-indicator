#include "battery_icon_renderer.h"
#include "resource_loader.h"

void BatteryIconRenderer::RenderBatteryIcon(Gdiplus::Graphics* g, int x, int y, int size, int batteryLevel, bool isCharging, Gdiplus::Color color) {
    std::string svgPath = GetBatteryIconPath(batteryLevel, isCharging);
    std::string svgContent = ResourceLoader::LoadSVGAsString(svgPath);
    
    if (!svgContent.empty()) {
        SVGRenderer::RenderSVGFromString(g, svgContent, x, y, size, size, color);
    }
}

std::string BatteryIconRenderer::GetBatteryIconPath(int batteryLevel, bool isCharging) {
    if (isCharging) {
        return "assets/svgs/battery_icons/battery_icon_charging.svg";
    } else if (batteryLevel <= 0) {
        return "assets/svgs/battery_icons/battery_icon_empty.svg";
    } else if (batteryLevel <= 15) {
        return "assets/svgs/battery_icons/battery_icon_critical.svg";
    } else if (batteryLevel <= 25) {
        return "assets/svgs/battery_icons/battery_icon_low.svg";
    } else if (batteryLevel <= 75) {
        return "assets/svgs/battery_icons/battery_icon_medium.svg";
    } else {
        return "assets/svgs/battery_icons/battery_icon_full.svg";
    }
}

Gdiplus::Color BatteryIconRenderer::GetBatteryColor(int batteryLevel) {
    return Colors::GetBatteryColor(batteryLevel);
}

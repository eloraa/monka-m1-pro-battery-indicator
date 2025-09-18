#pragma once

#include <windows.h>
#include <gdiplus.h>

class ColorPickerDialog;

enum class ThemeMode {
    DARK,
    LIGHT
};

namespace Colors {
    const Gdiplus::Color PRIMARY(255, 61, 95, 230);     // rgb(61 95 230)
    const Gdiplus::Color SUCCESS(255, 139, 195, 74);    // rgb(139 195 74)
    const Gdiplus::Color WARNING(255, 219, 230, 61);    // rgb(219 230 61)
    const Gdiplus::Color CRITICAL(255, 232, 59, 59);    // rgb(232 59 59)

    namespace Dark {
        const Gdiplus::Color BACKGROUND(255, 31, 31, 31);       // Dark background
        const Gdiplus::Color FOREGROUND(255, 255, 255, 255);    // White foreground/text
        const Gdiplus::Color TEXT_SECONDARY(255, 180, 180, 180); // Light gray text
        const Gdiplus::Color HOVER_OVERLAY(40, 255, 255, 255);   // Semi-transparent white
        const Gdiplus::Color BORDER_DEFAULT(255, 60, 60, 60);    // Dark border
        const Gdiplus::Color BORDER_HOVER(255, 100, 100, 100);   // Lighter border on hover
        const Gdiplus::Color CARD_BACKGROUND(255, 45, 45, 45);   // Card background
    }

    namespace Light {
        const Gdiplus::Color BACKGROUND(255, 255, 255, 255);     // Light background
        const Gdiplus::Color FOREGROUND(255, 31, 31, 31);        // Dark foreground/text
        const Gdiplus::Color TEXT_SECONDARY(255, 120, 120, 120); // Dark gray text
        const Gdiplus::Color HOVER_OVERLAY(40, 0, 0, 0);         // Semi-transparent black
        const Gdiplus::Color BORDER_DEFAULT(255, 200, 200, 200); // Light border
        const Gdiplus::Color BORDER_HOVER(255, 150, 150, 150);   // Darker border on hover
        const Gdiplus::Color CARD_BACKGROUND(255, 255, 255, 255); // White card background
    }

    extern ThemeMode currentTheme;

    inline Gdiplus::Color GetBackground() {
        return (currentTheme == ThemeMode::DARK) ? Dark::BACKGROUND : Light::BACKGROUND;
    }

    inline Gdiplus::Color GetForeground() {
        return (currentTheme == ThemeMode::DARK) ? Dark::FOREGROUND : Light::FOREGROUND;
    }

    inline Gdiplus::Color GetTextSecondary() {
        return (currentTheme == ThemeMode::DARK) ? Dark::TEXT_SECONDARY : Light::TEXT_SECONDARY;
    }

    inline Gdiplus::Color GetHoverOverlay() {
        return (currentTheme == ThemeMode::DARK) ? Dark::HOVER_OVERLAY : Light::HOVER_OVERLAY;
    }

    inline Gdiplus::Color GetBorderDefault() {
        return (currentTheme == ThemeMode::DARK) ? Dark::BORDER_DEFAULT : Light::BORDER_DEFAULT;
    }

    inline Gdiplus::Color GetBorderHover() {
        return (currentTheme == ThemeMode::DARK) ? Dark::BORDER_HOVER : Light::BORDER_HOVER;
    }

    inline Gdiplus::Color GetCardBackground() {
        return (currentTheme == ThemeMode::DARK) ? Dark::CARD_BACKGROUND : Light::CARD_BACKGROUND;
    }

    Gdiplus::Color GetBatteryColor(int batteryLevel);

    inline COLORREF ToColorRef(const Gdiplus::Color& color) {
        return RGB(color.GetR(), color.GetG(), color.GetB());
    }

    inline void SetTheme(ThemeMode theme) {
        currentTheme = theme;
    }

    inline ThemeMode GetCurrentTheme() {
        return currentTheme;
    }
}
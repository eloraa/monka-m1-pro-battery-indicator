#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <commctrl.h>
#include "mouse_item.h"
#include "colors.h"

struct DeviceSettings {
    bool isThemeColored;        // true = COLORED, false = FLAT
    int selectedColor;          // 0 = PRIMARY, 1 = WARNING, 2 = CRITICAL
    ThemeMode themeMode;        // DARK | LIGHT
};

class SettingsView {
private:
    const MouseItem* deviceInfo;
    DeviceSettings settings;
    bool isVisible;
    HWND parentHwnd;

    bool showingColorDropdown;
    int activeColorIndex;
    int dropdownX, dropdownY;
    int hoveredDropdownItem;

    static bool s_iconModeColored;

    int hoveredButton;
    int selectedTheme;      // 0 = FLAT, 1 = COLORED
    int selectedColor;      // 0 = PRIMARY, 1 = WARNING, 2 = CRITICAL
    int selectedMode;       // 0 = DARK, 1 = LIGHT

    static const int BACK_BUTTON_WIDTH = 80;
    static const int BACK_BUTTON_HEIGHT = 30;
    static const int SECTION_SPACING = 40;
    static const int BUTTON_HEIGHT = 35;
    static const int BUTTON_SPACING = 15;

public:
    SettingsView();
    ~SettingsView();

    void Show(const MouseItem* device);
    void Hide();
    bool IsVisible() const { return isVisible; }
    void SetParentWindow(HWND hwnd) { parentHwnd = hwnd; }

    void Render(Gdiplus::Graphics* g, int width, int height);

    bool HandleClick(int mouseX, int mouseY, int width, int height);
    void HandleHover(int mouseX, int mouseY, int width, int height);

    const DeviceSettings& GetSettings() const { return settings; }
    void ApplySettings();

    static void LoadUISettings();
    static void SaveUISettings();
    static bool IsIconModeColored();
    static void SetIconModeColored(bool colored);

private:
    void RenderDeviceInfoSection(Gdiplus::Graphics* g, int startY, int width);
    void RenderThemeSection(Gdiplus::Graphics* g, int startY, int width);
    void RenderColorsSection(Gdiplus::Graphics* g, int startY, int width);

    void RenderButton(Gdiplus::Graphics* g, const std::wstring& text, int x, int y, int width, int height,
                     bool isSelected, bool isHovered, const Gdiplus::Color& accentColor);

    bool IsPointInRect(int x, int y, int rectX, int rectY, int rectW, int rectH);
    int GetHoveredElement(int mouseX, int mouseY, int width, int height);

    void ShowColorDropdown(int colorIndex, int x, int y);
    void HideColorDropdown();
    void HandleDropdownSelection(int selection);
    void RenderColorDropdown(Gdiplus::Graphics* g);
    
    void RenderMockDataIndicator(Gdiplus::Graphics* g, int width);


};
#pragma once

#include <windows.h>
#include <commdlg.h>
#include <gdiplus.h>
#include "colors.h"

struct ColorSettings {
    bool useCustomColors[4];        
    Gdiplus::Color customColors[4]; 

    ColorSettings() {
        for (int i = 0; i < 4; i++) {
            useCustomColors[i] = false;
        }
        customColors[0] = Colors::PRIMARY;
        customColors[1] = Colors::SUCCESS;
        customColors[2] = Colors::WARNING;
        customColors[3] = Colors::CRITICAL;
    }
};

class ColorPickerDialog {
private:
    HWND parentHwnd;
    static ColorSettings s_colorSettings;

    static COLORREF customColors[16];

    static int currentColorIndex;
    static Gdiplus::Color tempColor;

    static COLORREF GdiplusColorToColorRef(const Gdiplus::Color& color);
    static Gdiplus::Color ColorRefToGdiplusColor(COLORREF colorRef);

    static INT_PTR CALLBACK ColorPickerDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

public:
    ColorPickerDialog(HWND parent);
    ~ColorPickerDialog();

    // colorIndex: 0=PRIMARY, 1=SUCCESS, 2=WARNING, 3=CRITICAL
    bool ShowColorPicker(int colorIndex);

    static Gdiplus::Color GetColor(int colorIndex);

    static void ResetColorToDefault(int colorIndex);

    static void ResetAllColorsToDefault();

    static bool IsColorCustomized(int colorIndex);

    static void LoadColorSettings();
    static void SaveColorSettings();

    static const ColorSettings& GetColorSettings() { return s_colorSettings; }
    static void SetColorSettings(const ColorSettings& settings) { s_colorSettings = settings; }
};
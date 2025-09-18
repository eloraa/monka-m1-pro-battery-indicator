#include "settings_mouse_renderer.h"
#include "resource_loader.h"

using namespace Gdiplus;

void SettingsMouseRenderer::RenderMouseImage(Graphics* g, int x, int y, int size, bool isColored, bool isDarkTheme) {
    int batteryLevel = 60; 
    bool shouldSwapOverlays = batteryLevel > 90 && !isColored; 
    
    std::string mouseImagePath;
    if (shouldSwapOverlays) {
        mouseImagePath = isDarkTheme ?
            "assets/pngs/ui/mouse_light.png" :
            "assets/pngs/ui/mouse_dark.png";
    } else {
        mouseImagePath = isDarkTheme ?
            "assets/pngs/ui/mouse_dark.png" :
            "assets/pngs/ui/mouse_light.png";
    }

    Bitmap* mousePNG = LoadPNGAsBitmap(mouseImagePath);
    Bitmap* mouseMask = LoadPNGAsBitmap("assets/pngs/ui/mouse_mask.png");

    if (mousePNG && mouseMask) {
        if (isColored) {
            Bitmap* coloredMask = CreateColoredMouseMask(mouseMask, size, 60, true, false);
            if (coloredMask) {
                g->DrawImage(coloredMask, x, y, size, size);
                delete coloredMask;
            }
        } else {
            Color maskColor = isDarkTheme ? Color(255, 255, 255, 255) : Color(255, 0, 0, 0);
            Bitmap* flatMask = CreateColoredMouseMask(mouseMask, size, 60, true, false, maskColor);
            if (flatMask) {
                g->DrawImage(flatMask, x, y, size, size);
                delete flatMask;
            }
        }

        g->DrawImage(mousePNG, x, y, size, size);
        
        delete mousePNG;
        delete mouseMask;
    } else if (mousePNG) {
        g->DrawImage(mousePNG, x, y, size, size);
        delete mousePNG;
    } else {
        Pen mousePen(Color(255, 255, 255, 255), 2.0f);
        g->DrawEllipse(&mousePen, x + size/4, y + size/4, size/2, size/2);
    }
}

Bitmap* SettingsMouseRenderer::LoadPNGAsBitmap(const std::string& pngPath) {
    return ResourceLoader::LoadPNGAsBitmap(pngPath);
}

Bitmap* SettingsMouseRenderer::CreateColoredMouseMask(Bitmap* mouseMask, int iconSize,
                                                int batteryLevel, bool isOnline, bool isUpdating, 
                                                Color customColor) {
    if (!mouseMask) return nullptr;

    Color fillColor;
    if (customColor.GetA() != 0) {
        fillColor = customColor;
    } else if (!isOnline || isUpdating) {
        fillColor = Color(255, 128, 128, 128);
    } else {
        fillColor = GetBatteryColor(batteryLevel);
    }

    Bitmap* result = new Bitmap(iconSize, iconSize, PixelFormat32bppARGB);

    BitmapData maskData;
    Rect lockRect(0, 0, mouseMask->GetWidth(), mouseMask->GetHeight());
    mouseMask->LockBits(&lockRect, ImageLockModeRead, PixelFormat32bppARGB, &maskData);

    BitmapData resultData;
    Rect resultRect(0, 0, iconSize, iconSize);
    result->LockBits(&resultRect, ImageLockModeWrite, PixelFormat32bppARGB, &resultData);

    int fillHeight = (iconSize * batteryLevel) / 100;
    int fillY = iconSize - fillHeight;

    BYTE* maskPixels = (BYTE*)maskData.Scan0;
    BYTE* resultPixels = (BYTE*)resultData.Scan0;

    for (int y = 0; y < iconSize; y++) {
        for (int x = 0; x < iconSize; x++) {
            int srcX = (x * mouseMask->GetWidth()) / iconSize;
            int srcY = (y * mouseMask->GetHeight()) / iconSize;

            int maskIndex = (srcY * maskData.Stride) + (srcX * 4);
            BYTE maskAlpha = maskPixels[maskIndex + 3];
            BYTE maskRed = maskPixels[maskIndex + 2];
            BYTE maskGreen = maskPixels[maskIndex + 1];
            BYTE maskBlue = maskPixels[maskIndex];

            int resultIndex = (y * resultData.Stride) + (x * 4);

            if (maskRed > 200 && maskGreen > 200 && maskBlue > 200 && y >= fillY) {
                resultPixels[resultIndex] = fillColor.GetB();     // Blue
                resultPixels[resultIndex + 1] = fillColor.GetG(); // Green
                resultPixels[resultIndex + 2] = fillColor.GetR(); // Red
                resultPixels[resultIndex + 3] = fillColor.GetA(); // Alpha
            } else {
                resultPixels[resultIndex] = 0;     // Blue
                resultPixels[resultIndex + 1] = 0; // Green
                resultPixels[resultIndex + 2] = 0; // Red
                resultPixels[resultIndex + 3] = 0; // Alpha
            }
        }
    }

    mouseMask->UnlockBits(&maskData);
    result->UnlockBits(&resultData);

    return result;
}

Color SettingsMouseRenderer::GetBatteryColor(int batteryLevel) {
    return Colors::GetBatteryColor(batteryLevel);
}
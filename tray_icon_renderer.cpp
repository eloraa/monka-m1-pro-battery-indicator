#include "tray_icon_renderer.h"
#include "font_loader.h"
#include "settings_view.h"
#include "resource_loader.h"

using namespace Gdiplus;

DWORD TrayIconRenderer::s_lastBlinkTime = 0;
bool TrayIconRenderer::s_blinkState = true;

HICON TrayIconRenderer::CreateBatteryIcon(int batteryLevel, bool isCharging, bool isOnline, bool isUpdating) {
    const int iconSize = 64;

    Bitmap* iconBitmap = new Bitmap(iconSize, iconSize, PixelFormat32bppARGB);
    Graphics* g = Graphics::FromImage(iconBitmap);

    g->SetSmoothingMode(SmoothingModeAntiAlias);
    g->SetTextRenderingHint(TextRenderingHintAntiAlias);

    g->Clear(Color(0, 0, 0, 0)); // TRANSPARENT

    if (!isOnline) {
        extern BOOL IsDarkMode();
        bool isDarkTheme = IsDarkMode();

        std::string offlineIconPath = isDarkTheme ?
            "assets/pngs/ui/mouse_offline_dark.png" :
            "assets/pngs/ui/mouse_offline_light.png";

        Bitmap* offlineIcon = LoadPNGAsBitmap(offlineIconPath);
        if (offlineIcon) {
            g->DrawImage(offlineIcon, 0, 0, iconSize, iconSize);
            delete offlineIcon;
        } else {
            Color fallbackColor(153, 153, 153, 255);
            SolidBrush fallbackBrush(fallbackColor);
            g->FillEllipse(&fallbackBrush, iconSize/4, iconSize/4, iconSize/2, iconSize/2);
        }
    } else {
        extern BOOL IsDarkMode();
        bool isDarkTheme = IsDarkMode();

        std::string mouseImagePath;
        bool hasStatus = isCharging || batteryLevel <= 20; 
        
        bool shouldSwapOverlays = batteryLevel > 90 && !SettingsView::IsIconModeColored();

        if (shouldSwapOverlays) {
            mouseImagePath = isDarkTheme ?
                "assets/pngs/ui/mouse_light_swap.png" :
                "assets/pngs/ui/mouse_dark_swap.png";
        } else if (hasStatus) {
            mouseImagePath = isDarkTheme ?
                "assets/pngs/ui/mouse_corner_cut_dark.png" :
                "assets/pngs/ui/mouse_corner_cut_light.png";
        } else {
            mouseImagePath = isDarkTheme ?
                "assets/pngs/ui/mouse_dark.png" :
                "assets/pngs/ui/mouse_light.png";
        }

        Bitmap* mousePNG = LoadPNGAsBitmap(mouseImagePath);
        Bitmap* mouseMask = LoadPNGAsBitmap("assets/pngs/ui/mouse_mask.png");

        if (mousePNG && mouseMask) {
            bool showColoredMask = true;
            if (isCharging && batteryLevel < 100) {
                showColoredMask = GetChargingBlinkState();
            }

            if (showColoredMask) {
                Bitmap* coloredMask = CreateColoredMouseMask(mouseMask, iconSize, batteryLevel, isOnline, isUpdating);
                if (coloredMask) {
                    g->DrawImage(coloredMask, 0, 0, iconSize, iconSize);
                    delete coloredMask;
                }
            }

            g->DrawImage(mousePNG, 0, 0, iconSize, iconSize);
        } else if (mousePNG) {
            g->DrawImage(mousePNG, 0, 0, iconSize, iconSize);
        } else {
            Color batteryColor = GetBatteryColor(batteryLevel);
            SolidBrush batteryBrush(batteryColor);
            int fillHeight = (iconSize * batteryLevel) / 100;
            int fillY = iconSize - fillHeight;
            g->FillRectangle(&batteryBrush, iconSize/4, fillY, iconSize/2, fillHeight);
        }

        std::string statusIconPath;
        bool showStatusIcon = false;

        if (isCharging) {
            statusIconPath = isDarkTheme ?
                "assets/pngs/ui/status/zap_dark.png" :
                "assets/pngs/ui/status/zap_light.png";
            showStatusIcon = true;
        } else if (batteryLevel <= 10) {
            statusIconPath = isDarkTheme ?
                "assets/pngs/ui/status/battery_critical_dark.png" :
                "assets/pngs/ui/status/battery_critical_light.png";
            showStatusIcon = true;
        } else if (batteryLevel <= 20) {
            statusIconPath = isDarkTheme ?
                "assets/pngs/ui/status/battery_low_dark.png" :
                "assets/pngs/ui/status/battery_low_light.png";
            showStatusIcon = true;
        }

        Bitmap* statusIcon = nullptr;
        if (showStatusIcon) {
            statusIcon = LoadPNGAsBitmap(statusIconPath);
            if (statusIcon) {
                int statusSize = iconSize * 0.6; 
                int statusX = iconSize * 0.5;   
                int statusY = -10;  

                g->DrawImage(statusIcon, statusX, statusY, statusSize, statusSize);
            }
        }

        if (mousePNG) delete mousePNG;
        if (mouseMask) delete mouseMask;
        if (statusIcon) delete statusIcon;
    }

    HICON hIcon = BitmapToHIcon(iconBitmap);

    delete g;
    delete iconBitmap;

    return hIcon;
}

Bitmap* TrayIconRenderer::RenderMouseSVG(int iconSize, bool isDarkTheme) {
    std::string svgPath = "assets/svgs/ui/mouse.svg";

    Color fillColor = isDarkTheme ? Color(255, 255, 255, 255) : Color(255, 0, 0, 0);

    return LoadSVGAsBitmap(svgPath, iconSize, iconSize, fillColor);
}

Bitmap* TrayIconRenderer::CreateBatteryLevelMask(int width, int height, int batteryLevel) {
    std::string maskPath = "assets/pngs/ui/mouse_mask.png";
    Bitmap* maskBitmap = LoadPNGAsBitmap(maskPath);

    if (!maskBitmap) return nullptr;

    Bitmap* result = new Bitmap(width, height, PixelFormat32bppARGB);
    Graphics* g = Graphics::FromImage(result);
    g->Clear(Color(0, 0, 0, 0));

    int fillHeight = (height * batteryLevel) / 100;
    int fillY = height - fillHeight;

    Color batteryColor = GetBatteryColor(batteryLevel);

    SolidBrush fillBrush(batteryColor);

    Rect clipRect(0, fillY, width, fillHeight);
    g->SetClip(clipRect);

    g->FillRectangle(&fillBrush, 0, 0, width, height);

    g->ResetClip();

    g->SetCompositingMode(CompositingModeSourceOver);
    g->DrawImage(maskBitmap, 0, 0, width, height);

    delete g;
    delete maskBitmap;

    return result;
}

Bitmap* TrayIconRenderer::CreateColoredMouseMask(Bitmap* mouseMask, int iconSize,
                                                int batteryLevel, bool isOnline, bool isUpdating) {
    if (!mouseMask) return nullptr;

    Color fillColor;
    if (!isOnline || isUpdating) {
        fillColor = Color(255, 128, 128, 128);
    } else if (!SettingsView::IsIconModeColored()) {
        extern BOOL IsDarkMode();
        bool isDarkTheme = IsDarkMode();
        fillColor = isDarkTheme ? Color(255, 255, 255, 255) : Color(255, 0, 0, 0);
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

            int maskOffset = srcY * maskData.Stride + srcX * 4;
            BYTE maskAlpha = maskPixels[maskOffset + 3];
            BYTE maskValue = maskPixels[maskOffset]; 

            int resultOffset = y * resultData.Stride + x * 4;

            if (maskAlpha > 0 && maskValue > 128 && y >= fillY) {
                resultPixels[resultOffset + 0] = fillColor.GetB(); // Blue
                resultPixels[resultOffset + 1] = fillColor.GetG(); // Green
                resultPixels[resultOffset + 2] = fillColor.GetR(); // Red
                resultPixels[resultOffset + 3] = maskAlpha;        // Alpha
            } else {
                resultPixels[resultOffset + 0] = 0;
                resultPixels[resultOffset + 1] = 0;
                resultPixels[resultOffset + 2] = 0;
                resultPixels[resultOffset + 3] = 0;
            }
        }
    }

    mouseMask->UnlockBits(&maskData);
    result->UnlockBits(&resultData);

    return result;
}

Color TrayIconRenderer::GetBatteryColor(int batteryLevel) {
    return Colors::GetBatteryColor(batteryLevel);
}

void TrayIconRenderer::RenderBatteryText(Graphics* g, int batteryLevel, bool isUpdating,
                                        int iconSize, bool isDarkTheme) {
    Font* font = FontLoader::GetGeistMonoFont(FontWeight::Bold, 11.0f);
    if (!font) {
        FontFamily fontFamily(L"Arial");
        font = new Font(&fontFamily, 11, FontStyleBold, UnitPixel);
    }

    Color textColor = isDarkTheme ? Color(255, 255, 255, 255) : Color(255, 0, 0, 0);
    SolidBrush textBrush(textColor);

    std::wstring text;
    if (isUpdating) {
        text = L"...";
    } else {
        text = std::to_wstring(batteryLevel);
    }

    StringFormat format;
    format.SetAlignment(StringAlignmentCenter);
    format.SetLineAlignment(StringAlignmentCenter);

    RectF textRect(0, iconSize * 0.6f, (REAL)iconSize, iconSize * 0.4f);

    g->DrawString(text.c_str(), -1, font, textRect, &format, &textBrush);
}

void TrayIconRenderer::ApplyDimmingEffect(Graphics* g, int iconSize) {
    SolidBrush dimmingBrush(Color(128, 128, 128, 128));
    g->FillRectangle(&dimmingBrush, 0, 0, iconSize, iconSize);
}

HICON TrayIconRenderer::BitmapToHIcon(Bitmap* bitmap) {
    HICON hIcon = nullptr;
    if (bitmap) {
        bitmap->GetHICON(&hIcon);
    }
    return hIcon;
}

Bitmap* TrayIconRenderer::LoadSVGAsBitmap(const std::string& svgPath, int width, int height,
                                         Color fillColor) {
    std::wstring widePath(svgPath.begin(), svgPath.end());

    Image* svgImage = SVGRenderer::LoadSVGAsImage(widePath.c_str(), width, height, fillColor);

    if (!svgImage) return nullptr;

    Bitmap* bitmap = new Bitmap(width, height, PixelFormat32bppARGB);
    Graphics* g = Graphics::FromImage(bitmap);
    g->Clear(Color(0, 0, 0, 0));
    g->DrawImage(svgImage, 0, 0, width, height);

    delete g;
    SVGRenderer::FreeImage(svgImage);

    return bitmap;
}

Bitmap* TrayIconRenderer::LoadPNGAsBitmap(const std::string& pngPath) {
    return ResourceLoader::LoadPNGAsBitmap(pngPath);
}

bool TrayIconRenderer::GetChargingBlinkState() {
    DWORD currentTime = GetTickCount();

    if (currentTime - s_lastBlinkTime >= 200) {
        s_blinkState = !s_blinkState;
        s_lastBlinkTime = currentTime;
    }

    return s_blinkState;
}
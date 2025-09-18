#include "mouse_item.h"
#include "colors.h"
#include "font_loader.h"
#include "battery_icon_renderer.h"

using namespace Gdiplus;

void MouseItemRenderer::RenderMouseItem(Graphics* g, const MouseItem& item, int x, int y, int width, int height) {
    g->SetSmoothingMode(SmoothingModeAntiAlias);
    g->SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

    float dimmingFactor = item.isOnline ? 1.0f : 0.4f;

    Color baseBgColor = Colors::GetCardBackground();
    Color hoverBgColor = Colors::GetHoverOverlay();
    
    BYTE baseR = baseBgColor.GetR();
    BYTE baseG = baseBgColor.GetG();
    BYTE baseB = baseBgColor.GetB();
    
    BYTE hoverR = min(255, static_cast<BYTE>(baseR + 20));
    BYTE hoverG = min(255, static_cast<BYTE>(baseG + 20));
    BYTE hoverB = min(255, static_cast<BYTE>(baseB + 20));

    BYTE bgR = static_cast<BYTE>(baseR + (hoverR - baseR) * item.hoverProgress);
    BYTE bgG = static_cast<BYTE>(baseG + (hoverG - baseG) * item.hoverProgress);
    BYTE bgB = static_cast<BYTE>(baseB + (hoverB - baseB) * item.hoverProgress);

    bgR = static_cast<BYTE>(bgR * dimmingFactor);
    bgG = static_cast<BYTE>(bgG * dimmingFactor);
    bgB = static_cast<BYTE>(bgB * dimmingFactor);

    SolidBrush cardBrush(Color(255, bgR, bgG, bgB));
    RectF cardRect(x, y, width, height);
    g->FillRectangle(&cardBrush, cardRect);

    Color baseBorderColor = Colors::GetBorderDefault();
    Color hoverBorderColor = Colors::GetBorderHover();

    BYTE borderR = static_cast<BYTE>(baseBorderColor.GetR() + (hoverBorderColor.GetR() - baseBorderColor.GetR()) * item.hoverProgress);
    BYTE borderG = static_cast<BYTE>(baseBorderColor.GetG() + (hoverBorderColor.GetG() - baseBorderColor.GetG()) * item.hoverProgress);
    BYTE borderB = static_cast<BYTE>(baseBorderColor.GetB() + (hoverBorderColor.GetB() - baseBorderColor.GetB()) * item.hoverProgress);

    borderR = static_cast<BYTE>(borderR * dimmingFactor);
    borderG = static_cast<BYTE>(borderG * dimmingFactor);
    borderB = static_cast<BYTE>(borderB * dimmingFactor);

    Pen borderPen(Color(255, borderR, borderG, borderB), 1.0f);
    g->DrawRectangle(&borderPen, cardRect);
    
    Image* mouseImage = LoadMouseImage(item.imagePath);
    if (mouseImage) {
        int imageSize = min(width * 0.8f, height * 0.7f);
        int imageX = x + (width - imageSize) / 2;
        int imageY = y + height * 0.1f;

        if (item.isOnline) {
            g->DrawImage(mouseImage,
                Rect(imageX, imageY, imageSize, imageSize),
                0, 0, mouseImage->GetWidth(), mouseImage->GetHeight(),
                UnitPixel);
        } else {
            ColorMatrix colorMatrix = {
                dimmingFactor, 0.0f, 0.0f, 0.0f, 0.0f,
                0.0f, dimmingFactor, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, dimmingFactor, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 0.0f, 1.0f
            };

            ImageAttributes imageAttributes;
            imageAttributes.SetColorMatrix(&colorMatrix);

            g->DrawImage(mouseImage,
                Rect(imageX, imageY, imageSize, imageSize),
                0, 0, mouseImage->GetWidth(), mouseImage->GetHeight(),
                UnitPixel, &imageAttributes);
        }

        delete mouseImage;
    }
    
    Gdiplus::Font* interFont = FontLoader::GetInterFont(FontWeight::Medium, 14.0f);
    if (interFont) {
        g->SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);

        BYTE textAlpha = static_cast<BYTE>(255 * dimmingFactor);
        Color textColor = Colors::GetForeground();
        SolidBrush textBrush(Color(textAlpha, textColor.GetR(), textColor.GetG(), textColor.GetB()));
        StringFormat format;
        format.SetAlignment(StringAlignmentCenter);
        format.SetLineAlignment(StringAlignmentCenter);

        RectF nameRect(x, y + height * 0.7f, width, 20);
        g->DrawString(item.name.c_str(), -1, interFont, nameRect, &format, &textBrush);

        g->SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    }
    
    int batterySize = min(width * 0.12f, height * 0.12f);
    int batteryY = y + height * 0.8f;
    
    int textWidth = 28; 
    int spacing = 1;
    int totalWidth = batterySize + spacing + textWidth;
    int batteryX = x + (width - totalWidth) / 2; 
    
    Color batteryColor;
    int displayLevel = item.batteryLevel;
    bool displayCharging = item.isCharging;

    if (item.isUpdating) {
        batteryColor = Color(255, 255, 165, 0);
        displayLevel = 50; 
        displayCharging = false;
    } else if (!item.isOnline) {
        batteryColor = Color(255, 128, 128, 128);
        displayLevel = 0; 
        displayCharging = false;
    } else {
        batteryColor = Colors::GetBatteryColor(item.batteryLevel);
    }

    RenderBatteryIcon(g, batteryX, batteryY, batterySize, displayLevel, displayCharging, batteryColor);
    
    Gdiplus::Font* mediumFont = FontLoader::GetGeistMonoFont(FontWeight::Medium, 12.0f);
    if (mediumFont) {
        
        g->SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);

        SolidBrush numberBrush(batteryColor);
        StringFormat format;
        format.SetAlignment(StringAlignmentCenter);
        format.SetLineAlignment(StringAlignmentCenter);

        WCHAR percentage[16];
        if (item.isUpdating) {
            wcscpy_s(percentage, L"...");
        } else if (!item.isOnline) {
            wcscpy_s(percentage, L"---");
        } else {
            swprintf_s(percentage, L"%d", item.batteryLevel);
        }

        RectF numberRect(batteryX + batterySize + spacing, batteryY, textWidth, batterySize);
        g->DrawString(percentage, -1, mediumFont, numberRect, &format, &numberBrush);
        
        g->SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    }
}

Size MouseItemRenderer::GetRecommendedSize() {
    return Size(170, 200);
}

Image* MouseItemRenderer::LoadMouseImage(const std::wstring& imagePath) {
    return new Image(imagePath.c_str());
}

void MouseItemRenderer::RenderBatteryIcon(Graphics* g, int x, int y, int size, int batteryLevel, bool isCharging, Color color) {
    BatteryIconRenderer::RenderBatteryIcon(g, x, y, size, batteryLevel, isCharging, color);
}

Color MouseItemRenderer::GetBatteryColor(int batteryLevel) {
    return BatteryIconRenderer::GetBatteryColor(batteryLevel);
}

Color MouseItem::GetConnectionColor(ConnectionType type) const {
    switch (type) {
        case ConnectionType::USB_WIRED:
            return Color(255, 100, 200, 100); // GREEN
        case ConnectionType::WIRELESS_DONGLE:
            return Color(255, 100, 150, 255); // BLUE
        case ConnectionType::BLUETOOTH:
            return Color(255, 200, 100, 200); // PURPLE
        default:
            return Color(255, 255, 255, 255); // WHITE
    }
}



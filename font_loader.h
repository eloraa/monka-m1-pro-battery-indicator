#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <map>

#pragma comment(lib, "gdiplus.lib")

enum class FontWeight {
    Thin = 100,
    ExtraLight = 200,
    Light = 300,
    Regular = 400,
    Medium = 500,
    SemiBold = 600,
    Bold = 700,
    ExtraBold = 800,
    Black = 900
};

enum class CustomFontFamily {
    Inter,
    GeistMono
};

class FontLoader {
private:
    static bool initialized;
    static std::map<std::pair<CustomFontFamily, FontWeight>, Gdiplus::Font*> fontCache;

    static Gdiplus::Font* LoadFontWeight(CustomFontFamily family, FontWeight weight, float size);

    static std::wstring GetFontFamilyName(CustomFontFamily family, FontWeight weight);

public:
    static bool Initialize();

    static void Cleanup();

    static Gdiplus::Font* GetFont(CustomFontFamily family, FontWeight weight, float size = 14.0f);

    static Gdiplus::Font* GetInterFont(FontWeight weight = FontWeight::Regular, float size = 14.0f);

    static Gdiplus::Font* GetGeistMonoFont(FontWeight weight = FontWeight::Regular, float size = 14.0f);
    
    static bool PreloadCommonWeights();
};

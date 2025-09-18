#include "font_loader.h"
#include "resource.h"
#include <vector>

bool FontLoader::initialized = false;
std::map<std::pair<CustomFontFamily, FontWeight>, Gdiplus::Font*> FontLoader::fontCache;

bool FontLoader::Initialize() {
    if (initialized) return true;

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    initialized = true;
    return true;
}

void FontLoader::Cleanup() {
    for (auto& pair : fontCache) {
        delete pair.second;
    }
    fontCache.clear();
    initialized = false;
}

Gdiplus::Font* FontLoader::LoadFontWeight(CustomFontFamily family, FontWeight weight, float size) {
    HMODULE hModule = GetModuleHandle(NULL);
    if (!hModule) return nullptr;

    int resourceId = 0;
    switch (family) {
        case CustomFontFamily::Inter:
            resourceId = IDR_INTER_FONT;
            break;
        case CustomFontFamily::GeistMono:
            resourceId = IDR_GEIST_MONO_FONT;
            break;
        default:
            return nullptr;
    }

    HRSRC hRes = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
    if (!hRes) return nullptr;

    HGLOBAL hMem = LoadResource(hModule, hRes);
    if (!hMem) return nullptr;

    DWORD fontSize = SizeofResource(hModule, hRes);
    void* fontData = LockResource(hMem);
    if (!fontData) return nullptr;

    DWORD numFonts = 0;
    HANDLE fontHandle = AddFontMemResourceEx(fontData, fontSize, 0, &numFonts);
    if (!fontHandle) return nullptr;

    std::vector<std::wstring> familyNamesToTry;

    familyNamesToTry.push_back(GetFontFamilyName(family, weight));

    if (family == CustomFontFamily::GeistMono) {
        switch (weight) {
            case FontWeight::Medium:
                familyNamesToTry.push_back(L"GeistMono Medium");
                familyNamesToTry.push_back(L"Geist Mono Medium");
                familyNamesToTry.push_back(L"Geist Mono VF");
                break;
            case FontWeight::Bold:
                familyNamesToTry.push_back(L"GeistMono Bold");
                familyNamesToTry.push_back(L"Geist Mono Bold");
                familyNamesToTry.push_back(L"Geist Mono VF");
                break;
            case FontWeight::Regular:
                familyNamesToTry.push_back(L"GeistMono");
                familyNamesToTry.push_back(L"Geist Mono VF");
                break;
            default:
                familyNamesToTry.push_back(L"Geist Mono VF");
                break;
        }
    }

    std::wstring baseName = (family == CustomFontFamily::Inter) ? L"Inter" : L"Geist Mono";
    familyNamesToTry.push_back(baseName);

    Gdiplus::FontFamily* gdiFontFamily = nullptr;

    for (const auto& name : familyNamesToTry) {
        gdiFontFamily = new Gdiplus::FontFamily(name.c_str());

        if (gdiFontFamily && gdiFontFamily->IsAvailable()) {
            break;
        }

        if (gdiFontFamily) {
            delete gdiFontFamily;
            gdiFontFamily = nullptr;
        }
    }

    if (!gdiFontFamily) {
        return nullptr;
    }

    int fontStyle = Gdiplus::FontStyleRegular;
    switch (weight) {
        case FontWeight::Thin:
        case FontWeight::ExtraLight:
        case FontWeight::Light:
            fontStyle = Gdiplus::FontStyleRegular;
            break;
        case FontWeight::Regular:
            fontStyle = Gdiplus::FontStyleRegular;
            break;
        case FontWeight::Medium:
        case FontWeight::SemiBold:
            fontStyle = Gdiplus::FontStyleBold; 
            break;
        case FontWeight::Bold:
        case FontWeight::ExtraBold:
        case FontWeight::Black:
            fontStyle = Gdiplus::FontStyleBold;
            break;
        default:
            fontStyle = Gdiplus::FontStyleRegular;
            break;
    }

    Gdiplus::Font* font = new Gdiplus::Font(gdiFontFamily, size, fontStyle, Gdiplus::UnitPixel);

    if (!font || font->GetLastStatus() != Gdiplus::Ok) {
        delete gdiFontFamily;
        if (font) {
            delete font;
        }
        return nullptr;
    }

    delete gdiFontFamily;

    return font;
}

std::wstring FontLoader::GetFontFamilyName(CustomFontFamily family, FontWeight weight) {
    std::wstring baseName;
    std::wstring weightName;

    switch (family) {
        case CustomFontFamily::Inter:
            baseName = L"Inter";
            break;
        case CustomFontFamily::GeistMono:
            baseName = L"Geist Mono";
            break;
        default:
            return L"";
    }
    if (family == CustomFontFamily::GeistMono) {
        switch (weight) {
            case FontWeight::Thin:       // 100
                weightName = L" Thin";
                break;
            case FontWeight::ExtraLight: // 200
                weightName = L" UltraLight";
                break;
            case FontWeight::Light:      // 300
                weightName = L" Light";
                break;
            case FontWeight::Regular:    // 400
                weightName = L""; // REGULAR
                break;
            case FontWeight::Medium:     // 500
                weightName = L" Medium";
                break;
            case FontWeight::SemiBold:   // 600
                weightName = L" SemiBold";
                break;
            case FontWeight::Bold:       // 700
                weightName = L" Bold";
                break;
            case FontWeight::ExtraBold:  // 800
                weightName = L" ExtraBold";
                break;
            case FontWeight::Black:      // 900
                weightName = L" Black";
                break;
            default:
                weightName = L""; // REGULAR FALLBACK
                break;
        }
    } else {
        switch (weight) {
            case FontWeight::Thin:       // 100
                weightName = L" Thin";
                break;
            case FontWeight::ExtraLight: // 200
                weightName = L" ExtraLight";
                break;
            case FontWeight::Light:      // 300
                weightName = L" Light";
                break;
            case FontWeight::Regular:    // 400
                weightName = L""; // REGULAR
                break;
            case FontWeight::Medium:     // 500
                weightName = L" Medium";
                break;
            case FontWeight::SemiBold:   // 600
                weightName = L" SemiBold";
                break;
            case FontWeight::Bold:       // 700
                weightName = L" Bold";
                break;
            case FontWeight::ExtraBold:  // 800
                weightName = L" ExtraBold";
                break;
            case FontWeight::Black:      // 900
                weightName = L" Black";
                break;
            default:
                weightName = L""; // REGULAR FALLBACK
                break;
        }
    }

    return baseName + weightName;
}

Gdiplus::Font* FontLoader::GetFont(CustomFontFamily family, FontWeight weight, float size) {
    if (!initialized) {
        if (!Initialize()) return nullptr;
    }

    auto key = std::make_pair(family, weight);
    auto it = fontCache.find(key);

    if (it != fontCache.end()) {
        if (it->second->GetSize() == size) {
            return it->second;
        } else {
            delete it->second;
            fontCache.erase(it);
        }
    }

    Gdiplus::Font* font = LoadFontWeight(family, weight, size);
    if (font) {
        fontCache[key] = font;
    }

    return font;
}

Gdiplus::Font* FontLoader::GetInterFont(FontWeight weight, float size) {
    return GetFont(CustomFontFamily::Inter, weight, size);
}

Gdiplus::Font* FontLoader::GetGeistMonoFont(FontWeight weight, float size) {
    return GetFont(CustomFontFamily::GeistMono, weight, size);
}

bool FontLoader::PreloadCommonWeights() {
    if (!initialized) {
        if (!Initialize()) return false;
    }

    const std::vector<FontWeight> commonWeights = {
        FontWeight::Thin,       // 100
        FontWeight::ExtraLight, // 200
        FontWeight::Light,      // 300
        FontWeight::Regular,    // 400
        FontWeight::Medium,     // 500
        FontWeight::SemiBold,   // 600
        FontWeight::Bold,       // 700
        FontWeight::ExtraBold,  // 800
        FontWeight::Black       // 900
    };

    bool success = true;

    for (FontWeight weight : commonWeights) {
        Gdiplus::Font* interFont = GetInterFont(weight, 14.0f);
        Gdiplus::Font* geistFont = GetGeistMonoFont(weight, 14.0f);

        if (!interFont || !geistFont) {
            success = false;
        }
    }

    return success;
}
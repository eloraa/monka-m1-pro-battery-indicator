#include "resource_loader.h"
#include "resource.h"
#include <shlwapi.h>
#include <fstream>

#pragma comment(lib, "shlwapi.lib")

using namespace Gdiplus;

Bitmap* ResourceLoader::LoadPNGFromResource(int resourceId) {
    HRSRC hResource = FindResource(GetModuleHandle(NULL), MAKEINTRESOURCE(resourceId), RT_RCDATA);
    if (!hResource) return nullptr;

    HGLOBAL hGlobal = LoadResource(GetModuleHandle(NULL), hResource);
    if (!hGlobal) return nullptr;

    DWORD resourceSize = SizeofResource(GetModuleHandle(NULL), hResource);
    const BYTE* resourceData = static_cast<const BYTE*>(LockResource(hGlobal));

    if (!resourceData || resourceSize == 0) return nullptr;

    return CreateBitmapFromResourceData(resourceData, resourceSize);
}

Bitmap* ResourceLoader::LoadPNGAsBitmap(const std::string& pngPath) {
    int resourceId = GetResourceIdFromPath(pngPath);
    if (resourceId != -1) {
        Bitmap* bitmap = LoadPNGFromResource(resourceId);
        if (bitmap && bitmap->GetLastStatus() == Ok) {
            return bitmap;
        }
        if (bitmap) delete bitmap;
    }

    std::wstring widePath(pngPath.begin(), pngPath.end());
    Bitmap* fileBitmap = new Bitmap(widePath.c_str());

    if (fileBitmap->GetLastStatus() != Ok) {
        delete fileBitmap;
        return nullptr;
    }

    return fileBitmap;
}

std::string ResourceLoader::LoadSVGAsString(const std::string& svgPath) {
    int resourceId = GetResourceIdFromPath(svgPath);
    if (resourceId != -1) {
        std::string svgContent = LoadResourceAsString(resourceId);
        if (!svgContent.empty()) {
            return svgContent;
        }
    }

    std::ifstream file(svgPath);
    if (file.is_open()) {
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        return content;
    }

    return "";
}

std::string ResourceLoader::LoadResourceAsString(int resourceId) {
    HRSRC hResource = FindResource(GetModuleHandle(NULL), MAKEINTRESOURCE(resourceId), RT_RCDATA);
    if (!hResource) return "";

    HGLOBAL hGlobal = LoadResource(GetModuleHandle(NULL), hResource);
    if (!hGlobal) return "";

    DWORD resourceSize = SizeofResource(GetModuleHandle(NULL), hResource);
    const char* resourceData = static_cast<const char*>(LockResource(hGlobal));

    if (!resourceData || resourceSize == 0) return "";

    return std::string(resourceData, resourceSize);
}

bool ResourceLoader::ExtractResourceToDisk(int resourceId, const std::string& outputPath) {
    HRSRC hResource = FindResource(GetModuleHandle(NULL), MAKEINTRESOURCE(resourceId), RT_RCDATA);
    if (!hResource) return false;

    HGLOBAL hGlobal = LoadResource(GetModuleHandle(NULL), hResource);
    if (!hGlobal) return false;

    DWORD resourceSize = SizeofResource(GetModuleHandle(NULL), hResource);
    const BYTE* resourceData = static_cast<const BYTE*>(LockResource(hGlobal));

    if (!resourceData || resourceSize == 0) return false;

    std::ofstream file(outputPath, std::ios::binary);
    if (!file.is_open()) return false;

    file.write(reinterpret_cast<const char*>(resourceData), resourceSize);
    file.close();

    return true;
}

int ResourceLoader::GetResourceIdFromPath(const std::string& pngPath) {
    if (pngPath == "assets/pngs/mouse/m1_pro.png") return IDR_MOUSE_M1_PRO;
    if (pngPath == "assets/pngs/ui/mouse_corner_cut_dark.png") return IDR_MOUSE_CORNER_CUT_DARK;
    if (pngPath == "assets/pngs/ui/mouse_corner_cut_light.png") return IDR_MOUSE_CORNER_CUT_LIGHT;
    if (pngPath == "assets/pngs/ui/mouse_dark_swap.png") return IDR_MOUSE_DARK_SWAP;
    if (pngPath == "assets/pngs/ui/mouse_dark.png") return IDR_MOUSE_DARK;
    if (pngPath == "assets/pngs/ui/mouse_light_swap.png") return IDR_MOUSE_LIGHT_SWAP;
    if (pngPath == "assets/pngs/ui/mouse_light.png") return IDR_MOUSE_LIGHT;
    if (pngPath == "assets/pngs/ui/mouse_mask.png") return IDR_MOUSE_MASK;
    if (pngPath == "assets/pngs/ui/mouse_offline_dark.png") return IDR_MOUSE_OFFLINE_DARK;
    if (pngPath == "assets/pngs/ui/mouse_offline_light.png") return IDR_MOUSE_OFFLINE_LIGHT;
    if (pngPath == "assets/pngs/ui/status/battery_critical_dark.png") return IDR_BATTERY_CRITICAL_DARK;
    if (pngPath == "assets/pngs/ui/status/battery_critical_light.png") return IDR_BATTERY_CRITICAL_LIGHT;
    if (pngPath == "assets/pngs/ui/status/battery_low_dark.png") return IDR_BATTERY_LOW_DARK;
    if (pngPath == "assets/pngs/ui/status/battery_low_light.png") return IDR_BATTERY_LOW_LIGHT;
    if (pngPath == "assets/pngs/ui/status/zap_dark.png") return IDR_ZAP_DARK;
    if (pngPath == "assets/pngs/ui/status/zap_light.png") return IDR_ZAP_LIGHT;
    
    if (pngPath == "assets/svgs/battery_icons/battery_icon_charging.svg") return IDR_BATTERY_CHARGING;
    if (pngPath == "assets/svgs/battery_icons/battery_icon_critical.svg") return IDR_BATTERY_CRITICAL;
    if (pngPath == "assets/svgs/battery_icons/battery_icon_empty.svg") return IDR_BATTERY_EMPTY;
    if (pngPath == "assets/svgs/battery_icons/battery_icon_full.svg") return IDR_BATTERY_FULL;
    if (pngPath == "assets/svgs/battery_icons/battery_icon_low.svg") return IDR_BATTERY_LOW;
    if (pngPath == "assets/svgs/battery_icons/battery_icon_medium.svg") return IDR_BATTERY_MEDIUM;

    return -1; // NOT FOUND
}

Bitmap* ResourceLoader::CreateBitmapFromResourceData(const BYTE* data, DWORD size) {
    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!hGlobal) return nullptr;

    void* pData = GlobalLock(hGlobal);
    if (!pData) {
        GlobalFree(hGlobal);
        return nullptr;
    }

    memcpy(pData, data, size);
    GlobalUnlock(hGlobal);

    IStream* pStream = nullptr;
    if (CreateStreamOnHGlobal(hGlobal, TRUE, &pStream) != S_OK) {
        GlobalFree(hGlobal);
        return nullptr;
    }

    Bitmap* bitmap = Bitmap::FromStream(pStream);
    pStream->Release();

    if (!bitmap || bitmap->GetLastStatus() != Ok) {
        if (bitmap) delete bitmap;
        return nullptr;
    }

    return bitmap;
}
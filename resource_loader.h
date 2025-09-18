#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <string>

#pragma comment(lib, "gdiplus.lib")

class ResourceLoader {
public:
    static Gdiplus::Bitmap* LoadPNGFromResource(int resourceId);

    static Gdiplus::Bitmap* LoadPNGAsBitmap(const std::string& pngPath);

    static std::string LoadSVGAsString(const std::string& svgPath);

    static std::string LoadResourceAsString(int resourceId);

    static bool ExtractResourceToDisk(int resourceId, const std::string& outputPath);

private:
    static int GetResourceIdFromPath(const std::string& pngPath);

    static Gdiplus::Bitmap* CreateBitmapFromResourceData(const BYTE* data, DWORD size);
};
#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <string>

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

class SVGRenderer {
public:
    static bool RenderSVGFromFile(Graphics* g, const wchar_t* filePath, int x, int y, int width, int height, Color tintColor = Color(255, 255, 255, 255));
    
    static bool RenderSVGFromString(Graphics* g, const std::string& svgContent, int x, int y, int width, int height, Color tintColor = Color(255, 255, 255, 255));
    
    static Image* LoadSVGAsImage(const wchar_t* filePath, int width, int height, Color tintColor = Color(255, 255, 255, 255));
    
    static void FreeImage(Image* image);
    
private:
    static void ConvertRGBAtoBGRA(unsigned char* data, int width, int height);
};

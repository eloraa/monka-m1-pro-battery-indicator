#include "svg_renderer.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <string>

#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvg.h"
#include "nanosvgrast.h"

using namespace Gdiplus;

bool SVGRenderer::RenderSVGFromFile(Graphics* g, const wchar_t* filePath, int x, int y, int width, int height, Color tintColor) {
    char narrowPath[512];
    WideCharToMultiByte(CP_UTF8, 0, filePath, -1, narrowPath, sizeof(narrowPath), NULL, NULL);
    
    NSVGimage* image = nsvgParseFromFile(narrowPath, "px", 96.0f);
    if (!image) {
        return false;
    }
    
    NSVGrasterizer* rast = nsvgCreateRasterizer();
    if (!rast) {
        nsvgDelete(image);
        return false;
    }
    
    unsigned char* imgData = new unsigned char[width * height * 4];
    if (!imgData) {
        nsvgDeleteRasterizer(rast);
        nsvgDelete(image);
        return false;
    }
    
    nsvgRasterize(rast, image, 0, 0, (float)width / image->width, imgData, width, height, width * 4);
    
    ConvertRGBAtoBGRA(imgData, width, height);
    
    for (int i = 0; i < width * height; i++) {
        int pixelIndex = i * 4;
        BYTE originalAlpha = imgData[pixelIndex + 3]; 

        if (originalAlpha > 0) { 
            imgData[pixelIndex + 0] = tintColor.GetB();
            imgData[pixelIndex + 1] = tintColor.GetG();
            imgData[pixelIndex + 2] = tintColor.GetR(); 
            imgData[pixelIndex + 3] = originalAlpha; 
        }
    }
    
    Bitmap* bitmap = new Bitmap(width, height, width * 4, PixelFormat32bppARGB, imgData);
    
    g->DrawImage(bitmap, x, y, width, height);
    
    delete bitmap;
    nsvgDeleteRasterizer(rast);
    nsvgDelete(image);
    delete[] imgData;
    
    return true;
}

bool SVGRenderer::RenderSVGFromString(Graphics* g, const std::string& svgContent, int x, int y, int width, int height, Color tintColor) {
    if (svgContent.empty()) {
        return false;
    }
    
    std::string svgCopy = svgContent;
    NSVGimage* image = nsvgParse(const_cast<char*>(svgCopy.c_str()), "px", 96.0f);
    if (!image) {
        return false;
    }
    
    NSVGrasterizer* rast = nsvgCreateRasterizer();
    if (!rast) {
        nsvgDelete(image);
        return false;
    }
    
    unsigned char* imgData = new unsigned char[width * height * 4];
    if (!imgData) {
        nsvgDeleteRasterizer(rast);
        nsvgDelete(image);
        return false;
    }
    
    nsvgRasterize(rast, image, 0, 0, (float)width / image->width, imgData, width, height, width * 4);
    
    ConvertRGBAtoBGRA(imgData, width, height);
    
    for (int i = 0; i < width * height; i++) {
        int pixelIndex = i * 4;
        BYTE originalAlpha = imgData[pixelIndex + 3];

        if (originalAlpha > 0) { 
            imgData[pixelIndex + 0] = tintColor.GetB(); 
            imgData[pixelIndex + 1] = tintColor.GetG();
            imgData[pixelIndex + 2] = tintColor.GetR();
            imgData[pixelIndex + 3] = originalAlpha;    
        }
    }
    
    Bitmap* bitmap = new Bitmap(width, height, width * 4, PixelFormat32bppARGB, imgData);
    
    g->DrawImage(bitmap, x, y, width, height);
    
    delete bitmap;
    nsvgDeleteRasterizer(rast);
    nsvgDelete(image);
    delete[] imgData;
    
    return true;
}

Image* SVGRenderer::LoadSVGAsImage(const wchar_t* filePath, int width, int height, Color tintColor) {
    char narrowPath[512];
    WideCharToMultiByte(CP_UTF8, 0, filePath, -1, narrowPath, sizeof(narrowPath), NULL, NULL);
    
    NSVGimage* image = nsvgParseFromFile(narrowPath, "px", 96.0f);
    if (!image) {
        return nullptr;
    }
    
    NSVGrasterizer* rast = nsvgCreateRasterizer();
    if (!rast) {
        nsvgDelete(image);
        return nullptr;
    }
    
    unsigned char* imgData = new unsigned char[width * height * 4];
    if (!imgData) {
        nsvgDeleteRasterizer(rast);
        nsvgDelete(image);
        return nullptr;
    }
    
    nsvgRasterize(rast, image, 0, 0, (float)width / image->width, imgData, width, height, width * 4);
    
    ConvertRGBAtoBGRA(imgData, width, height);
    
    for (int i = 0; i < width * height; i++) {
        int pixelIndex = i * 4;
        BYTE originalAlpha = imgData[pixelIndex + 3]; 

        if (originalAlpha > 0) { 
            imgData[pixelIndex + 0] = tintColor.GetB(); 
            imgData[pixelIndex + 1] = tintColor.GetG();
            imgData[pixelIndex + 2] = tintColor.GetR();
            imgData[pixelIndex + 3] = originalAlpha;    
        }
    }
    
    Bitmap* bitmap = new Bitmap(width, height, width * 4, PixelFormat32bppARGB, imgData);
    
    nsvgDeleteRasterizer(rast);
    nsvgDelete(image);
    delete[] imgData;
    
    return bitmap;
}

void SVGRenderer::FreeImage(Image* image) {
    if (image) {
        delete image;
    }
}

void SVGRenderer::ConvertRGBAtoBGRA(unsigned char* data, int width, int height) {
    for (int i = 0; i < width * height; i++) {
        int pixelIndex = i * 4;
        BYTE r = data[pixelIndex + 0];
        BYTE g = data[pixelIndex + 1];
        BYTE b = data[pixelIndex + 2];
        BYTE a = data[pixelIndex + 3];
        
        data[pixelIndex + 0] = b; // Blue
        data[pixelIndex + 1] = g; // Green
        data[pixelIndex + 2] = r; // Red
        data[pixelIndex + 3] = a; // Alpha
    }
}

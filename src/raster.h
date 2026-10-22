#pragma once
#include <vector>
#include <cstdint>

// Packed pixel format: 0xAARRGGBB (native uint32; little-endian memory = B,G,R,A)
static inline uint32_t RGBA(unsigned r, unsigned g, unsigned b, unsigned a = 255) {
    return (uint32_t)(a << 24 | r << 16 | g << 8 | b);
}

class Raster {
public:
    int w = 0, h = 0;
    std::vector<uint32_t> px;

    void Resize(int width, int height);
    void Clear(uint32_t c);
    void BlendPixel(int x, int y, uint32_t c);
    void FillRect(int x, int y, int rw, int rh, uint32_t c);
    void FillTri(float x0, float y0, float x1, float y1, float x2, float y2, uint32_t c);
    void FillCircle(float cx, float cy, float r, uint32_t c);
    void ThickLine(float x1, float y1, float x2, float y2, float width, uint32_t c);
    void DrawChar(int x, int y, char ch, uint32_t c, int scale);
    void DrawText(int x, int y, const char* s, uint32_t c, int scale = 2);
    int  TextWidth(const char* s, int scale = 2);
    void DrawTextCentered(int cx, int y, const char* s, uint32_t c, int scale = 2);
};

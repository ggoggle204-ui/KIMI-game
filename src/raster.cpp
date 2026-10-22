#include "raster.h"
#include "font.h"
#include <cmath>
#include <cstring>

void Raster::Resize(int width, int height) {
    w = width; h = height;
    px.assign((size_t)w * h, 0xFF000000u);
}

void Raster::Clear(uint32_t c) {
    for (size_t i = 0; i < px.size(); i++) px[i] = c;
}

void Raster::BlendPixel(int x, int y, uint32_t c) {
    if (x < 0 || y < 0 || x >= w || y >= h) return;
    uint32_t d = px[(size_t)y * w + x];
    unsigned a = (c >> 24) & 255;
    if (a == 255) { px[(size_t)y * w + x] = c | 0xFF000000u; return; }
    if (a == 0) return;
    unsigned ia = 255 - a;
    unsigned r = (a * ((c >> 16) & 255) + ia * ((d >> 16) & 255)) / 255;
    unsigned g = (a * ((c >> 8) & 255) + ia * ((d >> 8) & 255)) / 255;
    unsigned b = (a * (c & 255) + ia * (d & 255)) / 255;
    px[(size_t)y * w + x] = 0xFF000000u | (r << 16) | (g << 8) | b;
}

void Raster::FillRect(int x, int y, int rw, int rh, uint32_t c) {
    for (int j = y; j < y + rh; j++)
        for (int i = x; i < x + rw; i++)
            BlendPixel(i, j, c);
}

void Raster::FillTri(float x0, float y0, float x1, float y1, float x2, float y2, uint32_t c) {
    float d = (y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2);
    if (fabsf(d) < 1e-6f) return;
    float minx = fminf(x0, fminf(x1, x2)), maxx = fmaxf(x0, fmaxf(x1, x2));
    float miny = fminf(y0, fminf(y1, y2)), maxy = fmaxf(y0, fmaxf(y1, y2));
    int ix0 = (int)floorf(minx), ix1 = (int)ceilf(maxx);
    int iy0 = (int)floorf(miny), iy1 = (int)ceilf(maxy);
    if (ix0 < 0) ix0 = 0; if (iy0 < 0) iy0 = 0;
    if (ix1 > w) ix1 = w; if (iy1 > h) iy1 = h;
    float inv = 1.0f / d;
    for (int y = iy0; y < iy1; y++) {
        for (int x = ix0; x < ix1; x++) {
            float px_ = x + 0.5f, py_ = y + 0.5f;
            float w0 = ((y1 - y2) * (px_ - x2) + (x2 - x1) * (py_ - y2)) * inv;
            float w1 = ((y2 - y0) * (px_ - x2) + (x0 - x2) * (py_ - y2)) * inv;
            float w2 = 1.0f - w0 - w1;
            if (w0 >= 0 && w1 >= 0 && w2 >= 0) BlendPixel(x, y, c);
        }
    }
}

void Raster::FillCircle(float cx, float cy, float r, uint32_t c) {
    float r2 = r * r;
    int x0 = (int)floorf(cx - r), x1 = (int)ceilf(cx + r);
    int y0 = (int)floorf(cy - r), y1 = (int)ceilf(cy + r);
    if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0;
    if (x1 > w) x1 = w; if (y1 > h) y1 = h;
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++) {
            float dx = x + 0.5f - cx, dy = y + 0.5f - cy;
            if (dx * dx + dy * dy <= r2) BlendPixel(x, y, c);
        }
}

void Raster::ThickLine(float x1, float y1, float x2, float y2, float width, uint32_t c) {
    float dx = x2 - x1, dy = y2 - y1;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1e-6f) { FillCircle(x1, y1, width * 0.5f, c); return; }
    float nx = -dy / len * width * 0.5f, ny = dx / len * width * 0.5f;
    FillTri(x1 + nx, y1 + ny, x2 + nx, y2 + ny, x2 - nx, y2 - ny, c);
    FillTri(x1 + nx, y1 + ny, x2 - nx, y2 - ny, x1 - nx, y1 - ny, c);
}

void Raster::DrawChar(int x, int y, char ch, uint32_t c, int scale) {
    uint8_t g[5];
    if (!GetGlyph(ch, g)) return;
    for (int row = 0; row < 5; row++)
        for (int col = 0; col < 3; col++)
            if (g[row] & (4 >> col))
                FillRect(x + col * scale, y + row * scale, scale, scale, c);
}

void Raster::DrawText(int x, int y, const char* s, uint32_t c, int scale) {
    int cx = x;
    for (const char* p = s; *p; p++) {
        if (*p == '\n') { y += 6 * scale; cx = x; continue; }
        DrawChar(cx, y, *p, c, scale);
        cx += 4 * scale;
    }
}

int Raster::TextWidth(const char* s, int scale) {
    int n = 0;
    for (const char* p = s; *p; p++) if (*p != '\n') n++;
    return n * 4 * scale - scale;
}

void Raster::DrawTextCentered(int cx, int y, const char* s, uint32_t c, int scale) {
    DrawText(cx - TextWidth(s, scale) / 2, y, s, c, scale);
}

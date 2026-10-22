#pragma once
#include <cstdint>

// Tiny 3x5 bitmap font. Returns false for unknown glyphs (drawn blank).
inline bool GetGlyph(char ch, uint8_t out[5]) {
    static const uint8_t F[26][5] = {
        {2,5,7,5,5},{6,5,6,5,6},{3,4,4,4,3},   // A B C
        {6,5,5,5,6},{7,4,6,4,7},{7,4,6,4,4},   // D E F
        {3,4,5,5,3},{5,5,7,5,5},{7,2,2,2,7},   // G H I
        {1,1,1,5,2},{5,5,6,5,5},{4,4,4,4,7},   // J K L
        {5,7,7,5,5},{6,5,5,5,5},{2,5,5,5,2},   // M N O
        {6,5,6,4,4},{2,5,5,6,3},{6,5,6,5,5},   // P Q R
        {3,4,2,1,6},{7,2,2,2,2},{5,5,5,5,7},   // S T U
        {5,5,5,5,2},{5,5,7,7,5},{5,5,2,5,5},   // V W X
        {5,5,2,2,2},{7,1,2,4,7},               // Y Z
    };
    static const uint8_t N[10][5] = {
        {7,5,5,5,7},{2,6,2,2,7},{7,1,7,4,7},{7,1,3,1,7},
        {5,5,7,1,1},{7,4,7,1,7},{7,4,7,5,7},{7,1,1,2,2},
        {7,5,7,5,7},{7,5,7,1,7},
    };
    for (int i = 0; i < 5; i++) out[i] = 0;
    if (ch >= 'A' && ch <= 'Z') { for (int i = 0; i < 5; i++) out[i] = F[ch - 'A'][i]; return true; }
    if (ch >= 'a' && ch <= 'z') { for (int i = 0; i < 5; i++) out[i] = F[ch - 'a'][i]; return true; }
    if (ch >= '0' && ch <= '9') { for (int i = 0; i < 5; i++) out[i] = N[ch - '0'][i]; return true; }
    switch (ch) {
        case ' ': return true;
        case ':': out[1] = 2; out[3] = 2; return true;
        case '.': out[4] = 2; return true;
        case '!': out[0] = 2; out[1] = 2; out[2] = 2; out[4] = 2; return true;
        case '/': out[0] = 1; out[1] = 1; out[2] = 2; out[3] = 4; out[4] = 4; return true;
        case '-': out[2] = 7; return true;
        case '+': out[1] = 2; out[2] = 7; out[3] = 2; return true;
        case '>': out[1] = 4; out[2] = 2; out[3] = 4; return true;
        case '<': out[1] = 1; out[2] = 2; out[3] = 1; return true;
        case '_': out[4] = 7; return true;
        case '(': out[0] = 1; out[1] = 2; out[2] = 2; out[3] = 2; out[4] = 1; return true;
        case ')': out[0] = 4; out[1] = 2; out[2] = 2; out[3] = 2; out[4] = 4; return true;
        default: return false;
    }
}

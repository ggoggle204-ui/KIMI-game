// Headless logic test: drives the car with scripted input, steps the sim,
// and dumps a PPM screenshot. Build:  g++ -O2 -std=c++11 -Isrc src/*.cpp tools/headless_test.cpp -o headless_test
#include "../game.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

int main() {
    Raster r;
    r.Resize(1280, 720);
    Game g;

    // Script: confirm level 1, hold gas for a while, wiggle steering.
    for (int f = 0; f < 60 * 30; f++) {
        Input in;
        in.gas = true;
        if (f == 0) in.menuConfirm = true;
        if (f > 120 && f < 200) in.left = true;
        if (f >= 200 && f < 260) in.right = true;
        if (f == 700) { in.gas = false; in.brake = true; }   // burnout phase
        if (f == 900) { in.gas = false; in.brake = false; }  // boost launch
        g.SetTime(g.Time() + 1.f / 60.f);
        g.Update(in);
    }
    g.Render(r);

    FILE* f = fopen("headless.ppm", "wb");
    if (!f) { perror("ppm"); return 1; }
    fprintf(f, "P6\n%d %d\n255\n", r.w, r.h);
    for (size_t i = 0; i < r.px.size(); i++) {
        uint32_t p = r.px[i];
        unsigned char rgb[3] = {(unsigned char)((p >> 16) & 255),
                                (unsigned char)((p >> 8) & 255),
                                (unsigned char)(p & 255)};
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);

    // sanity: framebuffer must not be a single flat color
    size_t distinct = 0;
    for (size_t i = 1; i < r.px.size(); i++) if (r.px[i] != r.px[0]) distinct++;
    printf("state=%d nonuniform_px=%zu/%zu\n", (int)g.State(), distinct, r.px.size());
    return distinct > 1000 ? 0 : 1;
}

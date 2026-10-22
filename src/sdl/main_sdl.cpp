// Desktop test build (SDL2). The PS3 build uses src/ps3/main_ps3.cpp instead.
#include <SDL.h>
#include <cstdio>
#include <cmath>
#include "../game.h"
#include "../audio_synth.h"

static const int W = 1280, H = 720;

static void audioCb(void* userdata, Uint8* stream, int len) {
    AudioSynth* s = (AudioSynth*)userdata;
    s->FillBuffer((int16_t*)stream, len / 4);
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* win = SDL_CreateWindow("Sky Circuit Precision Driver",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H, 0);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_PRESENTVSYNC);
    SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING, W, H);

    AudioSynth synth;
    SDL_AudioSpec want = {};
    want.freq = 48000; want.format = AUDIO_S16SYS; want.channels = 2;
    want.samples = 1024; want.callback = audioCb; want.userdata = &synth;
    SDL_AudioSpec got;
    if (SDL_OpenAudio(&want, &got) < 0) fprintf(stderr, "audio: %s\n", SDL_GetError());
    else SDL_PauseAudio(0);

    Raster raster;
    raster.Resize(W, H);
    Game game;
    game.SetSynth(&synth);

    SDL_GameController* pad = nullptr;
    for (int i = 0; i < SDL_NumJoysticks(); i++)
        if (SDL_IsGameController(i)) { pad = SDL_GameControllerOpen(i); break; }

    bool quit = false;
    bool pauseHeld = false, upHeld = false, downHeld = false, confHeld = false;
    Uint64 prev = SDL_GetPerformanceCounter();
    double acc = 0;
    const double STEP = 1.0 / 60.0;

    while (!quit) {
        Input in;
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) quit = true;
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE) quit = true;
        }
        const Uint8* k = SDL_GetKeyboardState(nullptr);
        in.left  = k[SDL_SCANCODE_LEFT]  || k[SDL_SCANCODE_A];
        in.right = k[SDL_SCANCODE_RIGHT] || k[SDL_SCANCODE_D];
        in.gas   = k[SDL_SCANCODE_UP]    || k[SDL_SCANCODE_W];
        in.brake = k[SDL_SCANCODE_DOWN]  || k[SDL_SCANCODE_S];
        in.hbrake = k[SDL_SCANCODE_SPACE];

        bool pauseNow = k[SDL_SCANCODE_P] || k[SDL_SCANCODE_RETURN2];
        in.pausePressed = pauseNow && !pauseHeld;
        pauseHeld = pauseNow;

        bool upNow = k[SDL_SCANCODE_UP] || k[SDL_SCANCODE_W];
        bool dnNow = k[SDL_SCANCODE_DOWN] || k[SDL_SCANCODE_S];
        in.menuUp = upNow && !upHeld; upHeld = upNow;
        in.menuDown = dnNow && !downHeld; downHeld = dnNow;
        bool confNow = k[SDL_SCANCODE_RETURN] || k[SDL_SCANCODE_KP_ENTER];
        in.menuConfirm = confNow && !confHeld; confHeld = confNow;
        in.menuBack = k[SDL_SCANCODE_BACKSPACE];

        if (pad) {
            Sint16 ax = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTX);
            in.left  = in.left  || ax < -8000 || SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
            in.right = in.right || ax >  8000 || SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
            in.gas   = in.gas   || SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_A);
            in.brake = in.brake || SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_X);
            in.hbrake = in.hbrake || SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_B);
            bool pNow = SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_START);
            in.pausePressed = in.pausePressed || (pNow && !pauseHeld);
            bool mUp = SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_UP);
            bool mDn = SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
            bool mCf = SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_A);
            in.menuUp = in.menuUp || (mUp && !upHeld);
            in.menuDown = in.menuDown || (mDn && !downHeld);
            in.menuConfirm = in.menuConfirm || (mCf && !confHeld);
            in.menuBack = in.menuBack || SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_BACK);
        }

        Uint64 now = SDL_GetPerformanceCounter();
        acc += (double)(now - prev) / SDL_GetPerformanceFrequency();
        prev = now;
        if (acc > 0.25) acc = 0.25;
        while (acc >= STEP) {
            game.SetTime(game.Time() + (float)STEP);
            game.Update(in);
            acc -= STEP;
        }

        game.Render(raster);
        SDL_UpdateTexture(tex, nullptr, raster.px.data(), W * 4);
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, nullptr, nullptr);
        SDL_RenderPresent(ren);
    }

    SDL_CloseAudio();
    if (pad) SDL_GameControllerClose(pad);
    SDL_DestroyTexture(tex); SDL_DestroyRenderer(ren); SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}

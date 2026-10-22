// PS3 (PSL1GHT) platform layer for Super Slim / all PS3 models via HEN/CFW.
//
// Display : software-rendered 720p ARGB -> uploaded as a tiny3d texture, drawn as one quad.
// Input   : Sixaxis/DualShock 3 via libio.
// Audio   : 48 kHz stereo via libaudio (Estwald), fed from AudioSynth::FillBuffer.
//
// NOTE: exact function signatures of tiny3d / libaudio vary slightly between
// ps3libraries versions. If a symbol mismatches, check your headers (tiny3d.h, audio.h).

#include <psl1ght/lv2.h>
#include <sysutil/sysutil.h>
#include <io/pad.h>
#include <tiny3d.h>
#include <audio/audio.h>
#include <malloc.h>
#include <string.h>
#include <math.h>

#include "../game.h"
#include "../audio_synth.h"

#define SCREEN_W 1280
#define SCREEN_H 720

static bool gQuit = false;
static void sysutilCallback(u64 status, u64 param, void* user_data) {
    (void)param; (void)user_data;
    if (status == SYSUTIL_EXIT_GAME) gQuit = true;
}

// libaudio feed state
static AudioSynth gSynth;
static int gAudioPort = -1;
static audioPortConfig gAudioCfg;
static int16_t gAudioBuf[4096 * 2];

static void AudioInit() {
    audioPortParam params;
    memset(&params, 0, sizeof(params));
    params.numChannels = 2;
    params.numBlocks   = AUDIO_BLOCK_8;          // 8 blocks in the ring
    params.blockSize   = AUDIO_BLOCK_SAMPLES_512 * 4; // stereo int16
    params.attr        = 0;
    params.level       = 1.0f;

    if (audioInit() == 0 && audioPortOpen(&params, &gAudioCfg) == 0) {
        gAudioPort = gAudioCfg.portNum;
        audioPortStart(gAudioPort);
    }
}

static void AudioPump() {
    if (gAudioPort < 0) return;
    // Fill every block the audio driver has consumed.
    u32* readIdx = (u32*)gAudioCfg.readIndex;    // available block count
    int avail = *readIdx;
    while (avail-- > 0) {
        gSynth.FillBuffer(gAudioBuf, AUDIO_BLOCK_SAMPLES_512);
        audioAddData(gAudioPort, gAudioBuf, gAudioCfg.blockSize, 1);
    }
}

static void AudioShutdown() {
    if (gAudioPort >= 0) { audioPortStop(gAudioPort); audioPortClose(gAudioPort); }
    audioQuit();
}

// --- pad state with edge detection ---
struct PadState {
    u16 prev[2];
    bool held(u16 mask) const { return (cur[0] & mask) != 0; }
    bool pressed(u16 mask) const { return (cur[0] & mask) != 0 && (prev[0] & mask) == 0; }
    u16 cur[2];
};

static PadState gPad;

static void PollPad(Input& in) {
    memset(&in, 0, sizeof(in));
    padInfo info;
    if (ioPadGetInfo(&info) != 0 || !info.status[0]) return;

    padData data;
    if (ioPadGetData(0, &data) != 0) return;
    gPad.prev[0] = gPad.cur[0]; gPad.prev[1] = gPad.cur[1];
    gPad.cur[0] = data.button[0]; gPad.cur[1] = data.button[1];

    in.left  = gPad.held(PAD_CTRL_LEFT);
    in.right = gPad.held(PAD_CTRL_RIGHT);
    in.gas   = gPad.held(PAD_CTRL_CROSS);
    in.brake = gPad.held(PAD_CTRL_SQUARE);
    in.hbrake = gPad.held(PAD_CTRL_CIRCLE);

    in.menuUp     = gPad.pressed(PAD_CTRL_UP);
    in.menuDown   = gPad.pressed(PAD_CTRL_DOWN);
    in.menuConfirm = gPad.pressed(PAD_CTRL_CROSS);
    in.menuBack   = gPad.pressed(PAD_CTRL_SELECT);
    in.pausePressed = gPad.pressed(PAD_CTRL_START);
}

int main() {
    ioPadInit();
    sysUtilRegisterCallback(SYSUTIL_EVENT_SLOT0, sysutilCallback, NULL);

    tiny3d_Init(1024 * 1024);
    tiny3d_Clear(0xff000000, 0x00000000);
    tiny3d_Flip();

    u32 texOffset = tiny3d_AllocTexture((u32)(SCREEN_W * SCREEN_H * 4));

    Raster raster;
    raster.Resize(SCREEN_W, SCREEN_H);

    Game game;
    game.SetSynth(&gSynth);
    gPad.cur[0] = gPad.cur[1] = gPad.prev[0] = gPad.prev[1] = 0;

    AudioInit();

    while (!gQuit) {
        sysUtilCheckCallback();
        Input in;
        PollPad(in);

        game.SetTime(game.Time() + 1.f / 60.f);   // fixed 60 Hz logic
        game.Update(in);

        game.Render(raster);
        AudioPump();

        // upload framebuffer and blit it as a single full-screen quad
        tiny3d_DmaTexture(texOffset, raster.px.data(), (u32)(SCREEN_W * SCREEN_H * 4));
        tiny3d_Clear(0xff000000, 0x00000000);

        tiny3d_Project2D();
        tiny3d_SetTexture(0, texOffset, SCREEN_W, SCREEN_H, SCREEN_W * 4,
                          TINY3D_TEX_FORMAT_A8R8G8B8, 1);
        tiny3d_SetPolygon(TINY3D_QUADS);
        tiny3d_VertexColor(0xffffffff);
        tiny3d_VertexTexture(0.0f, 0.0f); tiny3d_VertexPos(0.0f,          0.0f,          0.0f);
        tiny3d_VertexTexture(1.0f, 0.0f); tiny3d_VertexPos((float)SCREEN_W, 0.0f,        0.0f);
        tiny3d_VertexTexture(1.0f, 1.0f); tiny3d_VertexPos((float)SCREEN_W, (float)SCREEN_H, 0.0f);
        tiny3d_VertexTexture(0.0f, 1.0f); tiny3d_VertexPos(0.0f,          (float)SCREEN_H, 0.0f);
        tiny3d_Draw();

        tiny3d_Flip();
    }

    AudioShutdown();
    ioPadEnd();
    tiny3d_End();
    return 0;
}

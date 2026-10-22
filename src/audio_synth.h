#pragma once
#include <cstdint>
#include <vector>

// Pure-software SFX synthesizer, 48 kHz stereo int16.
// Platform layer calls FillBuffer() from its audio thread/callback.
class AudioSynth {
public:
    void UpdateEngine(float speedRatio, bool accelerating);
    void StopEngine();
    void SetScreech(float level);          // 0..1
    void PlayBoost();
    void PlayBombBeep(float progress);     // 0..1
    void PlayExplosion();
    void PlayBump();
    void PlayJump();
    void PlayFall();

    void FillBuffer(int16_t* out, int frames);   // stereo interleaved

private:
    struct Voice {
        int type;          // 0=beep 1=sine-up 2=tri-down 3=boost 4=explosion
        float t, dur, f0, f1, gain;
        float lp, sub, nstate;
    };
    std::vector<Voice> voices;

    float engPh1 = 0.f, engPh2 = 0.f, engLp = 0.f, engFreq = 40.f, engGain = 0.f;
    float scGain = 0.f, scLp1 = 0.f, scLp2 = 0.f, scLevel = 0.f;

    static float frand();
    static float clamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
};

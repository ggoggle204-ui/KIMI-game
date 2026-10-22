#include "audio_synth.h"
#include <cmath>

static const float SR = 48000.f;
static const float DT = 1.f / SR;

float AudioSynth::frand() { return (float)rand() / (float)RAND_MAX * 2.f - 1.f; }

void AudioSynth::UpdateEngine(float speedRatio, bool accelerating) {
    float r = fabsf(speedRatio);
    float target = 40.f + fminf(r, 1.5f) * 110.f + (accelerating ? 15.f : 0.f);
    engFreq += (target - engFreq) * 0.08f;
    float tg = (r > 0.02f) ? 0.04f + fminf(r, 1.5f) * 0.05f : 0.02f;
    engGain += (tg - engGain) * 0.08f;
}

void AudioSynth::StopEngine() { engGain = 0.f; }

void AudioSynth::SetScreech(float level) {
    scLevel = level;
    float atk = level > 0.f ? 0.03f : 0.05f;
    scGain += (0.12f * level - scGain) * atk;
}

void AudioSynth::PlayBoost()     { voices.push_back({3, 0.f, 0.4f, 150.f, 700.f, 0.14f, 0, 0, 0}); }
void AudioSynth::PlayJump()      { voices.push_back({1, 0.f, 0.3f, 120.f, 320.f, 0.15f, 0, 0, 0}); }
void AudioSynth::PlayFall()      { voices.push_back({2, 0.f, 0.4f, 200.f, 30.f, 0.12f, 0, 0, 0}); }
void AudioSynth::PlayBump()      { voices.push_back({2, 0.f, 0.12f, 80.f, 20.f, 0.15f, 0, 0, 0}); }
void AudioSynth::PlayBombBeep(float p) {
    voices.push_back({0, 0.f, 0.06f, 1600.f + p * 1400.f, 1000.f + p * 1000.f, 0.08f + p * 0.12f, 0, 0, 0});
}

void AudioSynth::PlayExplosion() {
    Voice v; v.type = 4; v.t = 0.f; v.dur = 2.4f; v.f0 = 120.f; v.f1 = 30.f;
    v.gain = 0.9f; v.lp = 0.f; v.sub = 0.f; v.nstate = 0.f;
    voices.push_back(v);
}

void AudioSynth::FillBuffer(int16_t* out, int frames) {
    for (int i = 0; i < frames; i++) {
        float s = 0.f;

        // --- engine hum: triangle + sub saw through a one-pole lowpass ---
        if (engGain > 0.0001f) {
            engPh1 += engFreq * DT; if (engPh1 >= 1.f) engPh1 -= 1.f;
            engPh2 += engFreq * 0.5f * DT; if (engPh2 >= 1.f) engPh2 -= 1.f;
            float tri = 4.f * fabsf(engPh1 - 0.5f) - 1.f;
            float saw = 2.f * engPh2 - 1.f;
            engLp += 0.18f * ((tri + saw * 0.5f) - engLp);
            s += engLp * engGain;
        }

        // --- tyre screech: noise through two cascaded one-poles (bandpass-ish) ---
        if (scGain > 0.0001f) {
            float n = frand();
            float b = 0.35f;
            scLp1 += b * (n - scLp1);
            scLp2 += b * (scLp1 - scLp2);
            s += (scLp1 - scLp2) * scGain * 6.f;
        }

        // --- one-shot voices ---
        for (size_t v = 0; v < voices.size(); v++) {
            Voice& V = voices[v];
            float env = 1.f - V.t / V.dur;
            float f = V.f0 + (V.f1 - V.f0) * (V.t / V.dur);
            switch (V.type) {
                case 0: { // beep: square, pitch slides down
                    V.nstate += f * DT; if (V.nstate >= 1.f) V.nstate -= 1.f;
                    s += (V.nstate < 0.5f ? 1.f : -1.f) * V.gain * env;
                    break; }
                case 1: { // sine sweep up
                    V.nstate += f * DT; if (V.nstate >= 1.f) V.nstate -= 1.f;
                    s += sinf(2.f * 3.14159265f * V.nstate) * V.gain * env;
                    break; }
                case 2: { // triangle sweep down
                    V.nstate += f * DT; if (V.nstate >= 1.f) V.nstate -= 1.f;
                    float tri = 4.f * fabsf(V.nstate - 0.5f) - 1.f;
                    s += tri * V.gain * env;
                    break; }
                case 3: { // boost: saw sweep up
                    V.nstate += f * DT; if (V.nstate >= 1.f) V.nstate -= 1.f;
                    s += (2.f * V.nstate - 1.f) * V.gain * env;
                    break; }
                case 4: { // explosion: decaying noise (lowpassed) + sub sine
                    float n = frand();
                    V.lp += 0.05f * (n - V.lp);
                    float noise = V.lp * expf(-V.t * 1.6f) * 2.2f;
                    V.sub += f * DT; if (V.sub >= 1.f) V.sub -= 1.f;
                    float sub = sinf(2.f * 3.14159265f * V.sub) * expf(-V.t * 2.5f) * 0.7f;
                    s += (noise + sub) * V.gain;
                    break; }
            }
            V.t += DT;
            if (V.t >= V.dur) { voices.erase(voices.begin() + v); v--; }
        }

        s = clamp(s, -1.f, 1.f);
        int16_t sample = (int16_t)(s * 32000.f);
        out[2 * i] = sample;
        out[2 * i + 1] = sample;
    }
}

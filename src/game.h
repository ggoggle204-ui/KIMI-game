#pragma once
#include <vector>
#include <string>
#include "raster.h"

struct Seg  { float x1, y1, x2, y2; bool ramp; };
struct Gap  { float x, y, radius, minSpeed; };
struct Bump { float x, y; int hitCooldown; };
struct SmokeP { float x, y, r; int life; };
struct Cloud  { float x, y, r; };

enum class GameState { LevelSelect, Playing, Paused, Dead, ExplodedPost };

struct Input {
    bool left = false, right = false, gas = false, brake = false, hbrake = false;
    bool menuUp = false, menuDown = false, menuConfirm = false, menuBack = false;
    bool pausePressed = false;   // edge-triggered by platform
};

class AudioSynth;

class Game {
public:
    Game();

    void SetSynth(AudioSynth* s) { sfx = s; }
    void SetTime(float t) { timeSec = t; }
    float Time() const { return timeSec; }

    void LoadLevel(int lvl);
    void RespawnCheckpoint();
    void RestartTrack();

    void Update(const Input& in);   // fixed 60 Hz step
    void Render(Raster& r) const;

    GameState State() const { return state; }

private:
    // --- tunables (from the HTML5 original) ---
    static constexpr float BASE_ROAD_WIDTH = 55.f;
    static constexpr float CAR_WIDTH = 24.f;
    static constexpr float CAR_LENGTH = 72.f;
    static constexpr float GAP_REQ_SPEED = 4.8f;
    static constexpr float BOOST_MULT = 2.0f;
    static constexpr float BOOST_MIN_CHARGE = 0.2f;
    static constexpr float TIME_TO_EXPLOSION = 5.0f;
    static constexpr float OVERSPEED_DECAY = 0.98f;

    // --- track ---
    std::vector<Seg> segs;
    std::vector<Gap> gaps;
    std::vector<Bump> bumps;
    float bMinX = 0, bMaxX = 0, bMinY = 0, bMaxY = 0;
    int level = 1;
    bool halfRoad = false;
    float RoadWidth() const { return halfRoad ? BASE_ROAD_WIDTH * 0.5f : BASE_ROAD_WIDTH; }

    // --- car ---
    float cx = 0, cy = 0, cangle = 0, cspeed = 0;
    float cscale = 1, cvert = 0;
    bool airborne = false;
    int airTimer = 0;
    int wheelsOnRoad = 4;

    // --- run state ---
    GameState state = GameState::LevelSelect;
    int menuSel = 0;
    float timeSec = 0;
    std::string deathReason = "FELL OFF INTO THE SKY!";
    std::string statusText = "ROAD OK";
    uint32_t statusColor = RGBA(0x27, 0xAE, 0x60);

    int cpTimer = 0;
    struct { float x, y, angle, speed; } checkpoint = {0, 0, 0, 0};

    bool isStuck = false;
    float stuckLeft = 3.f;
    bool danger = false;

    float burnoutTimer = 0.f;
    bool burnoutRocking = false;
    float rockWobble = 0.f;
    float sinceBeep = 0.f;
    float explosionAnim = 0.f;     // >0 while fireball grows
    float postDelay = 0.f;         // delay before death menu after explosion
    bool exploded = false;

    std::vector<SmokeP> smoke;
    std::vector<Cloud> clouds;

    AudioSynth* sfx = nullptr;

    float randf();
    void BuildLevelTrack(int lvl);
    void AddWavySegment(float x1, float y1, float x2, float y2, int waves);
    void AddSmoothArcSegment(float x1, float y1, float x2, float y2, float cx_, float cy_);
    void AddGapJumpSegment(float x1, float y1, float x2, float y2, float gapSize, float minSpeed);
    void UpdateBounds();

    float DistToSeg(float px, float py, const Seg& s) const;
    float MinDistToRoad(float px, float py) const;
    bool  PointOnRoad(float px, float py) const;

    struct Wheels { float flx, fly, frx, fry, blx, bly, brx, bry; };
    Wheels GetWheels() const;
    struct Status { int wheels; bool comOnRoad, stuck, failing, danger; };
    Status EvalStatus() const;

    void EmitSmoke(int perWheel);
    void UpdateSmoke();
    void Die(const char* reason);
    void ResetRun();

    // --- drawing helpers (screen space) ---
    mutable float camX = 0, camY = 0;
    void DrawWorld(Raster& r) const;
    void DrawCar(Raster& r) const;
    void DrawHUD(Raster& r) const;
    void DrawMinimap(Raster& r) const;
    void DrawMenu(Raster& r, const char* title, const std::vector<std::string>& items, uint32_t titleCol) const;
};

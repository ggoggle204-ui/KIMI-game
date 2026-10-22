#include "game.h"
#include "audio_synth.h"
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <algorithm>

float Game::randf() { return (float)rand() / (float)RAND_MAX; }

Game::Game() {
    srand(12345);
    for (int i = 0; i < 30; i++)
        clouds.push_back({(randf() - 0.5f) * 6000.f, (randf() - 0.5f) * 6000.f, 40.f + randf() * 60.f});
    BuildLevelTrack(1);
    state = GameState::LevelSelect;
    menuSel = 0;
}

// ============================== TRACK BUILDING ==============================

void Game::UpdateBounds() {
    float mnx = 1e30f, mxx = -1e30f, mny = 1e30f, mxy = -1e30f;
    for (auto& s : segs) {
        mnx = std::min(mnx, std::min(s.x1, s.x2)); mxx = std::max(mxx, std::max(s.x1, s.x2));
        mny = std::min(mny, std::min(s.y1, s.y2)); mxy = std::max(mxy, std::max(s.y1, s.y2));
    }
    bMinX = mnx - 100; bMaxX = mxx + 100; bMinY = mny - 100; bMaxY = mxy + 100;
}

void Game::AddWavySegment(float x1, float y1, float x2, float y2, int waves) {
    const int steps = 30;
    float dx = (x2 - x1) / steps, dy = (y2 - y1) / steps;
    float nx = -dy, ny = dx;
    float len = sqrtf(nx * nx + ny * ny);
    nx /= len; ny /= len;
    float px = x1, py = y1;
    for (int i = 1; i <= steps; i++) {
        float prog = (float)i / steps;
        float off = sinf(prog * 3.14159265f * 2.f * waves) * 100.f;
        float cx_ = x1 + dx * i + nx * off, cy_ = y1 + dy * i + ny * off;
        segs.push_back({px, py, cx_, cy_, false});
        px = cx_; py = cy_;
    }
}

void Game::AddSmoothArcSegment(float x1, float y1, float x2, float y2, float kx, float ky) {
    const int steps = 25;
    float px = x1, py = y1;
    for (int i = 1; i <= steps; i++) {
        float t = (float)i / steps;
        float a = (1 - t) * (1 - t), b = 2 * (1 - t) * t, c = t * t;
        float cx_ = a * x1 + b * kx + c * x2, cy_ = a * y1 + b * ky + c * y2;
        segs.push_back({px, py, cx_, cy_, false});
        px = cx_; py = cy_;
    }
}

void Game::AddGapJumpSegment(float x1, float y1, float x2, float y2, float gapSize, float minSpeed) {
    float midX = (x1 + x2) / 2, midY = (y1 + y2) / 2;
    float dirX = x2 - x1, dirY = y2 - y1;
    float len = sqrtf(dirX * dirX + dirY * dirY);
    float ndx = dirX / len, ndy = dirY / len;
    float r1x = midX - ndx * gapSize / 2, r1y = midY - ndy * gapSize / 2;
    float r2x = midX + ndx * gapSize / 2, r2y = midY + ndy * gapSize / 2;
    segs.push_back({x1, y1, r1x, r1y, true});
    segs.push_back({r2x, r2y, x2, y2, false});
    gaps.push_back({r1x, r1y, 45.f, minSpeed});
}

void Game::BuildLevelTrack(int lvl) {
    segs.clear(); gaps.clear(); bumps.clear();
    level = lvl;

    if (lvl == 1) {
        const float nx[10] = {0, 1000, 1000, 1800, 1800, 800, 800, -600, -600, 0};
        const float ny[10] = {0, 0, 800, 800, 1800, 1800, 2600, 2600, 1200, 1200};
        for (int i = 0; i < 10; i++) {
            int j = (i + 1) % 10;
            if (i == 1) AddWavySegment(nx[i], ny[i], nx[j], ny[j], 3);
            else if (i == 3) AddGapJumpSegment(nx[i], ny[i], nx[j], ny[j], 100, GAP_REQ_SPEED);
            else if (i == 6) AddSmoothArcSegment(nx[i], ny[i], nx[j], ny[j], 0, 2600);
            else segs.push_back({nx[i], ny[i], nx[j], ny[j], false});
        }
        bumps.push_back({450, 0, 0});
    } else if (lvl == 2) {
        const float nx[10] = {0, 1200, 1200, 400, 400, 1600, 1600, -400, -400, 0};
        const float ny[10] = {0, 0, 1200, 1200, 2000, 2000, 3200, 3200, 1600, 1600};
        for (int i = 0; i < 10; i++) {
            int j = (i + 1) % 10;
            if (i == 0) AddGapJumpSegment(nx[i], ny[i], nx[j], ny[j], 100, GAP_REQ_SPEED);
            else if (i == 4) AddGapJumpSegment(nx[i], ny[i], nx[j], ny[j], 100, GAP_REQ_SPEED);
            else if (i == 7) AddWavySegment(nx[i], ny[i], nx[j], ny[j], 2);
            else segs.push_back({nx[i], ny[i], nx[j], ny[j], false});
        }
        bumps.push_back({1200, 600, 0});
        bumps.push_back({-400, 2400, 0});
    } else {
        const float nx[10] = {0, 1400, 1400, 2400, 2400, 1200, 1200, -800, -800, 0};
        const float ny[10] = {0, 0, 1000, 1000, 2200, 2200, 3200, 3200, 1400, 1400};
        for (int i = 0; i < 10; i++) {
            int j = (i + 1) % 10;
            if (i == 0) AddGapJumpSegment(nx[i], ny[i], nx[j], ny[j], 100, GAP_REQ_SPEED);
            else if (i == 1) AddWavySegment(nx[i], ny[i], nx[j], ny[j], 5);
            else if (i == 3) AddGapJumpSegment(nx[i], ny[i], nx[j], ny[j], 100, GAP_REQ_SPEED);
            else if (i == 5) AddGapJumpSegment(nx[i], ny[i], nx[j], ny[j], 100, GAP_REQ_SPEED);
            else if (i == 7) AddWavySegment(nx[i], ny[i], nx[j], ny[j], 4);
            else if (i == 8) AddGapJumpSegment(nx[i], ny[i], nx[j], ny[j], 100, GAP_REQ_SPEED);
            else segs.push_back({nx[i], ny[i], nx[j], ny[j], false});
        }
        bumps.push_back({2400, 1600, 0});
        bumps.push_back({-800, 2300, 0});
    }
    UpdateBounds();
}

// ============================== GEOMETRY ==============================

float Game::DistToSeg(float px, float py, const Seg& s) const {
    float vx = s.x2 - s.x1, vy = s.y2 - s.y1;
    float l2 = vx * vx + vy * vy;
    float t = l2 > 0 ? ((px - s.x1) * vx + (py - s.y1) * vy) / l2 : 0.f;
    t = std::max(0.f, std::min(1.f, t));
    float dx = px - (s.x1 + t * vx), dy = py - (s.y1 + t * vy);
    return sqrtf(dx * dx + dy * dy);
}

float Game::MinDistToRoad(float px, float py) const {
    float m = 1e30f;
    for (auto& s : segs) { float d = DistToSeg(px, py, s); if (d < m) m = d; }
    return m;
}

bool Game::PointOnRoad(float px, float py) const {
    return MinDistToRoad(px, py) <= RoadWidth() / 2.f;
}

Game::Wheels Game::GetWheels() const {
    float hw = CAR_WIDTH / 2, hh = CAR_LENGTH / 2;
    float cs = cosf(cangle), sn = sinf(cangle);
    auto tr = [&](float lx, float ly, float& ox, float& oy) {
        ox = cx + (lx * cs - ly * sn);
        oy = cy + (lx * sn + ly * cs);
    };
    Wheels w;
    tr(-hw, -hh, w.flx, w.fly); tr(hw, -hh, w.frx, w.fry);
    tr(-hw, hh, w.blx, w.bly);  tr(hw, hh, w.brx, w.bry);
    return w;
}

Game::Status Game::EvalStatus() const {
    if (airborne) return {0, true, false, false, false};
    Wheels w = GetWheels();
    bool fl = PointOnRoad(w.flx, w.fly), fr = PointOnRoad(w.frx, w.fry);
    bool bl = PointOnRoad(w.blx, w.bly), br = PointOnRoad(w.brx, w.bry);
    int wheels = (fl?1:0) + (fr?1:0) + (bl?1:0) + (br?1:0);

    float halfW = RoadWidth() / 2.f;
    float cd = MinDistToRoad(cx, cy);
    bool com = cd <= halfW;
    bool dang = cd >= halfW * 0.60f;

    bool front = fl || fr, rear = bl || br;
    bool failing, stuck;
    if (com) {
        stuck = (wheels == 0);
        failing = (!front && !rear);
    } else {
        failing = !(front && rear);
        stuck = false;
    }
    return {wheels, com, stuck, failing, dang};
}

// ============================== RUN CONTROL ==============================

void Game::ResetRun() {
    cpTimer = 0;
    isStuck = false; stuckLeft = 3.f;
    burnoutTimer = 0.f; burnoutRocking = false; rockWobble = 0.f; sinceBeep = 0.f;
    explosionAnim = 0.f; postDelay = 0.f; exploded = false;
    smoke.clear();
    statusText = "ROAD OK"; statusColor = RGBA(0x27, 0xAE, 0x60);
    menuSel = 0;
}

void Game::LoadLevel(int lvl) {
    BuildLevelTrack(lvl);
    cx = 0; cy = 0; cangle = 0; cspeed = 0;
    airborne = false; cscale = 1; cvert = 0; wheelsOnRoad = 4;
    checkpoint = {0, 0, 0, 0};
    ResetRun();
    state = GameState::Playing;
}

void Game::RespawnCheckpoint() {
    cx = checkpoint.x; cy = checkpoint.y; cangle = checkpoint.angle; cspeed = 0;
    airborne = false; cscale = 1; cvert = 0; wheelsOnRoad = 4;
    ResetRun();
    state = GameState::Playing;
}

void Game::RestartTrack() { LoadLevel(level); }

void Game::Die(const char* reason) {
    state = GameState::Dead;
    isStuck = false;
    deathReason = reason;
    statusText = reason; statusColor = RGBA(0xD9, 0x38, 0x38);
    menuSel = 0;
    if (sfx) { sfx->SetScreech(0); sfx->PlayFall(); sfx->StopEngine(); }
}

void Game::EmitSmoke(int perWheel) {
    Wheels w = GetWheels();
    const float wx[4] = {w.flx, w.frx, w.blx, w.brx};
    const float wy[4] = {w.fly, w.fry, w.bly, w.bry};
    for (int k = 0; k < 4; k++)
        for (int i = 0; i < perWheel; i++)
            smoke.push_back({wx[k] + (randf() - 0.5f) * 6.f, wy[k] + (randf() - 0.5f) * 6.f,
                             4.f + randf() * 5.f, 25});
}

void Game::UpdateSmoke() {
    for (int i = (int)smoke.size() - 1; i >= 0; i--) {
        smoke[i].life--;
        smoke[i].r += 0.2f;
        if (smoke[i].life <= 0) smoke.erase(smoke.begin() + i);
    }
}

// ============================== UPDATE ==============================

void Game::Update(const Input& in) {
    const float DT = 1.f / 60.f;

    // ---------- menus ----------
    if (state == GameState::LevelSelect) {
        if (in.menuUp)   menuSel = (menuSel + 2) % 3;
        if (in.menuDown) menuSel = (menuSel + 1) % 3;
        if (in.menuConfirm) LoadLevel(menuSel + 1);
        return;
    }
    if (state == GameState::Paused) {
        if (in.pausePressed || in.menuBack) { state = GameState::Playing; return; }
        if (in.menuUp)   menuSel = (menuSel + 2) % 3;
        if (in.menuDown) menuSel = (menuSel + 1) % 3;
        if (in.menuConfirm) {
            if (menuSel == 0) state = GameState::Playing;
            else if (menuSel == 1) halfRoad = !halfRoad;
            else { state = GameState::LevelSelect; menuSel = level - 1; }
        }
        return;
    }
    if (state == GameState::Dead) {
        if (in.menuUp)   menuSel = (menuSel + 2) % 3;
        if (in.menuDown) menuSel = (menuSel + 1) % 3;
        if (in.menuConfirm) {
            if (menuSel == 0) RespawnCheckpoint();
            else if (menuSel == 1) RestartTrack();
            else { state = GameState::LevelSelect; menuSel = level - 1; }
        }
        return;
    }
    if (state == GameState::ExplodedPost) {
        postDelay -= DT;
        if (postDelay <= 0.f) {
            if (in.menuUp)   menuSel = (menuSel + 2) % 3;
            if (in.menuDown) menuSel = (menuSel + 1) % 3;
            if (in.menuConfirm) {
                if (menuSel == 0) RespawnCheckpoint();
                else if (menuSel == 1) RestartTrack();
                else { state = GameState::LevelSelect; menuSel = level - 1; }
            }
        }
        return;
    }

    // ---------- Playing ----------
    if (in.pausePressed) { state = GameState::Paused; menuSel = 0;
        if (sfx) { sfx->SetScreech(0); sfx->StopEngine(); } return; }

    UpdateSmoke();

    Status st = EvalStatus();
    wheelsOnRoad = st.wheels;
    danger = st.danger;

    if (st.failing && !airborne) { Die("FELL OFF INTO THE SKY!"); return; }

    if (st.stuck && !airborne) {
        if (!isStuck) { isStuck = true; stuckLeft = 3.f; }
        stuckLeft -= DT;
        if (stuckLeft <= 0.f) { Die("VEHICLE STUCK!"); return; }
    } else {
        isStuck = false;
    }

    // ---------- burnout & explosion ----------
    bool bothPedals = in.gas && in.brake;

    // Boost: one pedal released while rocking (blocked if handbrake held)
    if (burnoutRocking && !bothPedals && !in.hbrake && !airborne &&
        wheelsOnRoad >= 1 && burnoutTimer >= BOOST_MIN_CHARGE) {
        int dir = (in.brake && !in.gas) ? -1 : 1;
        cangle -= rockWobble;   // launch along the heading the player aimed
        cspeed = dir * 8.f * (BOOST_MULT + std::min(1.f, burnoutTimer / TIME_TO_EXPLOSION) * 0.5f);
        if (sfx) sfx->PlayBoost();
        EmitSmoke(3);
    }
    if (!burnoutRocking) rockWobble = 0.f;
    burnoutRocking = false;

    if (bothPedals && !airborne && wheelsOnRoad >= 1) {
        if (fabsf(cspeed) > 0.5f) {
            float bd = cspeed > 0 ? 1.f : -1.f;
            cspeed -= bd * 0.16f * 2.5f;
            if (bd > 0 && cspeed < 0) cspeed = 0;
            if (bd < 0 && cspeed > 0) cspeed = 0;
            burnoutTimer = 0.f; sinceBeep = 0.f;
        } else {
            burnoutRocking = true;
            if (sfx) { sfx->SetScreech(0); sfx->UpdateEngine(0.4f, true); }
            cspeed = 0;
            float wiggle = sinf(timeSec * 35.f) * 0.4f;
            float shake = cosf(timeSec * 25.f) * 0.03f;
            cangle += shake;
            rockWobble += shake;

            if (in.left)  cangle -= 0.012f + randf() * 0.008f;
            if (in.right) cangle += 0.012f + randf() * 0.008f;

            cx += sinf(cangle) * wiggle;
            cy -= cosf(cangle) * wiggle;

            EmitSmoke(1);

            burnoutTimer += DT;
            sinceBeep += DT;

            float prog = std::min(1.f, burnoutTimer / TIME_TO_EXPLOSION);
            if (sinceBeep >= 0.6f - prog * 0.55f) {
                if (sfx) sfx->PlayBombBeep(prog);
                sinceBeep = 0.f;
            }

            if (burnoutTimer >= TIME_TO_EXPLOSION) {
                burnoutRocking = false;
                if (sfx) { sfx->PlayExplosion(); sfx->StopEngine(); }
                explosionAnim = 0.6f;
                state = GameState::ExplodedPost;
                postDelay = 2.0f;
                deathReason = "VEHICLE EXPLODED FROM BURNOUT!";
                statusText = "EXPLODED!"; statusColor = RGBA(0xD9, 0x38, 0x38);
                menuSel = 0;
                return;
            }

            statusText = "BURNOUT!"; statusColor = RGBA(0xD9, 0x38, 0x38);
            return;   // skip the rest of physics while rocking
        }
    } else {
        burnoutTimer = 0.f; sinceBeep = 0.f;
    }

    // ---------- normal driving ----------
    bool canSteer = wheelsOnRoad >= 1 && !airborne && !isStuck;
    if (canSteer) {
        float dir = cspeed >= 0 ? 1.f : -1.f;
        if (in.left)  cangle -= 0.055f * dir;
        if (in.right) cangle += 0.055f * dir;
    }

    float power = (airborne || isStuck) ? 0.f : (float)wheelsOnRoad / 4.f;
    const float MAXS = 8.f, ACC = 0.16f, FRIC = 0.965f;

    if (in.hbrake && power > 0) {
        if (fabsf(cspeed) > 0.5f) {
            float bd = cspeed > 0 ? 1.f : -1.f;
            cspeed -= bd * ACC * 2.2f;
            if (bd > 0 && cspeed < 0) cspeed = 0;
            if (bd < 0 && cspeed > 0) cspeed = 0;
        } else cspeed = 0;
    } else if (bothPedals) {
        // burnout braking already handled above
    } else if (in.gas && power > 0) {
        if (cspeed < MAXS) cspeed = std::min(cspeed + ACC * power, MAXS);
    } else if (in.brake && power > 0) {
        if (cspeed > -MAXS) cspeed = std::max(cspeed - ACC * power, -MAXS);
    } else {
        cspeed *= FRIC;
    }

    if (fabsf(cspeed) > MAXS) cspeed *= OVERSPEED_DECAY;

    float absSpd = fabsf(cspeed);
    bool skidding = (in.hbrake || bothPedals) && power > 0 && absSpd > 0.5f;
    if (sfx) {
        sfx->SetScreech(skidding ? std::min(1.f, absSpd / 3.f) : 0.f);
        sfx->UpdateEngine(cspeed / MAXS, (in.gas || in.brake) && power > 0);
    }

    cx += sinf(cangle) * cspeed;
    cy -= cosf(cangle) * cspeed;

    if (!airborne) {
        for (auto& sb : bumps) {
            if (sb.hitCooldown > 0) sb.hitCooldown--;
            float d = sqrtf((cx - sb.x) * (cx - sb.x) + (cy - sb.y) * (cy - sb.y));
            if (d < 28 && sb.hitCooldown == 0) {
                cspeed *= 0.55f; cvert = -8; sb.hitCooldown = 20;
                if (sfx) sfx->PlayBump();
            }
        }
    }
    cvert *= 0.7f;

    for (auto& g : gaps) {
        float d = sqrtf((cx - g.x) * (cx - g.x) + (cy - g.y) * (cy - g.y));
        if (d < g.radius && !airborne) {
            float sp = fabsf(cspeed);
            if (sp >= g.minSpeed) { airborne = true; airTimer = 28; if (sfx) sfx->PlayJump(); }
            else if (sp >= g.minSpeed * 0.5f) { airborne = true; airTimer = 12; if (sfx) sfx->PlayJump(); }
        }
    }

    if (airborne) {
        airTimer--;
        cscale = 1.f + sinf((airTimer / 28.f) * 3.14159265f) * 0.4f;
        if (airTimer <= 0) { airborne = false; cscale = 1.f; }
    }

    cpTimer++;
    if (cpTimer >= 300) {
        if (wheelsOnRoad == 4 && !airborne)
            checkpoint = {cx, cy, cangle, cspeed};
        cpTimer = 0;
    }

    if (isStuck)            { statusText = "STUCK!";      statusColor = RGBA(0xD9, 0x38, 0x38); }
    else if (!canSteer && !airborne) { statusText = "NO TRACTION!"; statusColor = RGBA(0xE6, 0x7E, 0x22); }
    else if (airborne)      { statusText = "AIRBORNE!";   statusColor = RGBA(0x34, 0x98, 0xDB); }
    else                    { statusText = "ROAD OK";     statusColor = RGBA(0x27, 0xAE, 0x60); }
}

// ============================== RENDER ==============================

static void RotRect(Raster& r, float cx, float cy, float ang, float hw, float hh, uint32_t c) {
    float cs = cosf(ang), sn = sinf(ang);
    float x0 = -hw, y0 = -hh, x1 = hw, y1 = hh;
    float ax = cx + x0 * cs - y0 * sn, ay = cy + x0 * sn + y0 * cs;
    float bx = cx + x1 * cs - y0 * sn, by = cx + x1 * sn + y0 * cs;
    float ccx = cx + x1 * cs - y1 * sn, ccy = cy + x1 * sn + y1 * cs;
    float dx = cx + x0 * cs - y1 * sn, dy = cy + x0 * sn + y1 * cs;
    r.FillTri(ax, ay, bx, by, ccx, ccy, c);
    r.FillTri(ax, ay, ccx, ccy, dx, dy, c);
}

void Game::Render(Raster& r) const {
    camX = cx - r.w / 2.f;
    camY = cy - r.h / 2.f;
    DrawWorld(r);
    DrawHUD(r);
    DrawMinimap(r);

    if (state == GameState::LevelSelect)
        DrawMenu(r, "SELECT A TRACK", {"LVL 1: SKY HIGHWAY", "LVL 2: PRECISION CITY", "LVL 3: THE GAUNTLET"}, RGBA(0x1A, 0x25, 0x30));
    else if (state == GameState::Paused)
        DrawMenu(r, "PAUSED", {"RESUME", halfRoad ? "ROAD: HALF WIDTH" : "ROAD: FULL WIDTH", "CHANGE LEVEL"}, RGBA(0x1A, 0x25, 0x30));
    else if (state == GameState::Dead)
        DrawMenu(r, deathReason.c_str(), {"CHECKPOINT", "RESTART TRACK", "CHANGE LEVEL"}, RGBA(0xD9, 0x38, 0x38));
    else if (state == GameState::ExplodedPost && postDelay <= 0.f)
        DrawMenu(r, deathReason.c_str(), {"CHECKPOINT", "RESTART TRACK", "CHANGE LEVEL"}, RGBA(0xD9, 0x38, 0x38));
    else if (state == GameState::Playing && isStuck) {
        char buf[64];
        snprintf(buf, sizeof(buf), "STUCK! FAILING IN %ds", (int)ceilf(stuckLeft));
        int tw = r.TextWidth(buf, 3);
        r.FillRect(r.w / 2 - tw / 2 - 14, 50, tw + 28, 34, RGBA(0xD9, 0x38, 0x38, 0xF2));
        r.DrawTextCentered(r.w / 2, 58, buf, RGBA(255, 255, 255), 3);
    }
}

void Game::DrawWorld(Raster& r) const {
    // sky
    r.Clear(RGBA(0x70, 0xC5, 0xCE));
    for (auto& c : clouds)
        r.FillCircle(c.x - camX, c.y - camY, c.r, RGBA(255, 255, 255, 140));

    float rw = RoadWidth();
    auto SX = [&](float x) { return x - camX; };
    auto SY = [&](float y) { return y - camY; };

    // drop shadow
    for (auto& s : segs)
        r.ThickLine(SX(s.x1 + 15), SY(s.y1 + 15), SX(s.x2 + 15), SY(s.y2 + 15), rw + 8, RGBA(20, 45, 70, 64));
    // asphalt
    for (auto& s : segs)
        r.ThickLine(SX(s.x1), SY(s.y1), SX(s.x2), SY(s.y2), rw, RGBA(0x2C, 0x3E, 0x50));
    // red center stripe
    for (auto& s : segs)
        r.ThickLine(SX(s.x1), SY(s.y1), SX(s.x2), SY(s.y2), 4, RGBA(0xE7, 0x4C, 0x3C));
    // dashed yellow center line
    for (auto& s : segs) {
        float dx = s.x2 - s.x1, dy = s.y2 - s.y1;
        float len = sqrtf(dx * dx + dy * dy);
        if (len < 1e-3f) continue;
        float ux = dx / len, uy = dy / len;
        float pos = 0.f;
        while (pos < len) {
            float e = std::min(pos + 12.f, len);
            r.ThickLine(SX(s.x1 + ux * pos), SY(s.y1 + uy * pos),
                        SX(s.x1 + ux * e),   SY(s.y1 + uy * e), 2, RGBA(0xF1, 0xC4, 0x0F));
            pos += 24.f;
        }
    }
    // jump ramps
    for (auto& s : segs)
        if (s.ramp) r.FillCircle(SX(s.x2), SY(s.y2), rw / 2, RGBA(0xE6, 0x7E, 0x22));
    // speed bumps
    for (auto& sb : bumps) {
        r.FillRect((int)(SX(sb.x) - 12), (int)(SY(sb.y) - rw / 2), 24, (int)rw, RGBA(0xF1, 0xC4, 0x0F));
        for (float i = -rw / 2; i < rw / 2; i += 12)
            r.FillRect((int)(SX(sb.x) - 12), (int)(SY(sb.y) + i), 8, 6, RGBA(0x1A, 0x25, 0x30));
    }

    // smoke
    for (auto& p : smoke)
        r.FillCircle(SX(p.x), SY(p.y), p.r, RGBA(200, 200, 200, 128));

    DrawCar(r);
}

void Game::DrawCar(Raster& r) const {
    float sx = cx - camX, sy = cy - camY + cvert;

    if (state == GameState::ExplodedPost || (state == GameState::Dead && deathReason == "VEHICLE EXPLODED FROM BURNOUT!")) {
        RotRect(r, sx, sy, cangle, 15, 20, RGBA(0, 0, 0, 102));
        RotRect(r, sx, sy, cangle, 10, 14, RGBA(0x33, 0x33, 0x33));
        float pulse = sinf(timeSec * 12.f) * 6.f;
        r.FillCircle(sx, sy, 14 + pulse, RGBA(0xE7, 0x4C, 0x3C));
        r.FillCircle(sx - 2, sy + 2, 8 + pulse * 0.5f, RGBA(0xF1, 0xC4, 0x0F));
        return;
    }

    if (state == GameState::ExplodedPost && explosionAnim > 0.f) { /* unreachable, kept for clarity */ }

    RotRect(r, sx + 4, sy + 4, cangle, CAR_WIDTH / 2 * cscale, CAR_LENGTH / 2 * cscale, RGBA(0, 0, 0, 77));

    bool flash = (danger || isStuck) && ((int)(timeSec * 4) % 2 == 0);
    RotRect(r, sx, sy, cangle, CAR_WIDTH / 2 * cscale, CAR_LENGTH / 2 * cscale,
            flash ? RGBA(0xD9, 0x38, 0x38) : RGBA(0x1B, 0x36, 0x5D));
    // glass
    RotRect(r, sx, sy, cangle, (CAR_WIDTH / 2 - 3) * cscale, 14 * cscale, RGBA(0x11, 0x22, 0x33));
    // white stripe
    RotRect(r, sx, sy, cangle, CAR_WIDTH / 2 * cscale, 2 * cscale, RGBA(255, 255, 255));
    // lights
    float cs = cosf(cangle), sn = sinf(cangle);
    auto light = [&](float lx, float ly, uint32_t c) {
        r.FillRect((int)(sx + lx * cs - ly * sn) - 2, (int)(sy + lx * sn + ly * cs) - 2, 5, 4, c);
    };
    float hw = CAR_WIDTH / 2 * cscale, hh = CAR_LENGTH / 2 * cscale;
    light(-hw + 3, -hh + 2, RGBA(0xF1, 0xC4, 0x0F));
    light(hw - 3,  -hh + 2, RGBA(0xF1, 0xC4, 0x0F));
    light(-hw + 3, hh - 3, RGBA(0xF1, 0xC4, 0x0F));
    light(hw - 3,  hh - 3, RGBA(0xF1, 0xC4, 0x0F));
}

void Game::DrawHUD(Raster& r) const {
    char buf[96];
    uint32_t dark = RGBA(0x1A, 0x25, 0x30), white = RGBA(255, 255, 255, 204);
    snprintf(buf, sizeof(buf), "LEVEL: %d", level);
    r.DrawText(15, 15, buf, dark, 2);
    if (burnoutRocking) r.DrawText(15, 31, "SPEED: BURNOUT", dark, 2);
    else { snprintf(buf, sizeof(buf), "SPEED: %d km/h", (int)floorf(fabsf(cspeed) * 16)); r.DrawText(15, 31, buf, dark, 2); }
    snprintf(buf, sizeof(buf), "WHEELS ON ROAD: %d/4", airborne ? 0 : wheelsOnRoad);
    r.DrawText(15, 47, buf, dark, 2);
    r.DrawText(15, 63, statusText.c_str(), statusColor, 2);
    (void)white;
}

void Game::DrawMinimap(Raster& r) const {
    int mw = 120, mx = r.w - 135, my = 15;
    r.FillRect(mx, my, mw, mw, RGBA(26, 37, 48, 191));
    r.FillRect(mx, my, mw, 2, RGBA(255,255,255,220)); r.FillRect(mx, my+mw-2, mw, 2, RGBA(255,255,255,220));
    r.FillRect(mx, my, 2, mw, RGBA(255,255,255,220)); r.FillRect(mx+mw-2, my, 2, mw, RGBA(255,255,255,220));

    float w = bMaxX - bMinX, h = bMaxY - bMinY;
    float sc = std::min(100.f / w, 100.f / h);
    float ox = mx + (mw - w * sc) / 2, oy = my + (mw - h * sc) / 2;
    auto MX = [&](float x) { return ox + (x - bMinX) * sc; };
    auto MY = [&](float y) { return oy + (y - bMinY) * sc; };

    for (auto& s : segs)
        r.ThickLine(MX(s.x1), MY(s.y1), MX(s.x2), MY(s.y2), 3, RGBA(255, 255, 255, 102));
    if (checkpoint.x != 0 || checkpoint.y != 0)
        r.FillCircle(MX(checkpoint.x), MY(checkpoint.y), 4, RGBA(0x27, 0xAE, 0x60));
    r.FillCircle(MX(cx), MY(cy), 4, ((int)(timeSec * 4) % 2 == 0) ? RGBA(0xE7, 0x4C, 0x3C) : RGBA(255, 255, 255));
}

void Game::DrawMenu(Raster& r, const char* title, const std::vector<std::string>& items, uint32_t titleCol) const {
    int tw = r.TextWidth(title, 3);
    int pw = std::max(tw + 48, 340);
    int ph = 60 + (int)items.size() * 46;
    int px = r.w / 2 - pw / 2, py = r.h / 2 - ph / 2;
    r.FillRect(px + 6, py + 8, pw, ph, RGBA(0, 0, 0, 60));
    r.FillRect(px, py, pw, ph, RGBA(255, 255, 255, 235));
    r.FillRect(px, py, pw, 3, RGBA(0x1A, 0x25, 0x30));
    r.FillRect(px, py + ph - 3, pw, 3, RGBA(0x1A, 0x25, 0x30));
    r.FillRect(px, py, 3, ph, RGBA(0x1A, 0x25, 0x30));
    r.FillRect(px + pw - 3, py, 3, ph, RGBA(0x1A, 0x25, 0x30));
    r.DrawTextCentered(r.w / 2, py + 20, title, titleCol, 3);

    for (size_t i = 0; i < items.size(); i++) {
        int by = py + 62 + (int)i * 46;
        int bw = pw - 70, bx = r.w / 2 - bw / 2;
        uint32_t bc = (i == (size_t)menuSel) ? RGBA(0x1A, 0x25, 0x30) : RGBA(255, 255, 255);
        uint32_t tc = (i == (size_t)menuSel) ? RGBA(255, 255, 255) : RGBA(0x1A, 0x25, 0x30);
        r.FillRect(bx, by, bw, 34, bc);
        r.FillRect(bx, by, bw, 2, RGBA(0x1A, 0x25, 0x30));
        r.FillRect(bx, by + 32, bw, 2, RGBA(0x1A, 0x25, 0x30));
        r.FillRect(bx, by, 2, 34, RGBA(0x1A, 0x25, 0x30));
        r.FillRect(bx + bw - 2, by, 2, 34, RGBA(0x1A, 0x25, 0x30));
        r.DrawTextCentered(r.w / 2, by + 9, items[i].c_str(), tc, 2);
    }
}

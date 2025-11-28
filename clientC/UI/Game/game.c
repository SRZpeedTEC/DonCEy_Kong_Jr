#include "game.h"
#include "static_map.h"
#include "raylib.h"
#include <string.h>
#include <stdio.h>

#include "../Render/render.h"
#include "Logic/input.h"
#include "Logic/player.h"
#include "Logic/physics.h"
#include "Logic/map.h"
#include "Logic/crocodile.h"
#include "Logic/constants.h"
#include "Logic/fruit.h"
#include "Logic/collision.h"
#include "../../UtilsC/entities_tlv.h" 



// window/state
static int VW, VH, SCALE;
static RenderTexture2D rt;
static Texture2D g_bg = {0};
static float g_bgAlpha = 0.35f;

// player state
static Player gPlayer;

// crocodile state
static Crocodile gCrocs[MAX_CROCS];

// fruits state
static Fruit gFruits[MAX_FRUITS];

// total fruits picked in this run (debug counter)
static int gFruitsPickedCount = 0;

// round win state (handled only in game.c)
static bool gRoundWon = false;
static bool gRoundJustWon = false;
static bool gGameOver = false;
static uint8_t  gUiLives  = 0;
static uint32_t gUiScore  = 0;



// check if player rect contains the win point (109, 48)
static void game_check_win_condition(void) {
    if (gRoundWon) return;

    const int winX = 109;
    const int winY = 48;

    int px = gPlayer.x;
    int py = gPlayer.y;
    int pw = gPlayer.w;
    int ph = gPlayer.h;

    bool containsPoint =
        (winX >= px) && (winX < px + pw) &&
        (winY >= py) && (winY < py + ph);

    if (containsPoint) {
        gRoundWon = true;
        gRoundJustWon = true;
    }
}


// --- init / shutdown ---
void game_init(uint16_t vw, uint16_t vh, uint16_t scale) {
    VW = (int)vw; VH = (int)vh; SCALE = (int)scale;
    InitWindow(VW * SCALE, VH * SCALE, "Client");
    SetTargetFPS(60);
    gUiLives = 3; 
    gUiScore = 0;


    rt = LoadRenderTexture(VW, VH);
    SetTextureFilter(rt.texture, TEXTURE_FILTER_POINT); // crisp pixel-art

    render_init_assets();

    // start player box (will be replaced by sprites later)
    player_init(&gPlayer, 30, 195, 16, 16);

    // init croc state
    for (int i = 0; i < MAX_CROCS; ++i) {
        crocodile_init(&gCrocs[i], 8, 8);
    }

    // init fruit state
    for (int i = 0; i < MAX_FRUITS; i++){
        fruit_init(&gFruits[i], 8,8);
    }

    // total fruits picked in this run (debug counter)
    static int gFruitsPickedCount = 0;

}

void game_shutdown(void) {
    if (g_bg.id) { UnloadTexture(g_bg); g_bg.id = 0; }

    render_init_assets();

    UnloadRenderTexture(rt);
    CloseWindow();
}

void game_set_bg(const char* path, float alpha) {
    if (g_bg.id) { UnloadTexture(g_bg); g_bg.id = 0; }
    if (path && *path) {
        g_bg = LoadTexture(path);
        SetTextureFilter(g_bg, TEXTURE_FILTER_POINT);
    }
    if (alpha >= 0.f && alpha <= 1.f) g_bgAlpha = alpha;
}

// --- draw ---
void game_draw_static(const CP_Static* staticMap) {
    BeginTextureMode(rt);
        //ClearBackground(BLACK);

        // draw background texture (kept here to avoid exposing Texture2D in render.h)
        if (g_bg.id) {
            Rectangle src = (Rectangle){0, 0, (float)g_bg.width, (float)g_bg.height};
            Rectangle dst = (Rectangle){0, 0, (float)VW, (float)VH};
            DrawTexturePro(g_bg, src, dst, (Vector2){0, 0}, 0, Fade(WHITE, g_bgAlpha));
        }

        // draw level primitives + player box via render module
        render_draw_level(staticMap, &gPlayer, gCrocs,  MAX_CROCS, gFruits, MAX_FRUITS);


    EndTextureMode();

    BeginDrawing();
        //ClearBackground(BLACK);
        // note: negative height to flip the render texture
        Rectangle src = (Rectangle){0, 0, (float)rt.texture.width, -(float)rt.texture.height};
        Rectangle dst = (Rectangle){0, 0, (float)VW * SCALE, (float)VH * SCALE};
        DrawTexturePro(rt.texture, src, dst, (Vector2){0, 0}, 0, WHITE);   


        // show win debug centered at top
        if (gRoundWon) {
            const char* winMsg = "PLAYER WON";
            int fontSize = 20;
            int textWidth = MeasureText(winMsg, fontSize);
            int screenWidth = VW * SCALE;
            int posX = (screenWidth - textWidth) / 2;
            int posY = 4;
            DrawText(winMsg, posX, posY, fontSize, YELLOW);
        }

        if (gGameOver) {
            const char* txt = "GAME OVER";
            int fs = 24;
            int tw = MeasureText(txt, fs);
            int screenWidth = VW * SCALE;
            int screenHeight = VH * SCALE;
            DrawText(txt, (screenWidth - tw)/2, screenHeight/3, fs, RED);
            
            // Draw restart button
            const char* btnText = "RESTART";
            int btnFontSize = 20;
            int btnWidth = 120;
            int btnHeight = 40;
            int btnX = (screenWidth - btnWidth) / 2;
            int btnY = screenHeight/2;
            
            Rectangle restartBtn = (Rectangle){btnX, btnY, btnWidth, btnHeight};
            
            // Check if mouse is over button
            Vector2 mousePos = GetMousePosition();
            bool isHover = CheckCollisionPointRec(mousePos, restartBtn);
            
            // Draw button
            Color btnColor = isHover ? (Color){100, 180, 100, 255} : (Color){80, 140, 80, 255};
            DrawRectangleRec(restartBtn, btnColor);
            DrawRectangleLinesEx(restartBtn, 2, WHITE);
            
            // Center text in button
            int btnTextWidth = MeasureText(btnText, btnFontSize);
            int textX = btnX + (btnWidth - btnTextWidth) / 2;
            int textY = btnY + (btnHeight - btnFontSize) / 2;
            DrawText(btnText, textX, textY, btnFontSize, WHITE);
        }


        {
            char hud[64];
            snprintf(hud, sizeof(hud), "LIVES: %u   SCORE: %u", (unsigned)gUiLives, (unsigned)gUiScore);
            DrawText(hud, 8, 8, 12, WHITE);
        }

       
    EndDrawing();
}

// --- game logic hook ---
void game_update_and_get_proposal(const CP_Static* staticMap, ProposedState* out) {
    // read keyboard/game input

    if (gGameOver) {
        out->x = gPlayer.x; out->y = gPlayer.y;
        out->vx = 0;        out->vy = 0;
        out->flags = 0;     // sin grounded/just_died
        (void)staticMap;
        return;
    }


    InputState in = input_read();

    

    // build world view (map data + bounds)
    MapView mv = map_view_build();

    //update win state based on player position
    game_check_win_condition();


    if (crocodile_player_overlap(&gPlayer, gCrocs, MAX_CROCS)) {
        player_mark_dead(&gPlayer);  // unificado
    }

    


    // run physics only if player is alive
    if (!player_is_dead(&gPlayer)) {
        physics_step(&gPlayer, &in, &mv, GetFrameTime());
        // update crocodiles
        for (int i = 0; i < MAX_CROCS; ++i) {
            crocodile_update(&gCrocs[i], &mv);
        }
    
    if (player_pick_fruits(&gPlayer, gFruits, MAX_FRUITS))
    {
        gFruitsPickedCount++;
    }
        

    } else {
        // dead: freeze velocity so we do not drift
        gPlayer.vx = 0;
        gPlayer.vy = 0;
        
    }


    // fill proposal to send to server
    out->x = gPlayer.x;
    out->y = gPlayer.y;
    out->vx = gPlayer.vx;
    out->vy = gPlayer.vy;

    // flags:
    // bit 0 -> grounded (on floor or platform)
    // bit 1 -> just died this frame (one–shot event)
    uint8_t flags = 0;
    if (gPlayer.grounded) flags |= 0x01;
    if (gPlayer.justDied) flags |= 0x02;  // lee el campo, NO llames player_just_died aquí
    out->flags = flags;

}

// returns the total size written to dst, or 0 if dst is NULL / too small.
size_t game_build_entities_tlv(uint8_t* dst, size_t dstCapacity) {
    if (!dst || dstCapacity == 0) return 0;

    // temporary buffer to collect all entities this frame
    EntitySnapshot snapshots[1 + MAX_CROCS + MAX_FRUITS];
    uint8_t count = 0;

    // --- player snapshot ---
    {
        uint8_t spriteId = 0;  // placeholder, later replaced by real animation logic
        snapshots[count].kind     = ENTITY_KIND_PLAYER;
        snapshots[count].spriteId = spriteId;
        snapshots[count].x        = gPlayer.x;
        snapshots[count].y        = gPlayer.y;
        count++;
    }

    // --- crocodile snapshots ---
    for (int i = 0; i < MAX_CROCS; ++i) {
        if (!gCrocs[i].active) continue;
        if (count >= sizeof(snapshots) / sizeof(snapshots[0])) break;

        uint8_t spriteId = 0;
        // simple variant → sprite mapping
        if      (gCrocs[i].variant == CROC_VARIANT_BLUE) spriteId = 1;
        else if (gCrocs[i].variant == CROC_VARIANT_RED)  spriteId = 2;

        snapshots[count].kind     = ENTITY_KIND_CROC;
        snapshots[count].spriteId = spriteId;
        snapshots[count].x        = gCrocs[i].x;
        snapshots[count].y        = gCrocs[i].y;
        count++;
    }

    // --- fruit snapshots ---
    for (int i = 0; i < MAX_FRUITS; ++i) {
        if (!gFruits[i].active) continue;
        if (count >= sizeof(snapshots) / sizeof(snapshots[0])) break;

        uint8_t spriteId = 0;
        if      (gFruits[i].variant == FRUIT_VARIANT_APPLE)  spriteId = 1;
        else if (gFruits[i].variant == FRUIT_VARIANT_ORANGE) spriteId = 2;
        else if (gFruits[i].variant == FRUIT_VARIANT_BANANA) spriteId = 3;

        snapshots[count].kind     = ENTITY_KIND_FRUIT;
        snapshots[count].spriteId = spriteId;
        snapshots[count].x        = gFruits[i].x;
        snapshots[count].y        = gFruits[i].y;
        count++;
    }

    // serialize all collected snapshots into a TLV buffer
    return entities_tlv_build(dst, dstCapacity, snapshots, count);
}



void game_apply_correction(uint32_t tick, uint8_t grounded, int16_t platId, int16_t yCorr, int16_t vyCorr) {
    (void)tick; (void)platId;

    // snap or nudge according to server correction
    if (grounded) {
        gPlayer.y = yCorr;
        gPlayer.vy = 0;
        gPlayer.grounded = true;
    } else {
        gPlayer.y += vyCorr;
        gPlayer.grounded = false;
    }
}

void game_apply_remote_state(int16_t x, int16_t y, int16_t vx, int16_t vy, uint8_t flags) {
    
    gPlayer.x  = x;
    gPlayer.y  = y;
    gPlayer.vx = vx;
    gPlayer.vy = vy;

    
    gPlayer.grounded = (flags & 0x01) != 0;

    if (flags & 0x02) {
        
        player_mark_dead(&gPlayer);

    }
}

// Update dynamic entities for the spectator (no input, no local player physics)
void game_update_spectator(const CP_Static* staticMap) {
    (void)staticMap;

    // If game is over, freeze everything (no croc updates)
    if (gGameOver) {
        return;
    }

    // build world view
    MapView mv = map_view_build();

    // update crocodiles using the same logic as the player client
    for (int i = 0; i < MAX_CROCS; ++i) {
        crocodile_update(&gCrocs[i], &mv);
    }
}

void game_spawn_croc(uint8_t variant, int16_t x, int16_t y) {
    // search for an inactive crocodile to spawn
    for (int i = 0; i < MAX_CROCS; ++i) {
        if (!gCrocs[i].active) {
            crocodile_spawn(&gCrocs[i], variant, x, y);
            return;
        }
    }
    // if everything is active, overwrite the first one
    crocodile_spawn(&gCrocs[0], variant, x, y);
}

void game_spawn_fruit(uint8_t variant, int16_t x, int16_t y) {
    // search for an inactive fruit to spawn
    for (int i = 0; i < MAX_FRUITS; ++i) {
        if (!gFruits[i].active) {
            fruit_spawn(&gFruits[i], variant, x, y);
            return;
        }
    }
    // if everything is active, overwrite the first one
    fruit_spawn(&gFruits[0], variant, x, y);
}


void game_remove_fruit_at(int16_t x, int16_t y){
    for (int i = 0; i < MAX_FRUITS; ++i) {
        if (!gFruits[i].active) continue;
        int16_t fx = gFruits[i].x, fy = gFruits[i].y, fw = gFruits[i].w, fh = gFruits[i].h;
        if (x >= fx && x < fx + fw && y >= fy && y < fy + fh) {
            gFruits[i].active = false;
            break;
        }
    }
}

bool game_consume_death_event(void) {
    return player_just_died(&gPlayer); 
}



bool game_consume_win_event(void) {
    if (!gRoundJustWon) return false;
    gRoundJustWon = false;
    //crocodile_increase_speed();
    return true;
}

bool game_consume_fruit_event(int16_t* outX, int16_t* outY) {
    if (!gPlayer.justPickedFruit) {
        return false;
    }

    // Store the coordinates of the last picked fruit
    if (outX) *outX = gPlayer.lastPickedFruitX;
    if (outY) *outY = gPlayer.lastPickedFruitY;

    gPlayer.justPickedFruit = false;
    return true;
}

void game_over_event(void) {
    gGameOver = true;
}

// reset all dynamic entities for a fresh round
static void game_reset_entities(void) {
    for (int i = 0; i < MAX_CROCS; ++i) {
        gCrocs[i].active = false;
        gCrocs[i].vx = 0;
        gCrocs[i].vy = 0;
    }
    for (int i = 0; i < MAX_FRUITS; ++i) {
        gFruits[i].active = false;
    }
}

void game_set_ui_lives(uint8_t lives)  { gUiLives = lives; }

void game_set_ui_score(uint32_t score) { gUiScore = score; }


// respawn after a death: reset player and entities, keep croc speed
void game_respawn_death(void) {
    game_reset_entities();

    gRoundWon = false;
    gRoundJustWon = false;
    gPlayer.isDead = false;

    // reset player state and position
    player_init(&gPlayer, 30, 195, 16, 16);
}

// respawn after a win: reset player and entities, increase croc speed
void game_respawn_win(void) {
    game_reset_entities();

    // increase crocodile speed for next round
    //crocodile_increase_speed();

    gRoundWon = false;
    gRoundJustWon = false;

    // reset player state and position
    player_init(&gPlayer, 30, 195, 16, 16);
}

// full game restart: reset everything to initial state (3 lives, score 0, default croc speed)
void game_restart(void) {
    game_reset_entities();

    // Reset crocodile speed to default
    crocodile_reset_speed();

    // Reset game state flags
    gRoundWon = false;
    gRoundJustWon = false;
    gGameOver = false;
    gFruitsPickedCount = 0;

    // Reset UI (will be updated by server messages)
    gUiLives = 3;
    gUiScore = 0;

    // Reset player state and position
    player_init(&gPlayer, 30, 195, 16, 16);
}

// check if restart button was clicked (only valid when game over)
bool game_check_restart_clicked(void) {
    if (!gGameOver) return false;
    
    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return false;
    
    int screenWidth = VW * SCALE;
    int screenHeight = VH * SCALE;
    int btnWidth = 120;
    int btnHeight = 40;
    int btnX = (screenWidth - btnWidth) / 2;
    int btnY = screenHeight / 2;
    
    Rectangle restartBtn = (Rectangle){btnX, btnY, btnWidth, btnHeight};
    Vector2 mousePos = GetMousePosition();
    
    return CheckCollisionPointRec(mousePos, restartBtn);
}
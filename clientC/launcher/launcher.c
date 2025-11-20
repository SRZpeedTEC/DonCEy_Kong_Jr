#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "raylib.h"

#include "../clientPlayer/clientPlayer.h"
#include "../clientSpectator/clientSpectator.h"

int main(void) {
    const int screenWidth  = 600;
    const int screenHeight = 300;

    InitWindow(screenWidth, screenHeight, "DonCEy Kong Jr - Launcher");

    // Default connection params
    char ip[32]      = "127.0.0.1";
    char portStr[8]  = "9090";

    int selectedRole = 0; // 0 = none, 1 = player, 2 = spectator

    // Simple text-based “fields” (you can improve later)
    Rectangle btnPlayer    = { 80,  150, 180, 50 };
    Rectangle btnSpectator = { 340, 150, 180, 50 };

    SetTargetFPS(60);

    while (!WindowShouldClose()) {

        // Mouse input
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 m = GetMousePosition();
            if (CheckCollisionPointRec(m, btnPlayer)) {
                selectedRole = 1;
                break;
            }
            if (CheckCollisionPointRec(m, btnSpectator)) {
                selectedRole = 2;
                break;
            }
        }

        BeginDrawing();
        ClearBackground((Color){30, 30, 30, 255});

        DrawText("DonCEy Kong Jr - Launcher", 80, 20, 24, RAYWHITE);
        DrawText("Server IP:",   80, 70, 20, RAYWHITE);
        DrawText(ip,            190, 70, 20, RAYWHITE);

        DrawText("Port:",       80, 100, 20, RAYWHITE);
        DrawText(portStr,       190, 100, 20, RAYWHITE);

        // For now, IP/Port are fixed – later you can add text input if you want

        DrawRectangleRec(btnPlayer, DARKGREEN);
        DrawText("Join as PLAYER", btnPlayer.x + 10, btnPlayer.y + 15, 18, RAYWHITE);

        DrawRectangleRec(btnSpectator, DARKBLUE);
        DrawText("Join as SPECTATOR", btnSpectator.x + 10, btnSpectator.y + 15, 18, RAYWHITE);

        EndDrawing();
    }

    CloseWindow();

    if (selectedRole == 0) {
        // User closed the window without choosing
        return 0;
    }

    uint16_t port = (uint16_t)atoi(portStr);

    if (selectedRole == 1) {
        return run_player_client(ip, port);
    } else {
        return run_spectator_client(ip, port);
    }
}

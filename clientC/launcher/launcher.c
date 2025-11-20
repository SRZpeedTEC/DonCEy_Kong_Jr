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

    char ip[32]     = "127.0.0.1";
    char portStr[8] = "9090";

    int selectedRole = 0;   // 0 = none, 1 = player, 2 = spectator
    int launcherStep = 0;   // 0 = elegir rol, 1 = elegir slot de spectator
    int desiredSlot  = 1;   // 1 o 2 (por defecto Player 1)

    Rectangle btnPlayer      = (Rectangle){  80, 150, 180, 50 };
    Rectangle btnSpectator   = (Rectangle){ 340, 150, 180, 50 };
    Rectangle btnSlot1       = (Rectangle){  80, 150, 180, 50 };
    Rectangle btnSlot2       = (Rectangle){ 340, 150, 180, 50 };

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 m = GetMousePosition();

            if (launcherStep == 0) {
                // Pantalla 1: elegir rol
                if (CheckCollisionPointRec(m, btnPlayer)) {
                    selectedRole = 1;
                    break; // cerrar launcher y lanzar cliente player
                }
                if (CheckCollisionPointRec(m, btnSpectator)) {
                    selectedRole = 2;
                    launcherStep = 1; // pasar a elegir qué player ver
                }
            } else if (launcherStep == 1 && selectedRole == 2) {
                // Pantalla 2: elegir qué player espectear
                if (CheckCollisionPointRec(m, btnSlot1)) {
                    desiredSlot = 1;
                    break;
                }
                if (CheckCollisionPointRec(m, btnSlot2)) {
                    desiredSlot = 2;
                    break;
                }
            }
        }

        BeginDrawing();
        ClearBackground((Color){30, 30, 30, 255});

        DrawText("DonCEy Kong Jr - Launcher", 80, 20, 24, RAYWHITE);
        DrawText("Server IP:",   80, 70, 20, RAYWHITE);
        DrawText(ip,            190, 70, 20, RAYWHITE);
        DrawText("Port:",       80, 100, 20, RAYWHITE);
        DrawText(portStr,       190, 100, 20, RAYWHITE);

        if (launcherStep == 0) {
            // Pantalla elegir rol
            DrawRectangleRec(btnPlayer, DARKGREEN);
            DrawText("Join as PLAYER", btnPlayer.x + 10, btnPlayer.y + 15, 18, RAYWHITE);

            DrawRectangleRec(btnSpectator, DARKBLUE);
            DrawText("Join as SPECTATOR", btnSpectator.x + 10, btnSpectator.y + 15, 18, RAYWHITE);
        } else if (launcherStep == 1 && selectedRole == 2) {
            // Pantalla elegir player a espectear
            DrawText("Select player to spectate:", 80, 130, 20, RAYWHITE);

            DrawRectangleRec(btnSlot1, DARKPURPLE);
            DrawText("Spectate PLAYER 1", btnSlot1.x + 10, btnSlot1.y + 15, 18, RAYWHITE);

            DrawRectangleRec(btnSlot2, DARKPURPLE);
            DrawText("Spectate PLAYER 2", btnSlot2.x + 10, btnSlot2.y + 15, 18, RAYWHITE);
        }

        EndDrawing();
    }

    CloseWindow();

    if (selectedRole == 0) {
        // Usuario cerró el launcher sin elegir nada
        return 0;
    }

    uint16_t port = (uint16_t)atoi(portStr);

    if (selectedRole == 1) {
        return run_player_client(ip, port);
    } else {
        // spectator + slot elegido (1 o 2)
        return run_spectator_client(ip, port, (uint8_t)desiredSlot);
    }
}


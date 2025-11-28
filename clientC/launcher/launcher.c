#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "raylib.h"

#include "../clientPlayer/clientPlayer.h"
#include "../clientSpectator/clientSpectator.h"
#include "../UtilsC/msg_types.h"
#include "../UtilsC/proto.h"
#include "../clientPlayer/net.h"

static int check_server_capacity(const char* ip, uint16_t port, uint8_t requestedRole)
{
    if (!net_init()) {
        fprintf(stderr, "net_init failed in check_server_capacity\n");
        return -1;
    }

    int socketFd = net_connect(ip, port);
    if (socketFd < 0) {
        fprintf(stderr, "net_connect failed in check_server_capacity\n");
        net_cleanup();
        return -1;
    }

    // Enviar rol solicitado: 1 = PLAYER, 2 = SPECTATOR
    if (net_write_n(socketFd, &requestedRole, 1) <= 0) {
        fprintf(stderr, "Failed to send requestedRole\n");
        net_close(socketFd);
        net_cleanup();
        return -1;
    }

    // Leer CLIENT_ACK
    CP_Header header;
    if (!cp_read_header(socketFd, &header) || header.type != CP_TYPE_CLIENT_ACK) {
        fprintf(stderr, "Bad or missing CLIENT_ACK in check_server_capacity\n");
        net_close(socketFd);
        net_cleanup();
        return 0; // lo tratamos como rechazado
    }

    uint8_t roleByte = 0;

    if (header.payloadLen > 0) {
        uint8_t buf[8];
        if (header.payloadLen > sizeof(buf)) {
            fprintf(stderr, "CLIENT_ACK payload too large\n");
            net_close(socketFd);
            net_cleanup();
            return -1;
        }

        if (net_read_n(socketFd, buf, header.payloadLen) <= 0) {
            fprintf(stderr, "Failed to read CLIENT_ACK payload\n");
            net_close(socketFd);
            net_cleanup();
            return -1;
        }

        roleByte = buf[0];   // primer byte = rol
    } else {
        fprintf(stderr, "CLIENT_ACK without payload\n");
        net_close(socketFd);
        net_cleanup();
        return 0;
    }

    net_close(socketFd);
    net_cleanup();

    if (roleByte == 0) {
        // server respondiÃ³ "sin rol" => capacidad alcanzada
        return 0;
    }

    // cualquier valor distinto de 0 lo consideramos "hay espacio"
    return 1;
}

int main(void) {
    const int screenWidth  = 600;
    const int screenHeight = 300;

    InitWindow(screenWidth, screenHeight, "DonCEy Kong Jr - Launcher");

    char ip[32]     = "127.0.0.1";
    char portStr[8] = "9090";

    int selectedRole = 0;   // 0 = none, 1 = player, 2 = spectator 
    int launcherStep = 0;   // 0 = elegir rol, 1 = elegir slot de spectator
    int desiredSlot  = 1;   // 1 o 2 (por defecto Player 1)

    int showErrorMessage = 0;
    char errorText[128] = {0};
    
    bool shouldLaunchClient = false;  // flag to control when to actually launch

    Rectangle btnPlayer      = (Rectangle){  80, 150, 180, 50 };
    Rectangle btnSpectator   = (Rectangle){ 340, 150, 190, 50 };
    Rectangle btnSlot1       = (Rectangle){  80, 150, 180, 50 };
    Rectangle btnSlot2       = (Rectangle){ 340, 150, 180, 50 };

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 m = GetMousePosition();
            
            // If error is showing, any click dismisses it
            if (showErrorMessage) {
                showErrorMessage = 0;
                continue;  // Skip other button checks this frame
            }

            if (launcherStep == 0) {
                // Pantalla 1: elegir rol
                if (CheckCollisionPointRec(m, btnPlayer)) {
                    uint16_t port = (uint16_t)atoi(portStr);
                    
                    int cap = check_server_capacity(ip, port, 1); // 1 = PLAYER
                    if (cap == 1) {
                        // Server accepts - launch client
                        selectedRole = 1;
                        showErrorMessage = 0;
                        shouldLaunchClient = true;
                    } else if (cap == 0) {
                        // Server rejected - show error
                        showErrorMessage = 1;
                        strcpy(errorText, "Maximo de jugadores alcanzado.\nNo se puede crear otro PLAYER.");
                    } else {
                        // Connection error
                        showErrorMessage = 1;
                        strcpy(errorText, "No se pudo conectar al servidor.");
                    }
                }

                if (CheckCollisionPointRec(m, btnSpectator)) {
                    uint16_t port = (uint16_t)atoi(portStr);
                    
                    int cap = check_server_capacity(ip, port, 2); // 2 = SPECTATOR
                    if (cap == 1) {
                        // Server accepts - proceed to slot selection
                        selectedRole = 2;
                        launcherStep = 1;
                        showErrorMessage = 0;
                    } else if (cap == 0) {
                        // Server rejected - show error
                        showErrorMessage = 1;
                        strcpy(errorText, "Maximo de espectadores alcanzado.\nTodos los espacios estan llenos.");
                    } else {
                        // Connection error
                        showErrorMessage = 1;
                        strcpy(errorText, "No se pudo conectar al servidor.");
                    }
                }
            } else if (launcherStep == 1 && selectedRole == 2) {
                // Pantalla 2: elegir quÃ© player espectear
                if (CheckCollisionPointRec(m, btnSlot1)) {
                    desiredSlot = 1;
                    shouldLaunchClient = true;
                }
                if (CheckCollisionPointRec(m, btnSlot2)) {
                    desiredSlot = 2;
                    shouldLaunchClient = true;
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

        if (showErrorMessage) {
            int boxW = 560;
            int boxH = 80;
            int boxX = (screenWidth  - boxW) / 2;
            int boxY = screenHeight - boxH - 10;

            DrawRectangle(boxX, boxY, boxW, boxH, (Color){80, 0, 0, 255});
            DrawRectangleLines(boxX, boxY, boxW, boxH, RED);
            DrawText(errorText, boxX + 10, boxY + 10, 16, RAYWHITE);
            DrawText("Intenta creando un nuevo jugador", boxX + 10, boxY + 55, 14, GRAY);
        }
        EndDrawing();
        
        // Break the loop only when we should actually launch the client
        if (shouldLaunchClient) {
            break;
        }
    }

    CloseWindow();

    // Only launch client if explicitly flagged to do so
    if (!shouldLaunchClient || selectedRole == 0) {
        // Usuario cerrÃ³ el launcher sin elegir nada o hubo un error
        return 0;
    }

    uint16_t port = (uint16_t)atoi(portStr);

    if (selectedRole == 1) {
        return run_player_client(ip, port);
    } else {
        // spectator: send role (2) + slot (1 or 2) in the initial connection
        // This requires modifying the protocol to send 2 bytes instead of 1
        // For now, just launch the spectator client
        return run_spectator_client(ip, port, (uint8_t)desiredSlot);
    }
}
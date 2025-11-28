#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "raylib.h"

#include "../clientPlayer/clientPlayer.h"
#include "../clientSpectator/clientSpectator.h"
#include "../UtilsC/msg_types.h"
#include "../UtilsC/proto.h"
#include "../clientPlayer/net.h"

static int check_server_capacity(const char* ip, uint16_t port, uint8_t requestedRole,
                                 int* outPlayer1SpecCount, int* outPlayer2SpecCount,
                                 bool* outPlayer1Active, bool* outPlayer2Active)
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

    // Send requested role: 1 = PLAYER, 2 = SPECTATOR
    if (net_write_n(socketFd, &requestedRole, 1) <= 0) {
        fprintf(stderr, "Failed to send requestedRole\n");
        net_close(socketFd);
        net_cleanup();
        return -1;
    }

    // Read CLIENT_ACK
    CP_Header header;
    if (!cp_read_header(socketFd, &header) || header.type != CP_TYPE_CLIENT_ACK) {
        fprintf(stderr, "Bad or missing CLIENT_ACK in check_server_capacity\n");
        net_close(socketFd);
        net_cleanup();
        return 0;
    }

    uint8_t roleByte = 0;
    uint8_t player1Count = 255;
    uint8_t player2Count = 255;

    if (header.payloadLen >= 3) {
        // Extended format: [roleByte, player1Count, player2Count]
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

        roleByte = buf[0];
        player1Count = buf[1];
        player2Count = buf[2];
    } else if (header.payloadLen == 1) {
        // Legacy format: only role byte
        uint8_t buf[1];
        if (net_read_n(socketFd, buf, 1) <= 0) {
            fprintf(stderr, "Failed to read CLIENT_ACK payload\n");
            net_close(socketFd);
            net_cleanup();
            return -1;
        }
        roleByte = buf[0];
    } else {
        fprintf(stderr, "CLIENT_ACK without payload\n");
        net_close(socketFd);
        net_cleanup();
        return 0;
    }

    net_close(socketFd);
    net_cleanup();

    // Return slot info (255 means inactive)
    if (outPlayer1SpecCount) *outPlayer1SpecCount = (player1Count != 255) ? player1Count : -1;
    if (outPlayer2SpecCount) *outPlayer2SpecCount = (player2Count != 255) ? player2Count : -1;
    if (outPlayer1Active) *outPlayer1Active = (player1Count != 255);
    if (outPlayer2Active) *outPlayer2Active = (player2Count != 255);

    if (roleByte == 0) {
        return 0; // rejected
    }

    return 1; // accepted
}

int main(void) {
    const int screenWidth  = 600;
    const int screenHeight = 300;

    InitWindow(screenWidth, screenHeight, "DonCEy Kong Jr - Launcher");

    char ip[32]     = "127.0.0.1";
    char portStr[8] = "9090";

    int selectedRole = 0;
    int launcherStep = 0;
    int desiredSlot  = 1;

    int showErrorMessage = 0;
    char errorText[128] = {0};
    
    bool shouldLaunchClient = false;

    // Slot availability info
    int player1SpecCount = -1;
    int player2SpecCount = -1;
    bool player1Active = false;
    bool player2Active = false;

    Rectangle btnPlayer      = (Rectangle){  80, 150, 220, 50 };
    Rectangle btnSpectator   = (Rectangle){ 340, 150, 220, 50 };
    Rectangle btnSlot1       = (Rectangle){  80, 150, 220, 50 };
    Rectangle btnSlot2       = (Rectangle){ 340, 150, 220, 50 };

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 m = GetMousePosition();
            
            if (showErrorMessage) {
                showErrorMessage = 0;
                continue;
            }

            if (launcherStep == 0) {
                // Screen 1: choose role
                if (CheckCollisionPointRec(m, btnPlayer)) {
                    uint16_t port = (uint16_t)atoi(portStr);
                    
                    int cap = check_server_capacity(ip, port, 1, NULL, NULL, NULL, NULL);
                    if (cap == 1) {
                        selectedRole = 1;
                        showErrorMessage = 0;
                        shouldLaunchClient = true;
                    } else if (cap == 0) {
                        showErrorMessage = 1;
                        strcpy(errorText, "Máximo de jugadores alcanzados.\nNo hay más espacios para JUGADORES.");
                    } else {
                        showErrorMessage = 1;
                        strcpy(errorText, "No se pudo conectar al servidor.");
                    }
                }

                if (CheckCollisionPointRec(m, btnSpectator)) {
                    uint16_t port = (uint16_t)atoi(portStr);
                    
                    // Query slot availability
                    int cap = check_server_capacity(ip, port, 2, 
                                                   &player1SpecCount, &player2SpecCount,
                                                   &player1Active, &player2Active);
                    if (cap == 1) {
                        // Check if any slots available
                        bool hasAvailableSlot = false;
                        if (player1Active && player1SpecCount < 2) hasAvailableSlot = true;
                        if (player2Active && player2SpecCount < 2) hasAvailableSlot = true;
                        
                        if (!hasAvailableSlot && !player1Active && !player2Active) {
                            // No players online
                            showErrorMessage = 1;
                            strcpy(errorText, "No hay jugadores en línea.\nEspera a que un jugador se conecte.");
                        } else if (!hasAvailableSlot) {
                            // All slots full
                            showErrorMessage = 1;
                            strcpy(errorText, "Todos los espacios para espectadores están llenos.\nIntenta más tarde.");
                        } else {
                            // At least one slot available - proceed
                            selectedRole = 2;
                            launcherStep = 1;
                            showErrorMessage = 0;
                        }
                    } else if (cap == 0) {
                        showErrorMessage = 1;
                        strcpy(errorText, "El servidor rechazó al espectador.\nCapacidad alcanzada.");
                    } else {
                        showErrorMessage = 1;
                        strcpy(errorText, "No se pudo conectar al servidor.");
                    }
                }
            } else if (launcherStep == 1 && selectedRole == 2) {
                // Screen 2: choose player slot to spectate
                
                // Check if Player 1 slot is available and clicked
                if (player1Active && player1SpecCount < 2 && CheckCollisionPointRec(m, btnSlot1)) {
                    desiredSlot = 1;
                    shouldLaunchClient = true;
                }
                
                // Check if Player 2 slot is available and clicked
                if (player2Active && player2SpecCount < 2 && CheckCollisionPointRec(m, btnSlot2)) {
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
            // Screen: choose role
            DrawRectangleRec(btnPlayer, DARKGREEN);
            DrawText("Entrar como jugador", btnPlayer.x + 10, btnPlayer.y + 15, 18, RAYWHITE);

            DrawRectangleRec(btnSpectator, DARKBLUE);
            DrawText("Entrar como espectador", btnSpectator.x + 10, btnSpectator.y + 15, 18, RAYWHITE);
        } else if (launcherStep == 1 && selectedRole == 2) {
            // Screen: choose player to spectate
            DrawText("Selecciona jugador para espectar:", 80, 120, 20, RAYWHITE);

            // Player 1 slot button
            bool slot1Available = player1Active && player1SpecCount < 2;
            bool slot1Full = player1Active && player1SpecCount >= 2;
            
            Color slot1Color = DARKGRAY;
            if (slot1Available) slot1Color = DARKPURPLE;
            else if (slot1Full) slot1Color = (Color){80, 40, 40, 255}; // Dark red
            
            DrawRectangleRec(btnSlot1, slot1Color);
            
            if (!player1Active) {
                DrawText("JUGADOR 1: DESCONECTADO", btnSlot1.x + 10, btnSlot1.y + 10, 14, GRAY);
            } else if (slot1Full) {
                char buf[64];
                snprintf(buf, sizeof(buf), "JUGADOR 1 (LLENO %d/2)", player1SpecCount);
                DrawText(buf, btnSlot1.x + 10, btnSlot1.y + 15, 14, GRAY);
            } else {
                char buf[64];
                snprintf(buf, sizeof(buf), "Espectar JUGADOR 1 (%d/2)", player1SpecCount);
                DrawText(buf, btnSlot1.x + 10, btnSlot1.y + 15, 14, RAYWHITE);
            }

            // Player 2 slot button
            bool slot2Available = player2Active && player2SpecCount < 2;
            bool slot2Full = player2Active && player2SpecCount >= 2;
            
            Color slot2Color = DARKGRAY;
            if (slot2Available) slot2Color = DARKPURPLE;
            else if (slot2Full) slot2Color = (Color){80, 40, 40, 255}; // Dark red
            
            DrawRectangleRec(btnSlot2, slot2Color);
            
            if (!player2Active) {
                DrawText("JUGADOR 2: DESCONECTADO", btnSlot2.x + 10, btnSlot2.y + 10, 14, GRAY);
            } else if (slot2Full) {
                char buf[64];
                snprintf(buf, sizeof(buf), "JUGADOR 2 (LLENO %d/2)", player2SpecCount);
                DrawText(buf, btnSlot2.x + 10, btnSlot2.y + 15, 14, GRAY);
            } else {
                char buf[64];
                snprintf(buf, sizeof(buf), "Espectar JUGADOR 2 (%d/2)", player2SpecCount);
                DrawText(buf, btnSlot2.x + 10, btnSlot2.y + 15, 14, RAYWHITE);
            }
        }

        if (showErrorMessage) {
            int boxW = 560;
            int boxH = 80;
            int boxX = (screenWidth  - boxW) / 2;
            int boxY = screenHeight - boxH - 10;

            DrawRectangle(boxX, boxY, boxW, boxH, (Color){80, 0, 0, 255});
            DrawRectangleLines(boxX, boxY, boxW, boxH, RED);
            DrawText(errorText, boxX + 10, boxY + 10, 16, RAYWHITE);
            DrawText("Haz clic en cualquier lugar para cerrar", boxX + 10, boxY + 55, 14, GRAY);
        }
        EndDrawing();
        
        if (shouldLaunchClient) {
            break;
        }
    }

    CloseWindow();

    if (!shouldLaunchClient || selectedRole == 0) {
        return 0;
    }

    uint16_t port = (uint16_t)atoi(portStr);

    if (selectedRole == 1) {
        return run_player_client(ip, port);
    } else {
        return run_spectator_client(ip, port, (uint8_t)desiredSlot);
    }
}
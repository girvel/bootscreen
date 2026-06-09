#include <raylib.h>
#include "stdio.h"

#ifndef SCREEN_W
#define SCREEN_W 1366
#endif

#ifndef SCREEN_H
#define SCREEN_H 768
#endif

int main(void)
{
    InitWindow(SCREEN_W, SCREEN_H, "Boot screen");
    SetTargetFPS(30);
    HideCursor();
    SetExitKey(KEY_NULL);

    const char *text = "Entering the Void...";
    int font_size = 30;
    int text_w = MeasureText(text, font_size);

    int circle_r = text_w / 2;

    while (!WindowShouldClose()) {
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawCircle(SCREEN_W / 2, SCREEN_H / 2, circle_r, BLACK);
            DrawText(text,
                     (SCREEN_W - text_w) / 2,
                     (SCREEN_H - font_size) / 2,
                     font_size, RAYWHITE);
            circle_r += GetTime() * 5;
        EndDrawing();
    }
    CloseWindow();
    return 0;
}

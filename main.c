#include <raylib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#ifndef SCREEN_W
#define SCREEN_W 1366
#endif

#ifndef SCREEN_H
#define SCREEN_H 768
#endif

static const struct timespec delay = {.tv_nsec = 100000000L};
static const char *card = "/dev/dri/card0";

// TODO any keypress => new circle of the opposite color
// TODO raylib logs should be prefixed with Bootscreen:

int main(void)
{
    printf("Bootscreen: Started\n");
    int max_attempts = 100;
    while (access(card, F_OK) != 0) {
        nanosleep(&delay, NULL);
        max_attempts--;
        if (max_attempts <= 0) {
            printf("Bootscreen: Timeout, %s does not exist\n");
            return 1;
        }
    }
    printf("Bootscreen: %s ready\n", card);

    SetTraceLogLevel(LOG_WARNING);
    InitWindow(SCREEN_W, SCREEN_H, "Boot screen");
    SetTargetFPS(30);
    HideCursor();
    SetExitKey(KEY_NULL);

    const char *text = "Entering the Void...";
    int font_size = 30;
    int text_w = MeasureText(text, font_size);

    int circle_r = text_w / 2;

    printf("Bootscreen: Enter main loop\n");
    while (1) {
        if (WindowShouldClose()) {
            printf("Bootscreen: Closed manually\n");
            break;
        }

        if (access("/run/stop-bootscreen", F_OK) == 0) {
            printf("Bootscreen: Closed via /run/stop-bootscreen\n");
            break;
        }

        BeginDrawing();
            if (circle_r * circle_r * 4 < SCREEN_W * SCREEN_W + SCREEN_H * SCREEN_H) {
                ClearBackground(RAYWHITE);
                DrawCircle(SCREEN_W / 2, SCREEN_H / 2, circle_r, BLACK);
            } else {
                ClearBackground(BLACK);
            }
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

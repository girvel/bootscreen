#include <raylib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>

#ifndef SCREEN_W
#define SCREEN_W 1366
#endif

#ifndef SCREEN_H
#define SCREEN_H 768
#endif

static const struct timespec tenth = {.tv_nsec = 100000000L};
static const int max_attempts_n = 300;
static const char *card = "/dev/dri/card0";

// TODO any keypress => new circle of the opposite color
// TODO raylib logs should be prefixed with Bootscreen:

int main(void)
{
    printf("Bootscreen: Started\n");

    int attempt_n = 0;
    while (access(card, F_OK) != 0) {
        nanosleep(&tenth, NULL);
        attempt_n++;
        if (attempt_n >= max_attempts_n) {
            printf("Bootscreen: Timeout, %s does not exist\n", card);
            return 1;
        }
    }
    printf("Bootscreen: %s ready on attempt %d\n", card, attempt_n);

    DIR *drm_dir = opendir("/sys/class/drm");
    if (drm_dir == NULL) {
        printf("Bootscreen: Unable to open /sys/class/drm\n");
        perror("");
        return 1;
    }

    for (attempt_n = 0; attempt_n < max_attempts_n; attempt_n++) {
        struct dirent *de;
        while ((de = readdir(drm_dir))) {
            char path_buf[300];
            snprintf(path_buf, sizeof(path_buf), "/sys/class/drm/%s/status", de->d_name);
            int fd = open(path_buf, O_RDONLY);
            if (fd == -1) continue;

            char content_buf[16];
            int read_n = read(fd, content_buf, sizeof(content_buf) - 1);
            if (content_buf[read_n - 1] == '\n') {
                content_buf[read_n - 1] = '\0';
            } else {
                content_buf[read_n] = '\0';
            }
            close(fd);

            if (strcmp(content_buf, "connected") == 0) {
                printf("Bootscreen: %s connected on attempt %d\n", path_buf, attempt_n);
                goto connected;
            }
        }

        nanosleep(&tenth, NULL);
    }

    printf("Bootscreen: no drm ready after %d attempts\n", max_attempts_n);
    return 1;

connected:
    closedir(drm_dir);

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

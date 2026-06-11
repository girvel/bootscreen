#include <raylib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <math.h>

#ifndef SCREEN_W
#define SCREEN_W 1366
#endif

#ifndef SCREEN_H
#define SCREEN_H 768
#endif

#ifndef EXPECTED_TIME
#define EXPECTED_TIME 30
#endif

static const int max_attempts_n = 300;
static const float drawing_timeout = 90;
static const struct timespec tenth = {.tv_nsec = 100000000L};

volatile sig_atomic_t sigterm_received = 0;

void handle_sigterm(int sig)
{
    (void)sig;
    sigterm_received = 1;
}

bool wait_for_gpu()
{
    const char *card = "/dev/dri/card0";
    int attempt_n = 0;
    while (access(card, F_OK) != 0) {
        nanosleep(&tenth, NULL);
        attempt_n++;
        if (attempt_n >= max_attempts_n) {
            printf("Bootscreen: Timeout, %s does not exist\n", card);
            return false;
        }
    }
    printf("Bootscreen: %s ready on attempt %d\n", card, attempt_n);
    return true;
}

bool wait_for_drm()
{
    DIR *drm_dir = opendir("/sys/class/drm");
    if (drm_dir == NULL) {
        printf("Bootscreen: Unable to open /sys/class/drm\n");
        perror("");
        return false;
    }

    for (int attempt_n = 0; attempt_n < max_attempts_n; attempt_n++) {
        rewinddir(drm_dir);
        struct dirent *de;
        while ((de = readdir(drm_dir))) {
            char path_buf[300];
            snprintf(path_buf, sizeof(path_buf), "/sys/class/drm/%s/status", de->d_name);
            int fd = open(path_buf, O_RDONLY);
            if (fd == -1) continue;

            char content_buf[16];
            int read_n = read(fd, content_buf, sizeof(content_buf) - 1);
            if (read_n <= 0) {
                close(fd);
                continue;
            }

            if (content_buf[read_n - 1] == '\n') {
                content_buf[read_n - 1] = '\0';
            } else {
                content_buf[read_n] = '\0';
            }
            close(fd);

            if (strcmp(content_buf, "connected") == 0) {
                printf("Bootscreen: %s connected on attempt %d\n", path_buf, attempt_n);
                closedir(drm_dir);
                return true;
            }
        }

        nanosleep(&tenth, NULL);
    }

    printf("Bootscreen: no drm ready after %d attempts\n", max_attempts_n);
    return false;
}

// TODO any keypress => new circle of the opposite color
// TODO raylib logs should be prefixed with Bootscreen:

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("Bootscreen: Started\n");
    signal(SIGTERM, handle_sigterm);
    SetTraceLogLevel(LOG_WARNING);

    if (!wait_for_gpu()) return 1;
    if (!wait_for_drm()) return 1;

    InitWindow(SCREEN_W, SCREEN_H, "Boot screen");
    SetTargetFPS(30);
    HideCursor();

    const char *text = "Entering the Void...";
    int font_size = 30;
    int font_size_small = 16;
    int text_w = MeasureText(text, font_size);

    float start_r = text_w / 2;
    float max_r = sqrtf(SCREEN_H * SCREEN_H + SCREEN_W * SCREEN_W) / 2;
    float acceleration = 2 * (max_r - start_r) / EXPECTED_TIME / EXPECTED_TIME;

    printf("Bootscreen: Enter main loop\n");
    while (1) {
        float t = GetTime();
        if (WindowShouldClose()) {
            printf("Bootscreen: Closed manually\n");
            break;
        }

        if (sigterm_received) {
            printf("Bootscreen: Closed via SIGTERM\n");
            break;
        }

        if (t > drawing_timeout) {
            printf("Bootscreen: Timeout\n");
            break;
        }

        BeginDrawing();
            int circle_r = start_r + acceleration * t * t / 2;
            if (circle_r < max_r) {
                ClearBackground(RAYWHITE);
                DrawCircle(SCREEN_W / 2, SCREEN_H / 2, circle_r, BLACK);
            } else {
                ClearBackground(BLACK);
            }

            DrawText(text,
                     (SCREEN_W - text_w) / 2,
                     (SCREEN_H - font_size) / 2,
                     font_size, RAYWHITE);

            char buf[16];
            snprintf(buf, sizeof(buf), "%.1fs", t);
            int w = MeasureText(buf, font_size_small);
            DrawText(buf,
                     (SCREEN_W - w) / 2,
                     (SCREEN_H - font_size_small) / 2 + 40,
                     font_size_small, RAYWHITE);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}

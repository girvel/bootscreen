#include <raylib.h>
#include <rlgl.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>

#ifndef SCREEN_W
#define SCREEN_W 1366
#endif

#ifndef SCREEN_H
#define SCREEN_H 768
#endif

#ifndef EXPECTED_TIME
#define EXPECTED_TIME 30
#endif

#ifndef CIRCLES_MAX
#define CIRCLES_MAX 4
#endif

static const int max_attempts_n = 300;
static const float drawing_timeout = 90;
static const struct timespec tenth = {.tv_nsec = 100000000L};

#define EMBED_SHADER_IMPL(PATH, VARNAME, CIRCLES_MAX) \
    __asm__( \
        ".section .rodata\n" \
        ".global "#VARNAME"\n" \
        #VARNAME":\n" \
        ".ascii \"#version 100\\n\"\n" \
        ".ascii \"#define CIRCLES_MAX "#CIRCLES_MAX"\\n\"\n" \
        ".incbin \""PATH"\"\n" \
        ".byte 0\n" \
    ); \
    extern const char VARNAME[];

#define EMBED_SHADER_1(PATH, VARNAME, CIRCLES_MAX) EMBED_SHADER_IMPL(PATH, VARNAME, CIRCLES_MAX);
#define EMBED_SHADER(PATH, VARNAME) EMBED_SHADER_1(PATH, VARNAME, CIRCLES_MAX);

EMBED_SHADER("normal.frag", normal_shader_source);
EMBED_SHADER("circle.frag", circle_shader_source);

#define SET_CONSTANT(SHADER, IDENTIFIER, VALUE, UNIFORM_TYPE) \
    do { \
        __typeof__((VALUE)) SET_CONSTANT_value = (VALUE); \
        __typeof__((SHADER)) SET_CONSTANT_shader = (SHADER); \
        SetShaderValue(SET_CONSTANT_shader, \
                       GetShaderLocation(SET_CONSTANT_shader, IDENTIFIER), \
                       &SET_CONSTANT_value, UNIFORM_TYPE); \
    } while (0);

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

    // SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(SCREEN_W, SCREEN_H, "Boot screen");
    SetTargetFPS(30);
    HideCursor();

    float start_r = 0;
    float max_r = sqrtf(SCREEN_H * SCREEN_H + SCREEN_W * SCREEN_W) / 2;
    float acceleration = 2 * (max_r - start_r) / EXPECTED_TIME / EXPECTED_TIME;

    Shader normal_shader = LoadShaderFromMemory(NULL, normal_shader_source);
    assert(IsShaderValid(normal_shader));
    rlSetBlendFactors(RL_ONE_MINUS_DST_COLOR, RL_ZERO, RL_FUNC_ADD);

    Shader circle_shader = LoadShaderFromMemory(NULL, circle_shader_source);
    if (!IsShaderValid(circle_shader)) {
        printf("INVALID CIRCLE SHADER:\n");
        printf(circle_shader_source);
        return 1;
    }
    int circle_shader_t = GetShaderLocation(circle_shader, "t");
    int circle_shader_xs = GetShaderLocation(circle_shader, "xs");
    int circle_shader_ys = GetShaderLocation(circle_shader, "ys");
    int circle_shader_start_times = GetShaderLocation(circle_shader, "start_times");
    int circle_shader_circles_n = GetShaderLocation(circle_shader, "circles_n");
    SET_CONSTANT(circle_shader, "acceleration", acceleration, SHADER_UNIFORM_FLOAT);

    const char *upper_text = "Entering the Void...";
    int upper_font_size = 30;
    int lower_font_size = 16;
    int upper_text_w = MeasureText(upper_text, upper_font_size);

    int xs[CIRCLES_MAX] = {SCREEN_W/2};
    int ys[CIRCLES_MAX] = {SCREEN_H/2};
    float start_times[CIRCLES_MAX] = {};
    size_t circles_n = 1;

    SetShaderValueV(circle_shader, circle_shader_xs, xs, SHADER_UNIFORM_INT, CIRCLES_MAX);
    SetShaderValueV(circle_shader, circle_shader_ys, ys, SHADER_UNIFORM_INT, CIRCLES_MAX);
    SetShaderValueV(circle_shader, circle_shader_start_times,
                    start_times, SHADER_UNIFORM_FLOAT, CIRCLES_MAX);
    SetShaderValue(circle_shader, circle_shader_circles_n, &circles_n, SHADER_UNIFORM_INT);

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

        bool triggers_update = false;
        while (GetKeyPressed()) {
            if (circles_n >= CIRCLES_MAX) break;
            xs[circles_n] = rand() % SCREEN_W;
            ys[circles_n] = rand() % SCREEN_H;
            start_times[circles_n] = t;
            circles_n++;
            triggers_update = true;
        }

        if (triggers_update) {
            SetShaderValueV(circle_shader, circle_shader_xs, xs, SHADER_UNIFORM_INT, CIRCLES_MAX);
            SetShaderValueV(circle_shader, circle_shader_ys, ys, SHADER_UNIFORM_INT, CIRCLES_MAX);
            SetShaderValueV(circle_shader, circle_shader_start_times,
                            start_times, SHADER_UNIFORM_FLOAT, CIRCLES_MAX);
            SetShaderValue(circle_shader, circle_shader_circles_n, &circles_n, SHADER_UNIFORM_INT);
        }

        BeginDrawing();
            ClearBackground(WHITE);
            SetShaderValue(circle_shader, circle_shader_t, &t, SHADER_UNIFORM_FLOAT);
            BeginShaderMode(circle_shader);
                DrawRectangle(0, 0, SCREEN_W, SCREEN_H, WHITE);
                // ClearBackground(WHITE);
            EndShaderMode();

            BeginBlendMode(BLEND_CUSTOM);

                BeginShaderMode(normal_shader);
                    DrawText(upper_text,
                             (SCREEN_W - upper_text_w) / 2,
                             (SCREEN_H - upper_font_size) / 2,
                             upper_font_size, WHITE);
                    char buf[16];
                    snprintf(buf, sizeof(buf), "%.1fs", t);
                    int lower_text_w = MeasureText(buf, lower_font_size);
                    DrawText(buf,
                             (SCREEN_W - lower_text_w) / 2,
                             (SCREEN_H - lower_font_size) / 2 + 40,
                             lower_font_size, WHITE);
                EndShaderMode();
            EndBlendMode();
        EndDrawing();
    }
    CloseWindow();
    return 0;
}

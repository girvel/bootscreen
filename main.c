#include <raylib.h>

int main(void)
{
    InitWindow(1280, 720, "Boot screen");
    HideCursor();
    while (!WindowShouldClose()) {
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawText("Boot screen", 190, 200, 20, BLACK);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}

#pragma once

#define GAME_NAME "GAME"
#define GAME_WIDTH 400
#define GAME_HEIGHT 240
#define GAME_BPP 32
#define GAME_FRAME_SIZE (GAME_WIDTH * GAME_HEIGHT * (GAME_BPP / 8))
#define AVG_FPS_SAMPLE_SIZE 100 

#pragma warning(disable: 4820) // Disable structure padding warning
#pragma warning(disable: 5045) // Disable Spectre warning

typedef struct GameBitmap
{
    BITMAPINFO bitmap_info;
    void* memory;
} GameBitmap;

typedef struct Pixel32
{
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t alpha;
} Pixel32;

typedef struct PerformanceMetrics
{
    uint64_t total_frames_rendered;
    uint32_t raw_fps_avg;
    uint32_t fps_avg;
    LARGE_INTEGER frequency;
    LARGE_INTEGER frame_start;
    LARGE_INTEGER frame_end;
    LARGE_INTEGER elapsed_microseconds;
    int32_t monitor_width;
    int32_t monitor_height;
} PerformanceMetrics;

LRESULT CALLBACK main_window_procedure(_In_ HWND window, _In_ UINT message,
                                       _In_ WPARAM w_param,
                                       _In_ LPARAM l_param);
DWORD create_game_window(_In_ HINSTANCE instance);
BOOL another_instance_is_active(void);
void process_player_input(void);
void render_graphics(void);
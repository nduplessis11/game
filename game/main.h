#pragma once
#pragma warning(disable: 4820) // Disable structure padding warning
#pragma warning(disable: 5045) // Disable Spectre warning

#define GAME_NAME "GAME"
#define GAME_WIDTH 400
#define GAME_HEIGHT 240
#define VID_BPP 32
#define VID_BUFFER_SIZE (GAME_WIDTH * GAME_HEIGHT * (VID_BPP / 8))
#define AVG_FPS_SAMPLE_SIZE 100 
#define TARGET_MICROSECONDS_PER_FRAME 16667

#define SCREEN_ORIGIN ((GAME_WIDTH * GAME_HEIGHT) - GAME_WIDTH)
#define DRAW_PIXEL(X, Y) (SCREEN_ORIGIN - (GAME_WIDTH * (Y)) + (X))

#define KEY_STATE_DOWN(K) ((K) & 0x8000)

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

typedef struct DebugInfo
{
    double fps_avg;
    double fps_avg_raw;
    int32_t monitor_width;
    int32_t monitor_height;
    BOOL display_debug_info;
} DebugInfo;

LRESULT CALLBACK main_window_procedure(_In_ HWND window, _In_ UINT message,
                                       _In_ WPARAM w_param,
                                       _In_ LPARAM l_param);
DWORD create_game_window(_In_ HINSTANCE instance);
BOOL another_instance_is_active(void);
void process_player_input(void);
void render_graphics(void);
void clear_screen(_In_ __m128i color);
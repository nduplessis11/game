#pragma warning(push)
#pragma warning(disable: 4668)
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <emmintrin.h>
#pragma warning(pop)

#include "main.h"

HWND g_window = { 0 };
BOOL g_game_is_running = FALSE;
GameBitmap g_bitmap = { 0 };
DebugInfo g_debug_info = { 0 };

int WINAPI WinMain(_In_ HINSTANCE instance,
                   _In_opt_ HINSTANCE previous_instance,
                   _In_ PSTR command_line, _In_ int command_show)
{
    UNREFERENCED_PARAMETER(previous_instance);
    UNREFERENCED_PARAMETER(command_line);
    UNREFERENCED_PARAMETER(command_show);

    MSG message = { 0 };
    uint64_t frame_start = 0;
    uint64_t frame_end = 0;
    uint64_t total_frames = 0;
    uint64_t microseconds_per_frame = 0;
    uint64_t total_microseconds_raw = 0;
    uint64_t total_microseconds = 0;
    uint64_t frequency = 0;


    if (another_instance_is_active() == TRUE)
    {
        MessageBoxA(NULL,
                    "Another instance of this program is already running!",
                    "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }

    if (create_game_window(instance) != ERROR_SUCCESS)
    {
        MessageBoxA(NULL,
                    "Something went wrong!",
                    "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }

    QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);

    g_bitmap.bitmap_info.bmiHeader.biSize =
        sizeof(g_bitmap.bitmap_info.bmiHeader);
    g_bitmap.bitmap_info.bmiHeader.biWidth = GAME_WIDTH;
    g_bitmap.bitmap_info.bmiHeader.biHeight = GAME_HEIGHT;
    g_bitmap.bitmap_info.bmiHeader.biBitCount = VID_BPP;
    g_bitmap.bitmap_info.bmiHeader.biCompression = BI_RGB;
    g_bitmap.bitmap_info.bmiHeader.biPlanes = 1;
    g_bitmap.memory = VirtualAlloc(NULL, VID_BUFFER_SIZE,
                                   MEM_RESERVE | MEM_COMMIT,
                                   PAGE_READWRITE);
    if (g_bitmap.memory == NULL)
    {
        MessageBoxA(NULL, "Failed to allocate video buffer!", "Error!",
                    MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }

    memset(g_bitmap.memory, 0xAF, VID_BUFFER_SIZE);

    g_game_is_running = TRUE;
    while (g_game_is_running == TRUE)
    {
        QueryPerformanceCounter((LARGE_INTEGER*)&frame_start);
        while (PeekMessageA(&message, g_window, 0, 0, PM_REMOVE))
        {
            // Keyboard and mouse conversion
            TranslateMessage(&message);
            // Sends to my callback function main_window_procedure
            DispatchMessageA(&message);
        }

        process_player_input();
        render_graphics();

        // Before frame limiting
        QueryPerformanceCounter((LARGE_INTEGER*)&frame_end);
        microseconds_per_frame = frame_end - frame_start;
        microseconds_per_frame *= 1000000;
        microseconds_per_frame /= frequency;
        total_frames++;
        total_microseconds_raw += microseconds_per_frame;

        // Spend the same amount of time on each frame
        // Might want to use Win32 multimedia timers here
        while (microseconds_per_frame < TARGET_MICROSECONDS_PER_FRAME)
        {
            microseconds_per_frame = frame_end - frame_start;
            microseconds_per_frame *= 1000000;
            microseconds_per_frame /= frequency;
            QueryPerformanceCounter((LARGE_INTEGER*)&frame_end);
        }
        total_microseconds += microseconds_per_frame;

        if (total_frames == AVG_FPS_SAMPLE_SIZE)
        {
            g_debug_info.fps_avg_raw =
                AVG_FPS_SAMPLE_SIZE / (total_microseconds_raw * 0.000001f);

            g_debug_info.fps_avg =
                AVG_FPS_SAMPLE_SIZE / (total_microseconds * 0.000001f);

            total_microseconds_raw = 0;
            total_microseconds = 0;
            total_frames = 0;
        }
    }

Exit:
    return 0;
}

LRESULT CALLBACK main_window_procedure(_In_ HWND window, _In_ UINT message,
                                       _In_ WPARAM w_param,
                                       _In_ LPARAM l_param)
{
    LRESULT result = 0;

    switch (message)
    {
    case WM_CLOSE:
        g_game_is_running = FALSE;
        PostQuitMessage(0);
        result = 0;
        break;

    default:
        result = DefWindowProcA(window, message, w_param, l_param);
    }

    return result;
}

DWORD create_game_window(_In_ HINSTANCE instance)
{
    DWORD result = ERROR_SUCCESS;
    WNDCLASSEXA window_class = { 0 };
    const char CLASS_NAME[] = GAME_NAME "_WINDOW_CLASS";
    MONITORINFO monitor_info = { sizeof(MONITORINFO) };
    HMONITOR monitor = { 0 };

    window_class.cbSize = sizeof(WNDCLASSEXA);
    window_class.lpfnWndProc = main_window_procedure;
    window_class.hInstance = instance;
    window_class.lpszClassName = CLASS_NAME;
    window_class.hIcon = LoadIconA(NULL, IDI_APPLICATION);
    window_class.hIconSm = LoadIconA(NULL, IDI_APPLICATION);
    window_class.hCursor = LoadCursorA(NULL, IDC_ARROW);
    window_class.hbrBackground = CreateSolidBrush(RGB(175, 15, 15));

    if (RegisterClassExA(&window_class) == 0)
    {
        result = GetLastError();
        MessageBoxA(NULL, "Window Registration Failed!", "Error!",
                    MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }

    g_window = CreateWindowExA(0, CLASS_NAME, GAME_NAME,
                               WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT,
                               CW_USEDEFAULT, GAME_WIDTH * 3, GAME_HEIGHT * 3,
                               NULL, NULL, instance, NULL);

    if (g_window == NULL)
    {
        result = GetLastError();
        MessageBoxA(NULL, "Window Creation Failed!", "Error!",
                    MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }

    monitor = MonitorFromWindow(g_window, MONITOR_DEFAULTTOPRIMARY);
    if (GetMonitorInfoA(monitor, &monitor_info) == 0)
    {
        result = ERROR_MONITOR_NO_DESCRIPTOR;
        MessageBoxA(NULL, "Failed to get monitor info!", "Error!",
                    MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }

    // In dual monitor setups screen position(s) could be > (0, 0)
    g_debug_info.monitor_width =
        monitor_info.rcMonitor.right - monitor_info.rcMonitor.left;
    g_debug_info.monitor_height =
        monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;

    if (SetWindowLongPtrA(g_window, GWL_STYLE, WS_VISIBLE) == 0)
    {
        result = GetLastError();
        MessageBoxA(NULL, "Failed to set window to borderless fullscreen!",
                    "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }

    if (SetWindowPos(g_window, HWND_TOP,
                     monitor_info.rcMonitor.left,
                     monitor_info.rcMonitor.top,
                     g_debug_info.monitor_width,
                     g_debug_info.monitor_height,
                     SWP_FRAMECHANGED) == FALSE)
    {
        result = GetLastError();
        MessageBoxA(NULL, "Failed to set window position!", "Error!",
                    MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }

Exit:
    return result;
}

BOOL another_instance_is_active(void)
{
    HANDLE mutex = NULL;
    mutex = CreateMutexA(NULL, FALSE, GAME_NAME "_GameMutex");

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void process_player_input(void)
{
    int16_t quit_key_state = GetAsyncKeyState(VK_ESCAPE);
    int16_t debug_key_state = GetAsyncKeyState(VK_F1);
    static BOOL debug_key_was_down;

    if (KEY_STATE_DOWN(quit_key_state))
    {
        SendMessageA(g_window, WM_CLOSE, 0, 0);
    }

    if (KEY_STATE_DOWN(debug_key_state) && !debug_key_was_down)
    {
        g_debug_info.display_debug_info = !g_debug_info.display_debug_info;
    }

    // Display performance stats
    debug_key_was_down = KEY_STATE_DOWN(debug_key_state);
}

void render_graphics(void)
{
    __m128i green_pixel = { 0x00, 0x6c, 0x00, 0xff,
                            0x00, 0x6c, 0x00, 0xff,
                            0x00, 0x6c, 0x00, 0xff,
                            0x00, 0x6c, 0x00, 0xff };
    __m128i blue_pixel = { 0xea, 0x00, 0x00, 0xff,
                           0xea, 0x00, 0x00, 0xff,
                           0xea, 0x00, 0x00, 0xff,
                           0xea, 0x00, 0x00, 0xff };
    __m128i red_pixel = { 0x13, 0x00, 0xec, 0xff,
                          0x13, 0x00, 0xec, 0xff,
                          0x13, 0x00, 0xec, 0xff,
                          0x13, 0x00, 0xec, 0xff };
    __m128i clear_pixel = { 0xcf, 0x00, 0xdf, 0xff,
                            0xcf, 0x00, 0xdf, 0xff,
                            0xcf, 0x00, 0xdf, 0xff,
                            0xcf, 0x00, 0xdf, 0xff };

    int32_t x0 = 40;
    int32_t y0 = 104;

    clear_screen(clear_pixel);

    // Draw grass 4 vertices at a time with SIMD
    for (int x = 0; x < (GAME_WIDTH * GAME_HEIGHT) / 2; x += 4)
    {
        _mm_store_si128((Pixel32*)g_bitmap.memory + x, green_pixel);
    }

    // Draw sky 4 vertices at a time with SIMD
    for (int x = (GAME_WIDTH * GAME_HEIGHT) / 2;
         x < (GAME_WIDTH * GAME_HEIGHT); x += 4)
    {
        _mm_store_si128((Pixel32*)g_bitmap.memory + x, blue_pixel);
    }

    // 16x16 Sprite
    for (int32_t y = 0; y < 16; y++)
    {
        for (int32_t x = 0; x < 16; x += 4)
        {
            _mm_store_si128(
                (Pixel32*)g_bitmap.memory + DRAW_PIXEL(x0 + x, y0 + y),
                red_pixel);
        }
    }
    HDC device_context = GetDC(g_window);

    // Might replace with BitBlit if performance is needing
    StretchDIBits(device_context, 0, 0, g_debug_info.monitor_width,
                  g_debug_info.monitor_height, 0, 0, GAME_WIDTH,
                  GAME_HEIGHT, g_bitmap.memory,
                  &g_bitmap.bitmap_info, DIB_RGB_COLORS, SRCCOPY);

    if (g_debug_info.display_debug_info == TRUE)
    {
        char debug_text_buffer[64] = { 0 };

        sprintf_s(debug_text_buffer, sizeof(debug_text_buffer),
                  "FPS Raw: %.1f", g_debug_info.fps_avg_raw);
        TextOutA(device_context, 0, 0, debug_text_buffer,
                 (int)strlen(debug_text_buffer));

        sprintf_s(debug_text_buffer, sizeof(debug_text_buffer), "FPS: %.1f",
                  g_debug_info.fps_avg);
        TextOutA(device_context, 0, 16, debug_text_buffer,
                 (int)strlen(debug_text_buffer));
    }

    ReleaseDC(g_window, device_context);
}

__forceinline void clear_screen(_In_ __m128i color)
{
    for (int x = 0; x < (GAME_WIDTH * GAME_HEIGHT); x += 4)
    {
        _mm_store_si128((Pixel32*)g_bitmap.memory + x, color);
    }
}
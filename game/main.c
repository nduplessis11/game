#pragma warning(push)
#pragma warning(disable: 4668)
#include <windows.h>
#pragma warning(pop)

#include <stdio.h>
#include <stdint.h>

#include "main.h"

HWND g_window = { 0 };
BOOL g_game_is_running = FALSE;
GameBitmap g_video_buffer = { 0 };
PerformanceMetrics g_perf_metrics = { 0 };

int WINAPI WinMain(_In_ HINSTANCE instance,
                   _In_opt_ HINSTANCE previous_instance,
                   _In_ PSTR command_line, _In_ int command_show)
{
    UNREFERENCED_PARAMETER(previous_instance);
    UNREFERENCED_PARAMETER(command_line);
    UNREFERENCED_PARAMETER(command_show);

    if (another_instance_is_active() == TRUE)
    {
        MessageBoxA(NULL,
                    "Another instance of this program is already running!",
                    "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

    if (create_game_window(instance) != ERROR_SUCCESS)
    {
        goto Exit;
    }
    QueryPerformanceFrequency(&g_perf_metrics.frequency);

    g_video_buffer.bitmap_info.bmiHeader.biSize =
        sizeof(g_video_buffer.bitmap_info.bmiHeader);
    g_video_buffer.bitmap_info.bmiHeader.biWidth = GAME_WIDTH;
    g_video_buffer.bitmap_info.bmiHeader.biHeight = GAME_HEIGHT;
    g_video_buffer.bitmap_info.bmiHeader.biBitCount = GAME_BPP;
    g_video_buffer.bitmap_info.bmiHeader.biCompression = BI_RGB;
    g_video_buffer.bitmap_info.bmiHeader.biPlanes = 1;
    g_video_buffer.memory = VirtualAlloc(NULL, GAME_FRAME_SIZE,
                                         MEM_RESERVE | MEM_COMMIT,
                                         PAGE_READWRITE);
    if (g_video_buffer.memory == NULL)
    {
        MessageBoxA(NULL, "Failed to allocate frame buffer!", "Error!",
                    MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }

    memset(g_video_buffer.memory, 0xAF, GAME_FRAME_SIZE);

    g_game_is_running = TRUE;

    MSG message = { 0 };
    while (g_game_is_running == TRUE)
    {
        QueryPerformanceCounter(&g_perf_metrics.frame_start);
        while (PeekMessageA(&message, g_window, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }

        process_player_input();
        render_graphics();

        QueryPerformanceCounter(&g_perf_metrics.frame_end);

        g_perf_metrics.elapsed_microseconds.QuadPart =
            g_perf_metrics.frame_end.QuadPart -
            g_perf_metrics.frame_start.QuadPart;

        g_perf_metrics.elapsed_microseconds.QuadPart *= 1000000;
        g_perf_metrics.elapsed_microseconds.QuadPart /=
            g_perf_metrics.frequency.QuadPart;

        Sleep(1);
        g_perf_metrics.total_frames_rendered++;
        if (g_perf_metrics.total_frames_rendered % AVG_FPS_SAMPLE_SIZE == 0)
        {
            char str[64] = { 0 };
            _snprintf_s(str, _countof(str), _TRUNCATE,
                        "Elapsed microseconds: %lli\n",
                        g_perf_metrics.elapsed_microseconds.QuadPart);

            OutputDebugStringA(str);
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

    MONITORINFO monitor_info = { sizeof(MONITORINFO) };
    HMONITOR monitor = { 0 };
    monitor = MonitorFromWindow(g_window, MONITOR_DEFAULTTOPRIMARY);
    if (GetMonitorInfoA(monitor, &monitor_info) == 0)
    {
        result = ERROR_MONITOR_NO_DESCRIPTOR;
        MessageBoxA(NULL, "Failed to get monitor info!", "Error!",
                    MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }

    // In dual monitor setups position could be nonzero
    g_perf_metrics.monitor_width =
        monitor_info.rcMonitor.right -
        monitor_info.rcMonitor.left;
    g_perf_metrics.monitor_height =
        monitor_info.rcMonitor.bottom -
        monitor_info.rcMonitor.top;

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
                     g_perf_metrics.monitor_width,
                     g_perf_metrics.monitor_height,
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
    const int16_t KEYDOWN_MASK = 0x8001;
    int16_t escape_key_state = GetAsyncKeyState(VK_ESCAPE);

    if (escape_key_state & KEYDOWN_MASK)
    {
        SendMessageA(g_window, WM_CLOSE, 0, 0);
    }
}

void render_graphics(void)
{
    Pixel32 green_pixel = { 0 };
    green_pixel.blue = 0x00;
    green_pixel.green = 0x6c;
    green_pixel.red = 0x00;
    green_pixel.alpha = 0xff;

    Pixel32 blue_pixel = { 0 };
    blue_pixel.blue = 0xea;
    blue_pixel.green = 0x00;
    blue_pixel.red = 0x00;
    blue_pixel.alpha = 0xff;

    // Draw grass
    for (int x = 0; x < (GAME_WIDTH * GAME_HEIGHT) / 2; x++)
    {
        memcpy_s((Pixel32*)g_video_buffer.memory + x, sizeof(green_pixel),
                 &green_pixel, sizeof(green_pixel));
    }

    // Draw sky
    for (int x = (GAME_WIDTH * GAME_HEIGHT) / 2;
         x < (GAME_WIDTH * GAME_HEIGHT); x++)
    {
        memcpy_s((Pixel32*)g_video_buffer.memory + x, sizeof(blue_pixel),
                 &blue_pixel, sizeof(blue_pixel));
    }

    HDC device_context = GetDC(g_window);

    // Might replace with BitBlit if performance is needing
    StretchDIBits(device_context, 0, 0, g_perf_metrics.monitor_width,
                  g_perf_metrics.monitor_height, 0, 0, GAME_WIDTH,
                  GAME_HEIGHT, g_video_buffer.memory,
                  &g_video_buffer.bitmap_info, DIB_RGB_COLORS, SRCCOPY);

    ReleaseDC(g_window, device_context);
}
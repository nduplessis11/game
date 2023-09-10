#pragma warning(push)
#pragma warning(disable: 4668)
#include <windows.h>
#pragma warning(pop)

#include <stdint.h>

#include "main.h"

HWND g_window = { 0 };
BOOL g_game_is_running = FALSE;
GameBitmap g_video_buffer = { 0 };
MONITORINFO g_monitor_info = { sizeof(MONITORINFO) };

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

while (g_game_is_running == TRUE)
{
    MSG message = { 0 };
    while (PeekMessageA(&message, g_window, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }

    process_player_input();
    render_graphics();

    Sleep(1);
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

    HMONITOR monitor = { 0 };
    monitor = MonitorFromWindow(g_window, MONITOR_DEFAULTTOPRIMARY);
    if (GetMonitorInfoA(monitor, &g_monitor_info) == 0)
    {
        result = ERROR_MONITOR_NO_DESCRIPTOR;
        MessageBoxA(NULL, "Failed to get monitor info!", "Error!",
                    MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }

    int monitor_width =
        g_monitor_info.rcMonitor.right - g_monitor_info.rcMonitor.left;
    int monitor_height =
        g_monitor_info.rcMonitor.bottom - g_monitor_info.rcMonitor.top;

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
    HDC device_context = GetDC(g_window);

    // Might replace with BitBlit if performance is needing
    StretchDIBits(device_context, 0, 0, GAME_WIDTH * 3, GAME_HEIGHT * 3, 0, 0,
                  GAME_WIDTH, GAME_HEIGHT, g_video_buffer.memory,
                  &g_video_buffer.bitmap_info, DIB_RGB_COLORS, SRCCOPY);

    ReleaseDC(g_window, device_context);
}
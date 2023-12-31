// James Wah (abrasive) 2023
// This file is in the public domain.

// handle a delightful Windows/X11 type name conflict
#define Status XStatus
#include <X11/Xlib.h>
#undef Status
#undef ControlMask

#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <stdio.h>
#include <wchar.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static const wchar_t whole_window_prop[] = L"__wine_x11_whole_window";

HANDLE hProc = NULL;

void cleanup(int signal) {
    if (hProc) {
        TerminateProcess(hProc, 0);
        hProc = NULL;
    }

    exit(0);
}

BOOL found_child = FALSE;
BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam) {
    found_child = TRUE;
}

Display *dpy = NULL;
int parent_xwindow;

void die(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    char msg[512];
    vsprintf(msg, format, ap);
    if (!msg)
        exit(1);

    printf("ERROR: %s\n", msg);

    XGCValues gc_values;
    gc_values.foreground = XWhitePixel(dpy, 0);
    GC gc = XCreateGC(dpy, parent_xwindow, GCForeground, &gc_values);
    XTextItem xti = {
        .chars = msg,
        .nchars = strlen(msg),
        };
    XDrawText(dpy, parent_xwindow, gc, 100, 100, &xti, 1);
    XFlush(dpy);

    Sleep(3000);

    exit(1);
}


int main(int argc, char **argv) {
    signal(SIGTERM, cleanup);
    signal(SIGINT, cleanup);

    dpy = XOpenDisplay(NULL);
    HINSTANCE hInstance = NULL;

    const char *parent_xwindow_str = getenv("XSCREENSAVER_WINDOW");
    if (!parent_xwindow_str) {
        printf("error: XSCREENSAVER_WINDOW is not set\n");
        return 1;
    }
    parent_xwindow = strtol(parent_xwindow_str, NULL, 0);

    Window root;
    int x, y;
    unsigned int width, height, border_width, depth;

    XGetGeometry(dpy, parent_xwindow, &root, &x, &y, &width, &height, &border_width, &depth);

    if (argc < 2) {
        printf("usage: %s screensaver.scr\n", argv[0]);
        return 1;
    }

    const wchar_t CLASS_NAME[]  = L"nightcap";

    WNDCLASS wc = { };

    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"nightcap.exe",
        WS_POPUP,

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,

        NULL,       // Parent window
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
        );

    if (hwnd == NULL)
    {
        return 0;
    }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

    char cmdline[MAX_PATH];
    wchar_t wcmdline[MAX_PATH];
    sprintf(cmdline, "\"%s\" /p %lu", argv[1], (uintptr_t)hwnd);
    MultiByteToWideChar(CP_UTF8, 0, cmdline, -1, wcmdline, MAX_PATH);

    if (!CreateProcess(NULL,
        wcmdline,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi)) {
        die("CreateProcess failed (%d). Probably the .scr was not found?", GetLastError());
    }

    while (!found_child) {
        EnumChildWindows(hwnd, EnumChildProc, 0);
        Sleep(5);
    }

    RECT winsize;
    GetWindowRect(hwnd, &winsize);

    hProc = pi.hProcess;

    int our_xwindow = (uintptr_t)GetPropW(hwnd, whole_window_prop);

    int result = XReparentWindow(dpy, our_xwindow, parent_xwindow, 0, 0);
    XFlush(dpy);
    ShowWindow(hwnd, SW_SHOW);

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        if (hProc) {
            TerminateProcess(hProc, 0);
        }
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

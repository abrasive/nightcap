#define Status XStatus
#include <X11/Xlib.h>
#undef Status
#undef ControlMask

#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//Returns the last Win32 error, in string format. Returns an empty string if there is no error.
const char * GetLastErrorAsString()
{
    //Get the error message ID, if any.
    DWORD errorMessageID = GetLastError();
    if(errorMessageID == 0) {
        return NULL;
    }

    LPSTR messageBuffer = NULL;

    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
    return strdup(messageBuffer);
}

static const WCHAR whole_window_prop[] =
    {'_','_','w','i','n','e','_','x','1','1','_','w','h','o','l','e','_','w','i','n','d','o','w',0};

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

int main(int argc, char **argv) {
    signal(SIGTERM, cleanup);
    signal(SIGINT, cleanup);

    Display *d = XOpenDisplay(NULL);
    printf("xconn: %p\n", d);
    HINSTANCE hInstance = NULL;

    const char *parent_xwindow_str = getenv("XSCREENSAVER_WINDOW");
    if (!parent_xwindow_str) {
        printf("error: XSCREENSAVER_WINDOW is not set\n");
        return 1;
    }
    int parent_xwindow = strtol(parent_xwindow_str, NULL, 0);

    Window root;
    int x, y;
    unsigned int width, height, border_width, depth;

    XGetGeometry(d, parent_xwindow, &root, &x, &y, &width, &height, &border_width, &depth);
    printf("size: %dx%d bw %d dep %d\n", width, height, border_width, depth);

    if (argc < 2) {
        printf("usage: %s screensaver.scr\n", argv[0]);
        return 1;
    }
    printf("screensaver: %s\n", argv[1]);

    const wchar_t CLASS_NAME[]  = L"Sample Window Class";

    WNDCLASS wc = { };

    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    // Create the window.

    HWND hwnd = CreateWindowEx(
        0,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        L"Learn to Program Windows",    // Window text
        WS_POPUP,            // Window style

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

    char cmdline[256];

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

    sprintf(cmdline, "\"%s\" /p %lu", argv[1], (uintptr_t)hwnd);
    printf("cmdline: %s\n", cmdline);

    if (!CreateProcessA( argv[1],
        cmdline,        // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi )           // Pointer to PROCESS_INFORMATION structure
       ) {
        printf( "CreateProcess failed (%s).\n", GetLastErrorAsString() );
        return 1;
    }

    while (!found_child) {
        EnumChildWindows(hwnd, EnumChildProc, 0);
        Sleep(5);
        printf(".");
    }

    RECT winsize;
    GetWindowRect(hwnd, &winsize);
    printf("rect: %d x %d @ %d,%d", winsize.right - winsize.left, winsize.bottom - winsize.top, winsize.left, winsize.top);

    hProc = pi.hProcess;

    int our_xwindow = (uintptr_t)GetPropW(hwnd, whole_window_prop);
    printf("reparenting %x into %x\n", our_xwindow, parent_xwindow);

    XGetGeometry(d, our_xwindow, &root, &x, &y, &width, &height, &border_width, &depth);
    printf("our pre-reparent size: %dx%d bw %d dep %d\n", width, height, border_width, depth);


    int result = XReparentWindow(d, our_xwindow, parent_xwindow, 0, 0);
    XFlush(d);
    XGetGeometry(d, our_xwindow, &root, &x, &y, &width, &height, &border_width, &depth);
    printf("our post-reparent size: %dx%d bw %d dep %d\n", width, height, border_width, depth);
    ShowWindow(hwnd, SW_SHOW);
    printf("result: %d\n", result);

    // Run the message loop.

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

#ifndef CLIPBOARD_WIN_H
#define CLIPBOARD_WIN_H

#include <stdio.h>
#include <windows.h>

#define WC_CLIPBOARD_CLASS_NAME L"CLIPBOARD"
#define WM_TRAY_CALLBACK_MESSAGE (WM_USER + 1)
#define ID_TRAY_FIRST 1000

static WNDCLASSEX wc;
static HWND hwnd;
static HWND hwndnextviewer;
static void *_context = NULL;
static void (*_clipboard_onchange_text)(void *, const char *, int);
static int change_from_set = 0;

static void _clipboard_onchange(HWND hwnd)
{
    if (!_clipboard_onchange_text) 
    {
        return;
    }

    if (!OpenClipboard(hwnd))
    {
        return;
    }

    HGLOBAL wbuf_handle = GetClipboardData(CF_UNICODETEXT);
    if (wbuf_handle == NULL)
    {
        CloseClipboard();
        return;
    }

    const WCHAR* wbuf_global = GlobalLock(wbuf_handle);
    if (wbuf_global)
    {
        int buf_len = WideCharToMultiByte(CP_UTF8, 0, wbuf_global, -1, NULL, 0, NULL, NULL);
        char *text = (char *)malloc(buf_len);
        WideCharToMultiByte(CP_UTF8, 0, wbuf_global, -1, text, buf_len, NULL, NULL);
        _clipboard_onchange_text(_context, text, change_from_set);
        free(text);
    }
    change_from_set = 0;
    GlobalUnlock(wbuf_handle);
    CloseClipboard();
}

static void clipboard_settext(const char *text)
{
    if (!OpenClipboard(NULL))
    {
        return;
    }
    const int wbuf_length = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    HGLOBAL wbuf_handle = GlobalAlloc(GMEM_MOVEABLE, (SIZE_T)wbuf_length * sizeof(WCHAR));
    if (wbuf_handle == NULL)
    {
        CloseClipboard();
        return;
    }
    WCHAR *wbuf_global = (WCHAR *)GlobalLock(wbuf_handle);
    MultiByteToWideChar(CP_UTF8, 0, text, -1, wbuf_global, wbuf_length);
    GlobalUnlock(wbuf_handle);
    EmptyClipboard();
    if (SetClipboardData(CF_UNICODETEXT, wbuf_handle) == NULL)
    {
        GlobalFree(wbuf_handle);
    }
    else
    {
        change_from_set = 1;
    }
    CloseClipboard();
}

static LRESULT CALLBACK _wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_CREATE:
        hwndnextviewer = SetClipboardViewer(hwnd);
        return 0;
    case WM_CHANGECBCHAIN:
        // If the next window is closing, repair the chain.
        if ((HWND)wparam == hwndnextviewer)
        {
            hwndnextviewer = (HWND)lparam;
        }
        else if (hwndnextviewer != NULL)
        {
            // Otherwise, pass the message to the next link.
            SendMessage(hwndnextviewer, msg, wparam, lparam);
        }
        return 0;
    case WM_DRAWCLIPBOARD:
        SendMessage(hwndnextviewer, msg, wparam, lparam);
        _clipboard_onchange(hwnd);
        return 0;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        ChangeClipboardChain(hwnd, hwndnextviewer);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

static int clipboard_init(void *context, void (*p)(void *, const char *, int))
{
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = _wnd_proc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = WC_CLIPBOARD_CLASS_NAME;
    if (!RegisterClassEx(&wc))
    {
        return -1;
    }

    hwnd = CreateWindowEx(0, WC_CLIPBOARD_CLASS_NAME, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (hwnd == NULL)
    {
        return -1;
    }

    _context = context;
    _clipboard_onchange_text = p;

    UpdateWindow(hwnd);
    return 0;
}

static int clipboard_loop()
{
    MSG msg;
    PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
    if (msg.message == WM_QUIT)
    {
        return -1;
    }
    TranslateMessage(&msg);
    DispatchMessage(&msg);
    return 0;
}

static void clipboard_exit()
{
    PostQuitMessage(0);
    UnregisterClass(WC_CLIPBOARD_CLASS_NAME, GetModuleHandle(NULL));
}

#endif

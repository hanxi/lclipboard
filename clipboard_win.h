#ifndef CLIPBOARD_WIN_H
#define CLIPBOARD_WIN_H

#include <stdio.h>
#include <windows.h>

#define WC_CLIPBOARD_CLASS_NAME "CLIPBOARD"
#define WM_TRAY_CALLBACK_MESSAGE (WM_USER + 1)
#define ID_TRAY_FIRST 1000

static WNDCLASSEX wc;
static HWND hwnd;
static HWND hwndnextviewer;
static UINT format_list[] = {
    CF_TEXT,
};
static void *_context = NULL;
static void (*_clipboard_onchange_text)(void *, const char *, int);
static int change_from_set = 0;

char *asciitoutf8(const char *lpstr)
{
    int ulen = MultiByteToWideChar(CP_ACP, 0, lpstr, -1, NULL, 0);
    int bytes = (ulen + 1) * sizeof(wchar_t);
    wchar_t *utext = malloc(bytes);
    MultiByteToWideChar(CP_ACP, 0, lpstr, -1, utext, ulen);

    int sz = WideCharToMultiByte(CP_UTF8, 0, utext, -1, NULL, 0, NULL, NULL);
    char *utf8text = malloc(sz);
    WideCharToMultiByte(CP_UTF8, 0, utext, -1, utf8text, sz, NULL, NULL);
    free(utext);
    return utf8text;
}

static void _clipboard_onchange(HWND hwnd)
{
    UINT format = GetPriorityClipboardFormat(format_list, 1);
    if (format == CF_TEXT)
    {
        if (OpenClipboard(hwnd))
        {
            HGLOBAL hglb = GetClipboardData(format);
            LPSTR lpstr = GlobalLock(hglb);
            if (_clipboard_onchange_text)
            {
                char *utf8text = asciitoutf8(lpstr);
                _clipboard_onchange_text(_context, utf8text, change_from_set);
                free(utf8text);
            }
            change_from_set = 0;
            GlobalUnlock(hglb);
            CloseClipboard();
        }
    }
}

char *utf8toascii(const char *text, int *sz)
{
    int ulen = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    int bytes = (ulen + 1) * sizeof(wchar_t);
    wchar_t *utext = malloc(bytes);
    MultiByteToWideChar(CP_UTF8, 0, text, -1, utext, ulen);

    *sz = WideCharToMultiByte(CP_ACP, 0, utext, -1, NULL, 0, NULL, NULL);
    char *asciitext = malloc(*sz);
    WideCharToMultiByte(CP_ACP, 0, utext, -1, asciitext, *sz, NULL, NULL);
    free(utext);
    return asciitext;
}

static void clipboard_settext(const char *text)
{
    int sz = 0;
    char *asciitext = utf8toascii(text, &sz);
    HGLOBAL hmem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, sz);
    if (hmem != NULL)
    {
        if (OpenClipboard(hwnd))
        {
            PVOID pdata = GlobalLock(hmem);
            if (pdata != NULL)
            {
                CopyMemory(pdata, asciitext, sz);
                change_from_set = 1;
            }
            GlobalUnlock(hmem);
            if (EmptyClipboard())
            {
                if (SetClipboardData(CF_TEXT, hmem))
                {
                }
                else
                {
                    GlobalFree(hmem);
                }
            }
            CloseClipboard();
        }
        else
        {
            GlobalFree(hmem);
        }
    }
    free(asciitext);
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

#include <windows.h>
#include <windowsx.h>
#include "resource.h"
#include <thread>
#include <iostream>
#include "ConsoleScript.h"
#include <tlhelp32.h>
#include <vector>
#include <string>
#include <regex>

// Forward declarations from ConsoleScript.cpp
DWORD GetProcessIdByName(const wchar_t* processName);
bool PatchGameMemory(DWORD processId);
bool PatchModString(DWORD processId);

// Forward declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Global variables
HBITMAP g_hBackgroundBitmap = NULL;
HBITMAP g_hBackground2Bitmap = NULL;
HBITMAP g_hHintBitmap = NULL;
HBITMAP g_hHint2Bitmap = NULL;
HBITMAP g_hCreditsBitmap = NULL;
HBITMAP g_hCredits2Bitmap = NULL;
bool g_isBackground2 = false;
bool g_isCreditsShown = false;
std::wstring g_textField1, g_textField2, g_textField3;
COLORREF g_textColor1 = RGB(255, 0, 0), g_textColor2 = RGB(255, 0, 0), g_textColor3 = RGB(255, 0, 0);
bool g_showField2 = false, g_showField3 = false;
HWND g_hwnd = NULL;

void UpdateTextFields(int field, const std::wstring& text, COLORREF color) {
    switch (field) {
    case 1:
        g_textField1 = text;
        g_textColor1 = color;
        break;
    case 2:
        g_textField2 = text;
        g_textColor2 = color;
        g_showField2 = !text.empty();
        break;
    case 3:
        g_textField3 = text;
        g_textColor3 = color;
        g_showField3 = !text.empty();
        break;
    }
    InvalidateRect(g_hwnd, NULL, TRUE);
}

void RunConsoleScript() {
    std::cout << "Searching for ChronoArk.exe..." << std::endl;
    DWORD processId = 0;
    bool initialPatchDone = false;
    int withModPatchCount = 0;

    while (true) {
        processId = GetProcessIdByName(L"ChronoArk.exe");
        if (processId != 0) {
            UpdateTextFields(1, L"Game found!", RGB(0, 255, 0));

            if (!initialPatchDone) {
                UpdateTextFields(2, L"Attempting to patch...", RGB(255, 0, 0));
                if (PatchGameMemory(processId)) {
                    UpdateTextFields(2, L"Patch applied!", RGB(0, 255, 0));
                    initialPatchDone = true;
                    Sleep(1500);
                }
                else {
                    UpdateTextFields(2, L"Patch failed!", RGB(255, 0, 0));
                }
            }

            UpdateTextFields(3, L"", RGB(255, 0, 0));
            if (PatchModString(processId)) {
                withModPatchCount++;
                if (withModPatchCount == 1) {
                    UpdateTextFields(3, L"", RGB(255, 165, 0));
                }
                else if (withModPatchCount >= 2) {
                    UpdateTextFields(3, L"", RGB(0, 255, 0));
                }
            }
        }
        else {
            UpdateTextFields(1, L"No game running...", RGB(255, 0, 0));
            UpdateTextFields(2, L"", RGB(255, 0, 0));
            UpdateTextFields(3, L"", RGB(255, 0, 0));
            initialPatchDone = false;
            withModPatchCount = 0;
        }
        Sleep(500);
    }
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Register the window class
    const wchar_t CLASS_NAME[] = L"Chrono Ark Watermark Patcher Class";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));

    RegisterClass(&wc);

    // Calculate window size to achieve desired client area
    RECT rc = { 0, 0, 464, 300 };
    AdjustWindowRect(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);

    // Create the window
    HWND hwnd = CreateWindowEx(
        0,                      // Optional window styles
        CLASS_NAME,             // Window class
        L"Chrono Ark Watermark Patcher", // Window text
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, // Window style

        // Position and size
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,

        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
    );

    if (hwnd == NULL)
    {
        return 0;
    }

    g_hwnd = hwnd;

    ShowWindow(hwnd, nShowCmd);

    // Start console script in a new thread
    std::thread consoleThread(RunConsoleScript);
    consoleThread.detach();

    // Run the message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
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
    case WM_CREATE:
    {
        HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
        g_hBackgroundBitmap = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_BACKGROUND), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
        g_hBackground2Bitmap = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_BACKGROUND2), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
        g_hHintBitmap = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_HINT), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
        g_hHint2Bitmap = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_HINT2), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
        g_hCreditsBitmap = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_CREDITS), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
        g_hCredits2Bitmap = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_CREDITS2), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
        return 0;
    }

    case WM_DESTROY:
        if (g_hBackgroundBitmap) DeleteObject(g_hBackgroundBitmap);
        if (g_hBackground2Bitmap) DeleteObject(g_hBackground2Bitmap);
        if (g_hHintBitmap) DeleteObject(g_hHintBitmap);
        if (g_hHint2Bitmap) DeleteObject(g_hHint2Bitmap);
        if (g_hCreditsBitmap) DeleteObject(g_hCreditsBitmap);
        if (g_hCredits2Bitmap) DeleteObject(g_hCredits2Bitmap);
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, g_isBackground2 ? g_hBackground2Bitmap : g_hBackgroundBitmap);

        BITMAP bm;
        GetObject(g_isBackground2 ? g_hBackground2Bitmap : g_hBackgroundBitmap, sizeof(bm), &bm);
        BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);

        if (!g_isCreditsShown) {
            SelectObject(hdcMem, g_isBackground2 ? g_hHint2Bitmap : g_hHintBitmap);
            GetObject(g_isBackground2 ? g_hHint2Bitmap : g_hHintBitmap, sizeof(bm), &bm);
            BitBlt(hdc, 421, 250, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);

            // Draw text fields
            SetBkMode(hdc, TRANSPARENT);
            HFONT hFont = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
                CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"));
            HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

            SetTextColor(hdc, g_textColor1);
            TextOut(hdc, 277, 222, g_textField1.c_str(), g_textField1.length());

            if (g_showField2) {
                SetTextColor(hdc, g_textColor2);
                TextOut(hdc, 277, 244, g_textField2.c_str(), g_textField2.length());
            }

            if (g_showField3) {
                SetTextColor(hdc, g_textColor3);
                TextOut(hdc, 277, 266, g_textField3.c_str(), g_textField3.length());
            }

            SelectObject(hdc, hOldFont);
            DeleteObject(hFont);
        }

        // Draw credits last so it overlays everything
        if (g_isCreditsShown) {
            SelectObject(hdcMem, g_isBackground2 ? g_hCredits2Bitmap : g_hCreditsBitmap);
            GetObject(g_isBackground2 ? g_hCredits2Bitmap : g_hCreditsBitmap, sizeof(bm), &bm);
            BitBlt(hdc, 0, 0, 464, 300, hdcMem, 0, 0, SRCCOPY);
        }

        SelectObject(hdcMem, hbmOld);
        DeleteDC(hdcMem);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN:
    {
        int xPos = GET_X_LPARAM(lParam);
        int yPos = GET_Y_LPARAM(lParam);

        if (g_isCreditsShown) {
            g_isCreditsShown = false;
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;
        }

        if (xPos >= 421 && xPos <= 454 && yPos >= 250 && yPos <= 283) {
            g_isCreditsShown = true;
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;
        }

        if (!g_isBackground2 && xPos >= 74 && xPos <= 90 && yPos >= 250 && yPos <= 268) {
            g_isBackground2 = true;
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;
        }

        if (g_isBackground2 && xPos >= 133 && xPos <= 151 && yPos >= 233 && yPos <= 266) {
            g_isBackground2 = false;
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;
        }

        return 0;
    }

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
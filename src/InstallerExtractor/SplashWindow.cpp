/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "SplashWindow.hpp"

#include <stdexcept>

#include <Windows.h>
#include <VersionHelpers.h>

namespace {

const int g_borderWidth = 1;

SplashWindow* g_splashInst = nullptr;

void DrawWindowBorder(HDC hdc, int w, int h)
{
    const auto lightGray = RGB(214, 214, 214);
    auto       hPen      = CreatePen(PS_SOLID, 1, lightGray);
    SelectObject(hdc, hPen);
    MoveToEx(hdc, 0, 0, nullptr);

    LineTo(hdc, 0, h - 1);
    LineTo(hdc, w - 1, h - 1);
    LineTo(hdc, w - 1, 0);
    LineTo(hdc, 0, 0);

    DeleteObject(hPen);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
        case WM_PAINT:
            g_splashInst->Paint();
            break;
        case WM_DESTROY:
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
            break;
    }

    return 0;
}

}

SplashWindow::SplashWindow()
{
    g_splashInst = this;
}

SplashWindow::~SplashWindow()
{
    if (m_hWnd)
        DestroyWindow(m_hWnd);
    g_splashInst = nullptr;
}

void SplashWindow::CreateNativeWindow(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;
    TCHAR      szWindowClass[] = L"win32app"; // The main window class name.

    wcex.cbSize        = sizeof(WNDCLASSEX);
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = WndProc;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = 0;
    wcex.hInstance     = hInstance;
    wcex.hIcon         = nullptr;
    wcex.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    wcex.lpszMenuName  = nullptr;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm       = nullptr;

    if (!RegisterClassEx(&wcex))
        throw std::runtime_error("Call to RegisterClassEx failed!");

    m_hWnd = CreateWindowEx(WS_EX_TOOLWINDOW,
                            szWindowClass,
                            nullptr,
                            WS_POPUP,
                            m_posX,
                            m_posY, // pos
                            m_width,
                            m_height, // size
                            nullptr,
                            nullptr,
                            hInstance,
                            nullptr);

    if (!m_hWnd)
        throw std::runtime_error("CreateWindow failed!");
}

void SplashWindow::LoadSplashBitmap()
{
    m_hBitmap = (HBITMAP) LoadImageW(GetModuleHandle(0), L"splash", IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE);
    if (!m_hBitmap)
        throw std::runtime_error("Failed to load splash file");

    {
        BITMAP bm;
        GetObject(m_hBitmap, sizeof(bm), &bm);
        m_bmWidth  = bm.bmWidth;
        m_width    = m_bmWidth + g_borderWidth * 2;
        m_bmHeight = bm.bmHeight;
        m_height   = m_bmHeight + g_borderWidth * 2;
    }

    {
        const int screenWidth  = GetSystemMetrics(SM_CXSCREEN);
        const int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        m_posX                 = (screenWidth - m_width) / 2;
        m_posY                 = (screenHeight - m_height) / 2;
    }
}

void SplashWindow::Show()
{
    ShowWindow(m_hWnd, SW_SHOW);
    UpdateWindow(m_hWnd);
}

void SplashWindow::Hide()
{
    if (m_hWnd)
        ShowWindow(m_hWnd, SW_HIDE);
}

void SplashWindow::Paint()
{
    PAINTSTRUCT ps;
    HDC         hdc = BeginPaint(m_hWnd, &ps);

    if (!m_painted) {
        SelectObject(ps.hdc, GetStockObject(NULL_PEN));
        SelectObject(ps.hdc, GetStockObject(WHITE_BRUSH));
        Rectangle(ps.hdc, 0, 0, m_width, m_height);

        HDC     hdcMem = CreateCompatibleDC(hdc);
        HGDIOBJ hbmOld = SelectObject(hdcMem, m_hBitmap);

        BitBlt(hdc, g_borderWidth, g_borderWidth, m_bmWidth, m_bmHeight, hdcMem, 0, 0, SRCCOPY);
        SelectObject(hdcMem, hbmOld);
        DeleteDC(hdcMem);

        DrawWindowBorder(ps.hdc, m_width, m_height);
    }
    EndPaint(m_hWnd, &ps);
    m_painted = true;
}

void SplashWindow::KeepResponding()
{
    MSG msg;
    PeekMessage(&msg, m_hWnd, 0, 0, PM_REMOVE);
}

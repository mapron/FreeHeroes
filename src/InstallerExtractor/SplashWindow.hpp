/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <string>

#ifndef _WINDEF_
struct HINSTANCE__;
typedef HINSTANCE__* HINSTANCE;
struct HWND__;
typedef HWND__* HWND;
struct HBITMAP__;
typedef HBITMAP__* HBITMAP;
#endif

class SplashWindow {
public:
    SplashWindow();
    ~SplashWindow();

    void CreateNativeWindow(HINSTANCE hInstance);
    void LoadSplashBitmap();
    void Show();
    void Hide();
    void Paint();
    void KeepResponding();

private:
    HBITMAP m_hBitmap = nullptr;
    HWND    m_hWnd    = nullptr;

    bool m_painted = false;

    int m_progressOffset = 0;

    int m_posX = 0, m_posY = 0;        /// положение окна
    int m_bmWidth = 0, m_bmHeight = 0; /// ширина и высота картинки сплеша
    int m_width = 0, m_height = 0;     /// полные размеры окна с учетом рамки и белого поля
};

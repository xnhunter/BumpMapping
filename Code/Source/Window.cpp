// Copyright ⓒ 2018 Valentyn Bondarenko. All rights reserved.

#include <StdAfx.h>

#include <Window.h>

#include <AppInfo.h>

namespace bm
{
    Window::Window(const int& width, const int& height, const bool& fullscreen) :
        name(app_info::name +
             " "s +
             std::to_string(app_info::major_version) + 
             "." + 
             std::to_string(app_info::minor_version)),
        class_name(app_info::name),
        width(width),
        height(height),
		fullscreen(fullscreen)
    {
        
    }

    void Window::registerClass()
    {
        class_ex.cbSize = sizeof(WNDCLASSEX);
        class_ex.style = CS_HREDRAW | CS_VREDRAW;
        class_ex.lpfnWndProc = process;
        class_ex.cbClsExtra = 0;
        class_ex.cbWndExtra = 0;
        class_ex.hInstance = GetModuleHandleA(nullptr);
        class_ex.hIcon = LoadIconA(nullptr, IDI_APPLICATION);
        class_ex.hCursor = LoadCursorA(nullptr, IDC_ARROW);
        class_ex.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        class_ex.lpszMenuName = nullptr;
        class_ex.lpszClassName = class_name.c_str();
        class_ex.hIconSm = LoadIconA(nullptr, IDI_APPLICATION);

        if(!RegisterClassExA(&class_ex))
            MessageBoxA(nullptr, "Error registering class", "Error", MB_OK | MB_ICONERROR);
    }

    void Window::create()
    {
        handle = CreateWindowExA(fullscreen == true ? 0 : 0,
                                 class_name.c_str(),
                                 name.c_str(),
                                 fullscreen ? WS_POPUPWINDOW : WS_OVERLAPPEDWINDOW,
                                 0,
                                 0,
                                 width,
                                 height,
                                 nullptr,
                                 nullptr,
                                 GetModuleHandleA(nullptr),
                                 nullptr);

        if(!handle)
            MessageBoxA(nullptr, "Error creating window", "Error", MB_OK | MB_ICONERROR);

        ShowCursor(false);
        ShowWindow(handle, SW_SHOW);
        UpdateWindow(handle);
        InvalidateRect(handle, nullptr, false);
    }

    bool Window::update()
    {
        static MSG msg{ 0 };

        if(PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if(msg.message == WM_QUIT)
                return false;

            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        return true;
    }

    LRESULT __stdcall Window::process(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        PAINTSTRUCT ps{ 0 };
        HDC hdc{ 0 };

        switch(message)
        {
            case WM_PAINT:
                hdc = BeginPaint(hWnd, &ps);
                EndPaint(hWnd, &ps);
                break;

            case WM_DESTROY:
                PostQuitMessage(0);
                break;

            default:
                return DefWindowProcA(hWnd, message, wParam, lParam);
        }

        return 0;
    }
}
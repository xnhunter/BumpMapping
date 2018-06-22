// Copyright ⓒ 2018 Valentyn Bondarenko. All rights reserved.

#pragma once

namespace bm
{
    class Window
    {
    public:
        Window(const int& width, const int& height, const bool& fullscreen);
       ~Window() = default;

        Window(const Window&) = delete;
        Window(Window&&) = delete;

        Window operator=(const Window&) = delete;
        Window operator=(Window&&) = delete;

    public:
        void registerClass();
        void create();
        bool update();

        HWND getHandle() const { return handle; }

    public:
        static LRESULT __stdcall process(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    private:
        WNDCLASSEX class_ex;
        HWND handle;

        std::string name;
        std::string class_name;

        int width;
        int height;

		bool fullscreen;
    };
}
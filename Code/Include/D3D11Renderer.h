// Copyright ⓒ 2018 Valentyn Bondarenko. All rights reserved.

#pragma once

#include <dxgi.h>
#include <d3dcommon.h>
#include <d3d11.h>

namespace bm
{
    class D3D11Renderer
    {
    public:
        D3D11Renderer(const int& width, const int& height, const bool& fullscreen, HWND hwnd, const bool& vsync);
        ~D3D11Renderer();

        D3D11Renderer(const D3D11Renderer&) = delete;
        D3D11Renderer(D3D11Renderer&&) = delete;

        D3D11Renderer& operator=(const D3D11Renderer&) = delete;
        D3D11Renderer& operator=(D3D11Renderer&&) = delete;

    public:
        void clearScreen(const float color[4]);
        void swapBuffers();

    public:
        ID3D11Device* getDevice() { return device; }
        ID3D11DeviceContext* getDeviceContext() { return device_context; }

    private:
        bool vsync;

        IDXGISwapChain* swap_chain;
        ID3D11Device* device;
        ID3D11DeviceContext* device_context;
        ID3D11RenderTargetView* render_target_view;
        ID3D11Texture2D* depth_stencil_buffer;
        ID3D11DepthStencilState* depth_stencil_state;
        ID3D11DepthStencilView* depth_stencil_view;
        ID3D11RasterizerState* rasterizer_state;
        ID3D11DepthStencilState* disabled_depth_stencil_state;
        ID3D11BlendState* enabled_depth_blending_state;
        ID3D11BlendState* disabled_alpha_blending_state;
        D3D11_VIEWPORT m_viewport;
    };
}
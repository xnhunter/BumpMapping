// Copyright ⓒ 2018 Valentyn Bondarenko. All rights reserved.

#include <StdAfx.h>

#include "D3D11Renderer.h"

namespace bm
{
    D3D11Renderer::D3D11Renderer(const int& width, const int& height, const bool& fullscreen, HWND hwnd, const bool& vsync) :
		swap_chain(nullptr),
        device(nullptr),
        device_context(nullptr),
        render_target_view(nullptr),
        depth_stencil_buffer(nullptr),
        depth_stencil_state(nullptr),
        depth_stencil_view(nullptr),
        rasterizer_state(nullptr),
        disabled_depth_stencil_state(nullptr),
        enabled_depth_blending_state(nullptr),
        disabled_alpha_blending_state(nullptr),
        vsync(vsync)
    {
        IDXGIFactory* factory = nullptr;
        auto result = CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&factory));
        if(FAILED(result))
            return;

        IDXGIAdapter* adapter = nullptr;
        result = factory->EnumAdapters(0, &adapter);
        if(FAILED(result))
            return;

        IDXGIOutput* output;
        result = adapter->EnumOutputs(0, &output);
        if(FAILED(result))
            return;

        // Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display format for the adapter output (monitor).
        auto mode_count = UINT();
        result = output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &mode_count, nullptr);
        if(FAILED(result))
            return;

        // Create a list to hold all the possible display modes for this monitor/video card combination.
        DXGI_MODE_DESC* display_mode_list = nullptr;
        display_mode_list = new DXGI_MODE_DESC[mode_count];
        if(!display_mode_list)
            return;

        // Now fill the display mode list structures.
        result = output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &mode_count, display_mode_list);
        if(FAILED(result))
            return;

        // Now go through all the display modes and find the one that matches the screen width and height.
        // When a match is found store the numerator and denominator of the refresh rate for that monitor.
        auto numerator = 0,
             denominator = 0;

        for(auto i = decltype(mode_count)(); i < mode_count; i++)
        {
            if(display_mode_list[i].Width == static_cast<unsigned int>(width))
            {
                if(display_mode_list[i].Height == static_cast<unsigned int>(height))
                {
                    numerator = display_mode_list[i].RefreshRate.Numerator;
                    denominator = display_mode_list[i].RefreshRate.Denominator;
                }
            }
        }

        delete[] display_mode_list;

        output->Release();
        adapter->Release();
        factory->Release();

        DXGI_SWAP_CHAIN_DESC swap_chain_desc;
        memset(&swap_chain_desc, 0, sizeof(swap_chain_desc));
        swap_chain_desc.BufferCount = 1;

        swap_chain_desc.BufferDesc.Width = width;
        swap_chain_desc.BufferDesc.Height = height;

        swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

        if(vsync)
        {
            swap_chain_desc.BufferDesc.RefreshRate.Numerator = numerator;
            swap_chain_desc.BufferDesc.RefreshRate.Denominator = denominator;
        }
        else
        {
            swap_chain_desc.BufferDesc.RefreshRate.Numerator = 0;
            swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
        }

        swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_chain_desc.OutputWindow = hwnd;

        swap_chain_desc.Windowed = fullscreen ? false : true;

        swap_chain_desc.SampleDesc.Count = 1;
        swap_chain_desc.SampleDesc.Quality = 0;

        swap_chain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        swap_chain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

        swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        swap_chain_desc.Flags = 0;

        auto featureLevel = D3D_FEATURE_LEVEL_11_0;

        D3D_FEATURE_LEVEL feature_levels[] =
        {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
        };

        UINT feature_levels_count = ARRAYSIZE(feature_levels);
        UINT create_device_flags = 0U;

#ifdef _DEBUG
        create_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        result = D3D11CreateDeviceAndSwapChain(nullptr,
                                               D3D_DRIVER_TYPE_HARDWARE,
                                               nullptr,
                                               create_device_flags,
                                               feature_levels,
                                               feature_levels_count,
                                               D3D11_SDK_VERSION,
                                               &swap_chain_desc,
                                               &swap_chain,
                                               &device,
                                               &featureLevel,
                                               &device_context);
        if(FAILED(result))
        {
            MessageBoxA(nullptr, "Can't initialize D3D11 Renderer", "Renderer Error", MB_ICONERROR);
            return;
        }

        ID3D11Texture2D* back_buffer = nullptr;
        result = swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back_buffer));
        if(FAILED(result))
            return;

        result = device->CreateRenderTargetView(back_buffer, nullptr, &render_target_view);
        if(FAILED(result))
            return;

        back_buffer->Release();

        D3D11_TEXTURE2D_DESC depth_buffer_desc;
        memset(&depth_buffer_desc, 0, sizeof(depth_buffer_desc));
        depth_buffer_desc.Width = width;
        depth_buffer_desc.Height = height;
        depth_buffer_desc.MipLevels = 1U;
        depth_buffer_desc.ArraySize = 1U;
        depth_buffer_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depth_buffer_desc.SampleDesc.Count = 1;
        depth_buffer_desc.SampleDesc.Quality = 0;
        depth_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
        depth_buffer_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        depth_buffer_desc.CPUAccessFlags = 0U;
        depth_buffer_desc.MiscFlags = 0U;

        result = device->CreateTexture2D(&depth_buffer_desc, nullptr, &depth_stencil_buffer);
        if(FAILED(result))
            return;

        D3D11_DEPTH_STENCIL_DESC depth_stencil_desc;
        memset(&depth_stencil_desc, 0, sizeof(depth_stencil_desc));

        depth_stencil_desc.DepthEnable = true;
        depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depth_stencil_desc.DepthFunc = D3D11_COMPARISON_LESS;

        depth_stencil_desc.StencilEnable = true;
        depth_stencil_desc.StencilReadMask = 0xFF;
        depth_stencil_desc.StencilWriteMask = 0xFF;

        depth_stencil_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        depth_stencil_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
        depth_stencil_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        depth_stencil_desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

        depth_stencil_desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        depth_stencil_desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
        depth_stencil_desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        depth_stencil_desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

        result = device->CreateDepthStencilState(&depth_stencil_desc, &depth_stencil_state);
        if(FAILED(result))
            return;

        device_context->OMSetDepthStencilState(depth_stencil_state, 1);

        D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc;
        memset(&depth_stencil_view_desc, 0, sizeof(depth_stencil_view_desc));
        depth_stencil_view_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        depth_stencil_view_desc.Texture2D.MipSlice = 0;

        result = device->CreateDepthStencilView(depth_stencil_buffer, &depth_stencil_view_desc, &depth_stencil_view);
        if(FAILED(result))
            return;

        device_context->OMSetRenderTargets(1, &render_target_view, depth_stencil_view);

        D3D11_RASTERIZER_DESC rasterizer_desc;
        rasterizer_desc.AntialiasedLineEnable = false;
        rasterizer_desc.CullMode = D3D11_CULL_BACK;
        rasterizer_desc.DepthBias = 0;
        rasterizer_desc.DepthBiasClamp = 0.0f;
        rasterizer_desc.DepthClipEnable = true;
        rasterizer_desc.FillMode = D3D11_FILL_SOLID;
        rasterizer_desc.FrontCounterClockwise = false;
        rasterizer_desc.MultisampleEnable = false;
        rasterizer_desc.ScissorEnable = false;
        rasterizer_desc.SlopeScaledDepthBias = 0.0f;

        result = device->CreateRasterizerState(&rasterizer_desc, &rasterizer_state);
        if(FAILED(result))
            return;

        device_context->RSSetState(rasterizer_state);

        m_viewport.Width = static_cast<decltype(m_viewport.Width)>(width);
        m_viewport.Height = static_cast<decltype(m_viewport.Height)>(height);
        m_viewport.MinDepth = 0.0f;
        m_viewport.MaxDepth = 1.0f;
        m_viewport.TopLeftX = 0.0f;
        m_viewport.TopLeftY = 0.0f;

        device_context->RSSetViewports(1, &m_viewport);

        D3D11_DEPTH_STENCIL_DESC disabled_depth_stencil_desc;
        memset(&disabled_depth_stencil_desc, 0, sizeof(disabled_depth_stencil_desc));

        disabled_depth_stencil_desc.DepthEnable = false;
        disabled_depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        disabled_depth_stencil_desc.DepthFunc = D3D11_COMPARISON_LESS;
        disabled_depth_stencil_desc.StencilEnable = true;
        disabled_depth_stencil_desc.StencilReadMask = 0xFF;
        disabled_depth_stencil_desc.StencilWriteMask = 0xFF;
        disabled_depth_stencil_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        disabled_depth_stencil_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
        disabled_depth_stencil_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        disabled_depth_stencil_desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        disabled_depth_stencil_desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        disabled_depth_stencil_desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
        disabled_depth_stencil_desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        disabled_depth_stencil_desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

        result = device->CreateDepthStencilState(&disabled_depth_stencil_desc, &disabled_depth_stencil_state);
        if(FAILED(result))
            return;

        D3D11_BLEND_DESC blend_state_desc;
        memset(&blend_state_desc, 0, sizeof(D3D11_BLEND_DESC));
        blend_state_desc.RenderTarget[0].BlendEnable = TRUE;
        blend_state_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
        blend_state_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blend_state_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blend_state_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blend_state_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        blend_state_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blend_state_desc.RenderTarget[0].RenderTargetWriteMask = 0x0f;

        result = device->CreateBlendState(&blend_state_desc, &enabled_depth_blending_state);
        if(FAILED(result))
            return;

        blend_state_desc.RenderTarget[0].BlendEnable = FALSE;

        result = device->CreateBlendState(&blend_state_desc, &disabled_alpha_blending_state);
        if(FAILED(result))
            return;
    }

    D3D11Renderer::~D3D11Renderer()
    {
        if(swap_chain)
            swap_chain->SetFullscreenState(false, nullptr);

        if(enabled_depth_blending_state)
            enabled_depth_blending_state->Release();

        if(disabled_alpha_blending_state)
            disabled_alpha_blending_state->Release();

        if(disabled_depth_stencil_state)
            disabled_depth_stencil_state->Release();

        if(rasterizer_state)
            rasterizer_state->Release();

        if(depth_stencil_view)
            depth_stencil_view->Release();

        if(depth_stencil_state)
            depth_stencil_state->Release();

        if(depth_stencil_buffer)
            depth_stencil_buffer->Release();

        if(render_target_view)
            render_target_view->Release();

        if(swap_chain)
            swap_chain->Release();

        if(device_context)
            device_context->Release();

        if(device)
            device->Release();
    }

    void D3D11Renderer::clearScreen(const float color[4])
    {
        device_context->ClearRenderTargetView(render_target_view, color);
        device_context->ClearDepthStencilView(depth_stencil_view, D3D11_CLEAR_DEPTH, 1.0f, 0U);
    }

    void D3D11Renderer::swapBuffers()
    {
		swap_chain->Present(vsync ? 1U : 0U, 0U);
    }
}
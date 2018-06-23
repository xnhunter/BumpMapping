// Copyright ⓒ 2018 Valentyn Bondarenko. All rights reserved.

#include <StdAfx.h>

#include <Window.h>
#include <FPSCamera.h>

#include <DirectInput8.h>
#include <D3D11Renderer.h>

#include <Terrain.h>
#include <TerrainShader.h>

using namespace bm;

int __stdcall WinMain(HINSTANCE, HINSTANCE, char*, int)
{
    auto resource_directory_name = L"..\\..\\..\\Resource\\"s;
    auto terrain_name = L"terrain"s;

    auto dds_file_extension = L".dds"s;
    auto hlsl_file_extension = L".hlsl"s;

    std::wstring resources[] = {resource_directory_name + L"heightmap.bmp"s,
                                resource_directory_name + terrain_name + dds_file_extension,
                                resource_directory_name + terrain_name + L"_bump"s + dds_file_extension,
                                resource_directory_name + terrain_name + L"_vs"s + hlsl_file_extension,
                                resource_directory_name + terrain_name + L"_ps"s + hlsl_file_extension};

    constexpr auto ENABLE_FULLSCREEN = true;
    constexpr auto ENABLE_VSYNC = false;
    
    constexpr auto SCREEN_WIDTH = 1920;
    constexpr auto SCREEN_HEIGHT = 1080;

    auto window = std::make_shared<bm::Window>(SCREEN_WIDTH, SCREEN_HEIGHT, ENABLE_FULLSCREEN);
    window->registerClass();
    window->create();

    auto d3d11_renderer = std::make_shared<bm::D3D11Renderer>(SCREEN_WIDTH, SCREEN_HEIGHT, ENABLE_FULLSCREEN, window->getHandle(), ENABLE_VSYNC);

    auto terrain = std::make_shared<bm::Terrain>(d3d11_renderer->getDevice(), resources[0].c_str(), resources[1].c_str(), resources[2].c_str());
    auto terrain_shader = std::make_shared<bm::TerrainShader>(d3d11_renderer->getDevice(), resources[3].c_str(), resources[4].c_str());
   
    auto fps_camera = std::make_shared<bm::FPSCamera>(static_cast<float>(SCREEN_WIDTH),  static_cast<float>(SCREEN_HEIGHT));
	fps_camera->setPosition(500.f, 75.f, 400.f);
    fps_camera->setRotation(20.f, 30.f, 0.f); // in degree.

    auto direct_input_8 = std::make_shared<bm::DirectInput8>(window->getHandle());

    constexpr float CLEAR_COLOR[] = {0.84f, 0.84f, 1.f, 1.f};

    while(window->update())
    {
        direct_input_8->update(fps_camera->getMoveLeftRight(), fps_camera->getMoveBackForward(), fps_camera->getYaw(), fps_camera->getPitch());
        fps_camera->update();

        d3d11_renderer->clearScreen(CLEAR_COLOR);

        terrain->render(d3d11_renderer->getDeviceContext());

#pragma warning(push)
#pragma warning(disable : 4239)
        // FIXME: C4239 with Matrix getWorld(), Matrix getView() and Matrix getProjection().
        //        The arguments of the following function do not chash the app, but when
        //        we change them they do.
        terrain_shader->render(d3d11_renderer->getDeviceContext(),
                               terrain->getIndexCount(),
                               fps_camera->getWorld(),
                               fps_camera->getView(),
                               fps_camera->getProjection(),
                               {0.82f, 0.82f, 0.82f, 1.0f},
                               {-0.0f, -1.0f, 0.0f},
                               terrain->getColorTexture(),
                               terrain->getNormalMapTexture());
#pragma warning(pop)

        d3d11_renderer->swapBuffers();
    }

    return 0;
}
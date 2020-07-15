// Copyright ⓒ 2018, 2020 Valentyn Bondarenko. All rights reserved.

#pragma once

#include <d3d11.h>

#include <fstream>

namespace bm
{
    class TerrainShader
    {
    private:
        struct MatrixBufferType
        {
            Matrix world;
            Matrix view;
            Matrix projection;
        };

        struct LightBufferType
        {
            Vector4D diffuse_color;
            Vector3D light_direction;

            float padding;
        };

    public:
        TerrainShader(ID3D11Device*, const wchar_t* vs_file_name, const wchar_t* ps_file_name);
       ~TerrainShader();
        
		TerrainShader(const TerrainShader&) = delete;
		TerrainShader(TerrainShader&&) = delete;

		TerrainShader& operator=(const TerrainShader&) = delete;
		TerrainShader& operator=(TerrainShader&&) = delete;

	public:
        bool render(ID3D11DeviceContext* device_context,
                    int index_count,
                    Matrix&& world,
                    Matrix&& view,
                    Matrix&& projection,
                    Vector4D diffuse_color,
                    Vector3D light_direction,
                    ID3D11ShaderResourceView* diffuse_texture,
                    ID3D11ShaderResourceView* bump_map_texture);

    private:
        bool initializeShader(ID3D11Device* device, const wchar_t* vs_file_name, const wchar_t* ps_file_name);

        void outputShaderErrorMessage(ID3D10Blob* error_message, const wchar_t* shader_file_name);

        bool setShaderParameters(ID3D11DeviceContext* device_context,
                                 Matrix& world,
                                 Matrix& view,
                                 Matrix& projection,
                                 Vector4D diffuse_color,
                                 Vector3D light_direction,
                                 ID3D11ShaderResourceView* diffuse_texture,
                                 ID3D11ShaderResourceView* bump_map_texture);

        void renderShader(ID3D11DeviceContext* device_context, int indexCount);

    private:
        ID3D11VertexShader* vertex_shader;
        ID3D11PixelShader* pixel_shader;

        ID3D11InputLayout* layout;
        ID3D11SamplerState* sample_state;

        ID3D11Buffer* matrix_buffer;
        ID3D11Buffer* light_buffer;
    };
}
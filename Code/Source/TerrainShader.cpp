// Copyright ⓒ 2018, 2020 Valentyn Bondarenko. All rights reserved.

#include <StdAfx.h>

#include "TerrainShader.h"

namespace bm
{
    TerrainShader::TerrainShader(ID3D11Device* device, const wchar_t* vs_file_name, const wchar_t* ps_file_name) :
        vertex_shader(nullptr),
        pixel_shader(nullptr),
        layout(nullptr),
        sample_state(nullptr),
        matrix_buffer(nullptr),
        light_buffer(nullptr)
    {
        auto checkFileExisting([](const wchar_t* file_name)
        {
            if (!fs::exists(file_name))
            {
                std::wstring vs_file_name(file_name);
                std::string vs(vs_file_name.begin(), vs_file_name.end());

                throw std::runtime_error("Can't find file: " + vs);
            }
        });

        checkFileExisting(vs_file_name);
        checkFileExisting(ps_file_name);

        auto result = initializeShader(device, vs_file_name, ps_file_name);
        if (!result)
            return;
    }

    TerrainShader::~TerrainShader()
    {
        if (light_buffer)
            light_buffer->Release();

        if (matrix_buffer)
            matrix_buffer->Release();

        if (sample_state)
            sample_state->Release();

        if (layout)
            layout->Release();

        if (pixel_shader)
            pixel_shader->Release();

        if (vertex_shader)
            vertex_shader->Release();
    }

    bool TerrainShader::render(ID3D11DeviceContext* device_context,
                               int index_count,
                               Matrix&& world,
                               Matrix&& view,
                               Matrix&& projection,
                               Vector4D diffuse_color,
                               Vector3D light_direction,
                               ID3D11ShaderResourceView* diffuse_texture,
                               ID3D11ShaderResourceView* bump_map_texture)
    {
        auto result = setShaderParameters(device_context, world, view, projection, diffuse_color, light_direction, diffuse_texture, bump_map_texture);
        if (!result)
            return false;

        renderShader(device_context, index_count);

        return true;
    }


    bool TerrainShader::initializeShader(ID3D11Device* device, const wchar_t* vs_file_name, const wchar_t* ps_file_name)
    {
        ID3D10Blob* error_message = nullptr;
        ID3D10Blob* vertexShaderBuffer = nullptr;

        UINT shader_compile_flags = D3D10_SHADER_ENABLE_STRICTNESS;

#ifdef _DEBUG
        shader_compile_flags |= D3D10_SHADER_DEBUG;
#endif

        auto result = D3DCompileFromFile(vs_file_name,
                                         nullptr,
                                         D3D_COMPILE_STANDARD_FILE_INCLUDE,
                                         "TerrainVertexShader",
                                         "vs_4_0",
                                         shader_compile_flags,
                                         0U,
                                         &vertexShaderBuffer,
                                         &error_message);

#ifdef DEPRECATED_CODE
        {
            result = D3DX11CompileFromFileW(vsFilename,
                                            nullptr,
                                            nullptr,
                                            "TerrainVertexShader",
                                            "vs_4_0",
                                            D3D10_SHADER_ENABLE_STRICTNESS,
                                            0,
                                            nullptr,
                                            &vertexShaderBuffer,
                                            &error_message,
                                            nullptr);
        }
#endif
        if (FAILED(result))
        {
            if (error_message)
                outputShaderErrorMessage(error_message, vs_file_name);

            MessageBoxW(nullptr, vs_file_name, L"Can't compile a shader.", MB_ICONERROR);

            return false;
        }


        ID3D10Blob* pixelShaderBuffer = nullptr;
        result = D3DCompileFromFile(ps_file_name,
                                    nullptr,
                                    D3D_COMPILE_STANDARD_FILE_INCLUDE,
                                    "TerrainPixelShader",
                                    "ps_4_0",
                                    shader_compile_flags,
                                    0U,
                                    &pixelShaderBuffer,
                                    &error_message);

#ifdef DEPRECATED_CODE
        {
            result = D3DX11CompileFromFileW(psFilename,
                                            nullptr,
                                            nullptr,
                                            "TerrainPixelShader",
                                            "ps_4_0",
                                            D3D10_SHADER_ENABLE_STRICTNESS,
                                            0,
                                            nullptr,
                                            &pixelShaderBuffer,
                                            &error_message, nullptr);
        }
#endif

        if (FAILED(result))
        {
            if (error_message)
                outputShaderErrorMessage(error_message, ps_file_name);

            MessageBoxW(nullptr, ps_file_name, L"Can't compile a shader.", MB_ICONERROR);

            return false;
        }

        result = device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), nullptr, &vertex_shader);
        if (FAILED(result))
            return false;

        result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), nullptr, &pixel_shader);
        if (FAILED(result))
            return false;

        D3D11_INPUT_ELEMENT_DESC polygonLayout[5];
        polygonLayout[0].SemanticName = "POSITION";
        polygonLayout[0].SemanticIndex = 0U;
        polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
        polygonLayout[0].InputSlot = 0U;
        polygonLayout[0].AlignedByteOffset = 0U;
        polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        polygonLayout[0].InstanceDataStepRate = 0U;

        polygonLayout[1].SemanticName = "TEXCOORD";
        polygonLayout[1].SemanticIndex = 0U;
        polygonLayout[1].Format = DXGI_FORMAT_R32G32_FLOAT;
        polygonLayout[1].InputSlot = 0U;
        polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
        polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        polygonLayout[1].InstanceDataStepRate = 0U;

        polygonLayout[2].SemanticName = "NORMAL";
        polygonLayout[2].SemanticIndex = 0U;
        polygonLayout[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
        polygonLayout[2].InputSlot = 0U;
        polygonLayout[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
        polygonLayout[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        polygonLayout[2].InstanceDataStepRate = 0U;

        polygonLayout[3].SemanticName = "TANGENT";
        polygonLayout[3].SemanticIndex = 0U;
        polygonLayout[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
        polygonLayout[3].InputSlot = 0U;
        polygonLayout[3].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
        polygonLayout[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        polygonLayout[3].InstanceDataStepRate = 0U;

        polygonLayout[4].SemanticName = "BINORMAL";
        polygonLayout[4].SemanticIndex = 0;
        polygonLayout[4].Format = DXGI_FORMAT_R32G32B32_FLOAT;
        polygonLayout[4].InputSlot = 0;
        polygonLayout[4].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
        polygonLayout[4].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        polygonLayout[4].InstanceDataStepRate = 0U;

        UINT numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);

        result = device->CreateInputLayout(polygonLayout, numElements, vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(),
                                           &layout);
        if (FAILED(result))
            return false;

        vertexShaderBuffer->Release();

        pixelShaderBuffer->Release();

        D3D11_SAMPLER_DESC sampler_desc;
        sampler_desc.Filter = D3D11_FILTER_ANISOTROPIC;
        sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.MipLODBias = 0.0f;
        sampler_desc.MaxAnisotropy = 16U;
        sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        sampler_desc.BorderColor[0] = 0.f;
        sampler_desc.BorderColor[1] = 0.f;
        sampler_desc.BorderColor[2] = 0.f;
        sampler_desc.BorderColor[3] = 0.f;
        sampler_desc.MinLOD = 0.f;
        sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;

        result = device->CreateSamplerState(&sampler_desc, &sample_state);
        if (FAILED(result))
            return false;

        D3D11_BUFFER_DESC matrix_buffer_desc;
        matrix_buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
        matrix_buffer_desc.ByteWidth = sizeof(MatrixBufferType);
        matrix_buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        matrix_buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        matrix_buffer_desc.MiscFlags = 0U;
        matrix_buffer_desc.StructureByteStride = 0U;

        result = device->CreateBuffer(&matrix_buffer_desc, nullptr, &matrix_buffer);
        if (FAILED(result))
            return false;

        D3D11_BUFFER_DESC lightBufferDesc;
        lightBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        lightBufferDesc.ByteWidth = sizeof(LightBufferType);
        lightBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        lightBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        lightBufferDesc.MiscFlags = 0U;
        lightBufferDesc.StructureByteStride = 0U;

        result = device->CreateBuffer(&lightBufferDesc, nullptr, &light_buffer);
        if (FAILED(result))
            return false;

        return true;
    }

    void TerrainShader::outputShaderErrorMessage(ID3D10Blob* error_message, const wchar_t* shader_file_name)
    {
        auto compileErrors = reinterpret_cast<char*>(error_message->GetBufferPointer());
        auto bufferSize = error_message->GetBufferSize();

        std::ofstream fout("shaders.log");

        for (auto i = decltype(bufferSize)(); i < bufferSize; i++)
            fout << compileErrors[i];

        error_message->Release();

        MessageBoxW(nullptr, L"Error while compiling shader. Check shaders.log out for a message.", shader_file_name, MB_ICONERROR);
    }

    bool TerrainShader::setShaderParameters(ID3D11DeviceContext* device_context,
                                            Matrix& world,
                                            Matrix& view,
                                            Matrix& projection,
                                            Vector4D diffuse_color,
                                            Vector3D light_direction,
                                            ID3D11ShaderResourceView* diffuse_texture,
                                            ID3D11ShaderResourceView* bump_map_texture)
    {
        world = DirectX::XMMatrixTranspose(world);
        view = DirectX::XMMatrixTranspose(view);
        projection = DirectX::XMMatrixTranspose(projection);

        D3D11_MAPPED_SUBRESOURCE mapped_subresource;
        auto result = device_context->Map(matrix_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
        if (FAILED(result))
            return false;

        auto data = reinterpret_cast<MatrixBufferType*>(mapped_subresource.pData);

        data->world = world;
        data->view = view;
        data->projection = projection;
        device_context->Unmap(matrix_buffer, 0U);

        auto buffer_number = 0U;
        device_context->VSSetConstantBuffers(buffer_number, 1, &matrix_buffer);

        result = device_context->Map(light_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
        if (FAILED(result))
            return false;

        auto shader_configs = reinterpret_cast<LightBufferType*>(mapped_subresource.pData);

        shader_configs->diffuse_color = diffuse_color;
        shader_configs->light_direction = light_direction;
        shader_configs->padding = 0.0f;
        device_context->Unmap(light_buffer, 0U);

        buffer_number = 0U;
        device_context->PSSetConstantBuffers(buffer_number, 1, &light_buffer);

        device_context->PSSetShaderResources(0U, 1U, &diffuse_texture);
        device_context->PSSetShaderResources(1U, 1U, &bump_map_texture);

        return true;
    }


    void TerrainShader::renderShader(ID3D11DeviceContext* device_context, int index_count)
    {
        device_context->IASetInputLayout(layout);

        device_context->VSSetShader(vertex_shader, nullptr, 0U);
        device_context->PSSetShader(pixel_shader, nullptr, 0U);

        device_context->PSSetSamplers(0U, 1U, &sample_state);

        device_context->DrawIndexed(index_count, 0U, 0);
    }
}

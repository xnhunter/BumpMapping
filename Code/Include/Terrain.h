// Copyright ⓒ 2018 Valentyn Bondarenko. All rights reserved.

#pragma once

#include <d3d11.h>

#include <stdio.h>

namespace bm
{
    class Terrain
    {
    private:
        struct HeightMapType
        {
            float x, y, z;
            float nx, ny, nz;
        };

        struct VectorType
        {
            float x, y, z;
        };

        struct ModelType
        {
            float x, y, z;
            float tu, tv;
            float nx, ny, nz;
            float tx, ty, tz;
            float bx, by, bz;
        };

        struct TempVertexType
        {
            float x, y, z;
            float tu, tv;
            float nx, ny, nz;
        };

        struct VertexType
        {
            Vector3D position;
            Vector2D texture;
            Vector3D normal;
            Vector3D tangent;
            Vector3D binormal;
        };

    public:
        Terrain(ID3D11Device*, const wchar_t* height_map_file_name, const wchar_t* diffuse_map_file_name, const wchar_t* bump_map_file_name);
       ~Terrain();

        Terrain(const Terrain&) = delete;
		Terrain(Terrain&&) = delete;

		Terrain& operator=(const Terrain&) = delete;
		Terrain& operator=(Terrain&&) = delete;

	public:
        void render(ID3D11DeviceContext* device_context);

        int getIndexCount();
        ID3D11ShaderResourceView* getColorTexture();
        ID3D11ShaderResourceView* getNormalMapTexture();

    private:
        // For constructor, to make it easier for understanding.
        bool loadHeightMap(const wchar_t* file_name);
        void reduceHeightMap();
        bool calculateNormals();

        bool buildTerrainModel();

        void calculateTerrainVectors();
        void calculateTangentBinormal(TempVertexType vertex1, TempVertexType vertex2, TempVertexType vertex3, VectorType& tangent, VectorType& binormal);

        bool initializeBuffers(ID3D11Device* device);

        bool loadTextures(ID3D11Device* device, const wchar_t* diffuse_texture_file_name, const wchar_t* bump_map_file_name);

    private:
        int terrain_width, terrain_height;

        HeightMapType* height_map;
        ModelType* terrain_model;

        int vertex_count, index_count;

        ID3D11Buffer *vertex_buffer, *index_buffer;
        ID3D11ShaderResourceView* diffuse_texture, *bump_texture;
    };
}
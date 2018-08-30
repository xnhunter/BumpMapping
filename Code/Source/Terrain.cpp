// Copyright ⓒ 2018 Valentyn Bondarenko. All rights reserved.

#include <StdAfx.h>

#include "Terrain.h"

namespace bm
{
	Terrain::Terrain(ID3D11Device* device, const wchar_t* height_map_file_name, const wchar_t* diffuse_texture_file_name, const wchar_t* bump_map_file_name) :
        height_map(nullptr),
		terrain_model(nullptr),
		vertex_buffer(nullptr),
		index_buffer(nullptr),
		diffuse_texture(nullptr),
		bump_texture(nullptr)
	{
        auto result = loadHeightMap(height_map_file_name);
        if(!result)
            return;

        reduceHeightMap();

        result = calculateNormals();
        if(!result)
            return;

        result = buildTerrainModel();
        if(!result)
            return;

        calculateTerrainVectors();

        result = initializeBuffers(device);
        if(!result)
            return;

        result = loadTextures(device, diffuse_texture_file_name, bump_map_file_name);
        if(!result)
            return;
	}

    Terrain::~Terrain()
    {
        if(bump_texture)
            bump_texture->Release();

        if(diffuse_texture)
            diffuse_texture->Release();

        if(index_buffer)
            index_buffer->Release();

        if(vertex_buffer)
            vertex_buffer->Release();

        if(terrain_model)
            delete[] terrain_model;

        if(height_map)
            delete[] height_map;
    }

	void Terrain::render(ID3D11DeviceContext* device_context)
	{
        UINT stride = sizeof(VertexType);
        UINT offset = 0U;

        device_context->IASetVertexBuffers(0U, 1U, &vertex_buffer, &stride, &offset);
        device_context->IASetIndexBuffer(index_buffer, DXGI_FORMAT_R32_UINT, 0U);
        device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}


	int Terrain::getIndexCount()
	{
		return index_count;
	}


	ID3D11ShaderResourceView* Terrain::getColorTexture()
	{
		return diffuse_texture;
	}


	ID3D11ShaderResourceView* Terrain::getNormalMapTexture()
	{
		return bump_texture;
	}


	bool Terrain::loadHeightMap(const wchar_t* file_name)
	{
		FILE* filePtr = nullptr;
		auto error = _wfopen_s(&filePtr, file_name, L"rb");
		if(error != 0)
			return false;

		BITMAPFILEHEADER bitmap_file_header;
		auto count = fread(&bitmap_file_header, sizeof(BITMAPFILEHEADER), 1, filePtr);
		if(count != 1)
			return false;

		BITMAPINFOHEADER bitmap_info_header;
		count = fread(&bitmap_info_header, sizeof(BITMAPINFOHEADER), 1, filePtr);
		if(count != 1)
			return false;

		terrain_width = bitmap_info_header.biWidth;
		terrain_height = bitmap_info_header.biHeight;

		auto imageSize = terrain_width * terrain_height * 3;

		auto bitmap_image = new unsigned char[imageSize];
		if(!bitmap_image)
			return false;

		fseek(filePtr, bitmap_file_header.bfOffBits, SEEK_SET);

		count = fread(bitmap_image, 1, imageSize, filePtr);
		if(static_cast<decltype(imageSize)>(count) != imageSize)
			return false;

		error = fclose(filePtr);
		if(error != 0)
			return false;

		height_map = new HeightMapType[terrain_width * terrain_height];
		if(!height_map)
			return false;

		// Initialize the position in the image data buffer.
		// Read the image data into the height map.
		for(auto j = int(), k = int(); j < terrain_height; j++)
		{
			for(auto i = int(); i < terrain_width; i++)
			{
				auto height = bitmap_image[k];

				auto index = (terrain_width * j) + i;

				height_map[index].x = (float)i * 32;
				height_map[index].y = (float)height * 8;
				height_map[index].z = (float)j * 32;

				k += 3;
			}
		}

		delete[] bitmap_image;

		return true;
	}

	void Terrain::reduceHeightMap()
	{
		for(auto j = int(); j < terrain_height; j++)
		{
			for(auto i = int(); i < terrain_width; i++)
				height_map[(terrain_width * j) + i].y /= 15.0f;
		}
	}


	bool Terrain::calculateNormals()
	{
		int index1, index2, index3, index, count;
		float vertex1[3], vertex2[3], vertex3[3], vector1[3], vector2[3], sum[3], length;

		// Create a temporary array to hold the un-normalized normal vectors.
		auto normals = new VectorType[(terrain_height - 1) * (terrain_width - 1)];
		if(!normals)
			return false;

		// Go through all the faces in the mesh and calculate their normals.
		for(auto j = int(); j < (terrain_height - 1); j++)
		{
			for(auto i = int(); i < (terrain_width - 1); i++)
			{
				index1 = (j * terrain_width) + i;
				index2 = (j * terrain_width) + (i + 1);
				index3 = ((j + 1) * terrain_width) + i;

				// Get three vertices from the face.
				vertex1[0] = height_map[index1].x;
				vertex1[1] = height_map[index1].y;
				vertex1[2] = height_map[index1].z;

				vertex2[0] = height_map[index2].x;
				vertex2[1] = height_map[index2].y;
				vertex2[2] = height_map[index2].z;

				vertex3[0] = height_map[index3].x;
				vertex3[1] = height_map[index3].y;
				vertex3[2] = height_map[index3].z;

				// Calculate the two vectors for this face.
				vector1[0] = vertex1[0] - vertex3[0];
				vector1[1] = vertex1[1] - vertex3[1];
				vector1[2] = vertex1[2] - vertex3[2];
				vector2[0] = vertex3[0] - vertex2[0];
				vector2[1] = vertex3[1] - vertex2[1];
				vector2[2] = vertex3[2] - vertex2[2];

				index = (j * (terrain_width - 1)) + i;

				// Calculate the cross product of those two vectors to get the un-normalized value for this face normal.
				normals[index].x = (vector1[1] * vector2[2]) - (vector1[2] * vector2[1]);
				normals[index].y = (vector1[2] * vector2[0]) - (vector1[0] * vector2[2]);
				normals[index].z = (vector1[0] * vector2[1]) - (vector1[1] * vector2[0]);
			}
		}

		// Now go through all the vertices and take an average of each face normal that the vertex touches to get the averaged normal for that vertex.
		for(auto j = int(); j < terrain_height; j++)
		{
			for(auto i = int(); i < terrain_width; i++)
			{
				// Initialize the sum.
				sum[0] = 0.0f;
				sum[1] = 0.0f;
				sum[2] = 0.0f;

				// Initialize the count.
				count = 0;

				// Bottom left face.
				if(((i - 1) >= 0) && ((j - 1) >= 0))
				{
					index = ((j - 1) * (terrain_width - 1)) + (i - 1);

					sum[0] += normals[index].x;
					sum[1] += normals[index].y;
					sum[2] += normals[index].z;
					count++;
				}

				// Bottom right face.
				if((i < (terrain_width - 1)) && ((j - 1) >= 0))
				{
					index = ((j - 1) * (terrain_height - 1)) + i;

					sum[0] += normals[index].x;
					sum[1] += normals[index].y;
					sum[2] += normals[index].z;
					count++;
				}

				// Upper left face.
				if(((i - 1) >= 0) && (j < (terrain_height - 1)))
				{
					index = (j * (terrain_height - 1)) + (i - 1);

					sum[0] += normals[index].x;
					sum[1] += normals[index].y;
					sum[2] += normals[index].z;
					count++;
				}

				// Upper right face.
				if((i < (terrain_width - 1)) && (j < (terrain_height - 1)))
				{
					index = (j * (terrain_height - 1)) + i;

					sum[0] += normals[index].x;
					sum[1] += normals[index].y;
					sum[2] += normals[index].z;
					count++;
				}

				// Take the average of the faces touching this vertex.
				sum[0] = (sum[0] / (float)count);
				sum[1] = (sum[1] / (float)count);
				sum[2] = (sum[2] / (float)count);

				// Calculate the length of this normal.
				length = sqrt((sum[0] * sum[0]) + (sum[1] * sum[1]) + (sum[2] * sum[2]));

				// Get an index to the vertex location in the height map array.
				index = (j * terrain_height) + i;

				// Normalize the final shared normal for this vertex and store it in the height map array.
				height_map[index].nx = (sum[0] / length);
				height_map[index].ny = (sum[1] / length);
				height_map[index].nz = (sum[2] / length);
			}
		}

		// Release the temporary normals.
		delete[] normals;

		return true;
	}


	bool Terrain::buildTerrainModel()
	{
		vertex_count = (terrain_width - 1) * (terrain_height - 1) * 6;

		terrain_model = new ModelType[vertex_count];
		if(!terrain_model)
			return false;

        auto index = int();
		int index1, index2, index3, index4;
		for(auto j = int(); j < (terrain_height - 1); j++)
		{
			for(auto i = int(); i<(terrain_width - 1); i++)
			{
				index1 = (terrain_width * j) + i;          // Bottom left.
				index2 = (terrain_width * j) + (i + 1);      // Bottom right.
				index3 = (terrain_width * (j + 1)) + i;      // Upper left.
				index4 = (terrain_width * (j + 1)) + (i + 1);  // Upper right.

				// Upper left.
				terrain_model[index].x = height_map[index3].x;
				terrain_model[index].y = height_map[index3].y;
				terrain_model[index].z = height_map[index3].z;
				terrain_model[index].nx = height_map[index3].nx;
				terrain_model[index].ny = height_map[index3].ny;
				terrain_model[index].nz = height_map[index3].nz;
				terrain_model[index].tu = 0.0f;
				terrain_model[index].tv = 0.0f;
				index++;

				// Upper right.
				terrain_model[index].x = height_map[index4].x;
				terrain_model[index].y = height_map[index4].y;
				terrain_model[index].z = height_map[index4].z;
				terrain_model[index].nx = height_map[index4].nx;
				terrain_model[index].ny = height_map[index4].ny;
				terrain_model[index].nz = height_map[index4].nz;
				terrain_model[index].tu = 1.0f;
				terrain_model[index].tv = 0.0f;
				index++;

				// Bottom left.
				terrain_model[index].x = height_map[index1].x;
				terrain_model[index].y = height_map[index1].y;
				terrain_model[index].z = height_map[index1].z;
				terrain_model[index].nx = height_map[index1].nx;
				terrain_model[index].ny = height_map[index1].ny;
				terrain_model[index].nz = height_map[index1].nz;
				terrain_model[index].tu = 0.0f;
				terrain_model[index].tv = 1.0f;
				index++;

				// Bottom left.
				terrain_model[index].x = height_map[index1].x;
				terrain_model[index].y = height_map[index1].y;
				terrain_model[index].z = height_map[index1].z;
				terrain_model[index].nx = height_map[index1].nx;
				terrain_model[index].ny = height_map[index1].ny;
				terrain_model[index].nz = height_map[index1].nz;
				terrain_model[index].tu = 0.0f;
				terrain_model[index].tv = 1.0f;
				index++;

				// Upper right.
				terrain_model[index].x = height_map[index4].x;
				terrain_model[index].y = height_map[index4].y;
				terrain_model[index].z = height_map[index4].z;
				terrain_model[index].nx = height_map[index4].nx;
				terrain_model[index].ny = height_map[index4].ny;
				terrain_model[index].nz = height_map[index4].nz;
				terrain_model[index].tu = 1.0f;
				terrain_model[index].tv = 0.0f;
				index++;

				// Bottom right.
				terrain_model[index].x = height_map[index2].x;
				terrain_model[index].y = height_map[index2].y;
				terrain_model[index].z = height_map[index2].z;
				terrain_model[index].nx = height_map[index2].nx;
				terrain_model[index].ny = height_map[index2].ny;
				terrain_model[index].nz = height_map[index2].nz;
				terrain_model[index].tu = 1.0f;
				terrain_model[index].tv = 1.0f;
				index++;
			}
		}

		return true;
	}

	void Terrain::calculateTerrainVectors()
	{
		TempVertexType vertex1, vertex2, vertex3;
		VectorType tangent, binormal;

		auto faceCount = vertex_count / 3;

		auto index = int();
		for(auto i = int(); i<faceCount; i++)
		{
			// Get the three vertices for this face from the terrain model.
			vertex1.x = terrain_model[index].x;
			vertex1.y = terrain_model[index].y;
			vertex1.z = terrain_model[index].z;
			vertex1.tu = terrain_model[index].tu;
			vertex1.tv = terrain_model[index].tv;
			vertex1.nx = terrain_model[index].nx;
			vertex1.ny = terrain_model[index].ny;
			vertex1.nz = terrain_model[index].nz;
			index++;

			vertex2.x = terrain_model[index].x;
			vertex2.y = terrain_model[index].y;
			vertex2.z = terrain_model[index].z;
			vertex2.tu = terrain_model[index].tu;
			vertex2.tv = terrain_model[index].tv;
			vertex2.nx = terrain_model[index].nx;
			vertex2.ny = terrain_model[index].ny;
			vertex2.nz = terrain_model[index].nz;
			index++;

			vertex3.x = terrain_model[index].x;
			vertex3.y = terrain_model[index].y;
			vertex3.z = terrain_model[index].z;
			vertex3.tu = terrain_model[index].tu;
			vertex3.tv = terrain_model[index].tv;
			vertex3.nx = terrain_model[index].nx;
			vertex3.ny = terrain_model[index].ny;
			vertex3.nz = terrain_model[index].nz;
			index++;

			calculateTangentBinormal(vertex1, vertex2, vertex3, tangent, binormal);

			terrain_model[index - 1].tx = tangent.x;
			terrain_model[index - 1].ty = tangent.y;
			terrain_model[index - 1].tz = tangent.z;
			terrain_model[index - 1].bx = binormal.x;
			terrain_model[index - 1].by = binormal.y;
			terrain_model[index - 1].bz = binormal.z;

			terrain_model[index - 2].tx = tangent.x;
			terrain_model[index - 2].ty = tangent.y;
			terrain_model[index - 2].tz = tangent.z;
			terrain_model[index - 2].bx = binormal.x;
			terrain_model[index - 2].by = binormal.y;
			terrain_model[index - 2].bz = binormal.z;

			terrain_model[index - 3].tx = tangent.x;
			terrain_model[index - 3].ty = tangent.y;
			terrain_model[index - 3].tz = tangent.z;
			terrain_model[index - 3].bx = binormal.x;
			terrain_model[index - 3].by = binormal.y;
			terrain_model[index - 3].bz = binormal.z;
		}
	}


	void Terrain::calculateTangentBinormal(TempVertexType vertex1, TempVertexType vertex2, TempVertexType vertex3, VectorType& tangent, VectorType& binormal)
	{
		float vector1[3], vector2[3];
		float tuVector[2], tvVector[2];

		vector1[0] = vertex2.x - vertex1.x;
		vector1[1] = vertex2.y - vertex1.y;
		vector1[2] = vertex2.z - vertex1.z;

		vector2[0] = vertex3.x - vertex1.x;
		vector2[1] = vertex3.y - vertex1.y;
		vector2[2] = vertex3.z - vertex1.z;

		tuVector[0] = vertex2.tu - vertex1.tu;
		tvVector[0] = vertex2.tv - vertex1.tv;

		tuVector[1] = vertex3.tu - vertex1.tu;
		tvVector[1] = vertex3.tv - vertex1.tv;

		auto den = 1.0f / (tuVector[0] * tvVector[1] - tuVector[1] * tvVector[0]);

		tangent.x = (tvVector[1] * vector1[0] - tvVector[0] * vector2[0]) * den;
		tangent.y = (tvVector[1] * vector1[1] - tvVector[0] * vector2[1]) * den;
		tangent.z = (tvVector[1] * vector1[2] - tvVector[0] * vector2[2]) * den;

		binormal.x = (tuVector[0] * vector2[0] - tuVector[1] * vector1[0]) * den;
		binormal.y = (tuVector[0] * vector2[1] - tuVector[1] * vector1[1]) * den;
		binormal.z = (tuVector[0] * vector2[2] - tuVector[1] * vector1[2]) * den;

		auto length = sqrt((tangent.x * tangent.x) + (tangent.y * tangent.y) + (tangent.z * tangent.z));

		tangent.x = tangent.x / length;
		tangent.y = tangent.y / length;
		tangent.z = tangent.z / length;

		length = sqrt((binormal.x * binormal.x) + (binormal.y * binormal.y) + (binormal.z * binormal.z));

		binormal.x = binormal.x / length;
		binormal.y = binormal.y / length;
		binormal.z = binormal.z / length;
	}


	bool Terrain::initializeBuffers(ID3D11Device* device)
	{
		index_count = vertex_count;

		auto vertices = new VertexType[vertex_count];
		if(!vertices)
			return false;

		auto indices = new unsigned long[index_count];
		if(!indices)
			return false;

		for(auto i = int(); i < vertex_count; i++)
		{
			vertices[i].position = Vector3D(terrain_model[i].x, terrain_model[i].y, terrain_model[i].z);
			vertices[i].texture = Vector2D(terrain_model[i].tu, terrain_model[i].tv);
			vertices[i].normal = Vector3D(terrain_model[i].nx, terrain_model[i].ny, terrain_model[i].nz);
			vertices[i].tangent = Vector3D(terrain_model[i].tx, terrain_model[i].ty, terrain_model[i].tz);
			vertices[i].binormal = Vector3D(terrain_model[i].bx, terrain_model[i].by, terrain_model[i].bz);

			indices[i] = i;
		}

		D3D11_BUFFER_DESC vertex_buffer_desc;
		vertex_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		vertex_buffer_desc.ByteWidth = sizeof(VertexType) * vertex_count;
		vertex_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertex_buffer_desc.CPUAccessFlags = 0U;
		vertex_buffer_desc.MiscFlags = 0U;
		vertex_buffer_desc.StructureByteStride = 0U;

		D3D11_SUBRESOURCE_DATA vertex_data;
		vertex_data.pSysMem = vertices;
		vertex_data.SysMemPitch = 0U;
		vertex_data.SysMemSlicePitch = 0U;

		auto result = device->CreateBuffer(&vertex_buffer_desc, &vertex_data, &vertex_buffer);
		if(FAILED(result))
			return false;

		D3D11_BUFFER_DESC index_buffer_desc;
		index_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		index_buffer_desc.ByteWidth = sizeof(decltype(index_count)) * index_count;
		index_buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		index_buffer_desc.CPUAccessFlags = 0U;
		index_buffer_desc.MiscFlags = 0U;
		index_buffer_desc.StructureByteStride = 0U;

		D3D11_SUBRESOURCE_DATA index_data;
		index_data.pSysMem = indices;
		index_data.SysMemPitch = 0U;
		index_data.SysMemSlicePitch = 0U;

		result = device->CreateBuffer(&index_buffer_desc, &index_data, &index_buffer);
		if(FAILED(result))
			return false;

		delete[] vertices;
		delete[] indices;

		return true;
	}

	bool Terrain::loadTextures(ID3D11Device* device, const wchar_t* diffuse_texture_file_name, const wchar_t* bump_map_file_name)
	{
#ifdef DEPRECATED_CODE
		auto x = DirectX::CreateWICTextureFromFile(device, device_con, diffuse_texture_file_name, nullptr, &diffuse_texture);
		auto x = D3DX11CreateShaderResourceViewFromFileA(device, dd.c_str(), nullptr, nullptr, &diffuse_texture, nullptr);
#endif
		auto x = DirectX::CreateDDSTextureFromFile(device, diffuse_texture_file_name, nullptr, &diffuse_texture);
		if(FAILED(x))
			return false;

#ifdef DEPRECATED_CODE
		x = DirectX::CreateWICTextureFromFile(device, device_con, bump_map_file_name, nullptr, &bump_texture);
		x = D3DX11CreateShaderResourceViewFromFileA(device, ss.c_str(), nullptr, nullptr, &bump_texture, nullptr);
#endif

		x = DirectX::CreateDDSTextureFromFile(device, bump_map_file_name, nullptr, &bump_texture);
		if(FAILED(x))
			return false;

		return true;
	}
}

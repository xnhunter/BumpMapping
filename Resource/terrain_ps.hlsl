// Copyright (c) 2018 Valentyn Bondarenko. All rights reserved.

Texture2D diffuse_texture : register(t0);
Texture2D bump_texture : register(t1);


SamplerState SampleType;


cbuffer LightBuffer
{
	float4 diffuse_color;
	float3 light_direction;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
   	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 binormal : BINORMAL;
    float4 depth_position : TEXCOORD1;
};

float4 TerrainPixelShader(PixelInputType input) : SV_TARGET
{
    // Get the depth value of the pixel by dividing the Z pixel depth by the homogeneous W coordinate.
    float depth = input.depth_position.z / input.depth_position.w;

    // Sample the texture pixel at this location.
    float4 texture_color = diffuse_texture.Sample(SampleType, input.tex);
	
	float3 bump_normal;
	if(depth < 0.9999f)
	{    
		// Sample the pixel in the bump map.
		float4 bump_map = bump_texture.Sample(SampleType, input.tex);

		// Expand the range of the normal value from (0, +1) to (-1, +1).
	    bump_map = (bump_map * 1.82f) - 1.0f; // 2.1 is height for bump map

		// Calculate the normal from the data in the bump map.
		bump_normal = input.normal + bump_map.x * input.tangent + bump_map.y * input.binormal;

		// Normalize the resulting bump normal.
		bump_normal = normalize(bump_normal);
	}
	else
		bump_normal = input.normal;
	
    // Invert the light direction for calculations.
    float3 light_dir = -light_direction;

    // Calculate the amount of light on this pixel based on the bump map normal value.
    float lightIntensity = saturate(dot(bump_normal, light_dir));

    // Determine the final diffuse color based on the diffuse color and the amount of light intensity.
    float4 color = saturate(diffuse_color * lightIntensity);

    // Combine the final bump light color with the texture color.
    color = color * texture_color;

    return color;
}
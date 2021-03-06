#include "register_locations_pixel_shader.h"

Texture2D ShaderTexture : register(DIFFUSE_PS_HLSL);
SamplerState SampleType : register(SAMPLER_PS_HLSL);

struct UIPixelInput
{
    float4 screenPosition : SV_POSITION;
    float4 color : COLOR;
    float2 tex : TEXCOORD0;
};

float4 main(UIPixelInput input) : SV_TARGET
{
    clip(input.color.a - 0.0001f);
    
    float4 color = ShaderTexture.Sample(SampleType, input.tex) * input.color;
	
    return color;
}

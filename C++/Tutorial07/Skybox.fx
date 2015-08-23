TextureCube SkyMap;
SamplerState samLinear : register(s0);

cbuffer MatrixBuffer : register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;
};

cbuffer ColorBuffer : register(b1)
{
	float4 vMeshColor;
};

cbuffer LightBuffer : register(b2)
{
	float4 ambientColor;
	float4 diffuseColor;
	float3 lightDirection;
	float specularPower;
	float4 specularColor;
};

cbuffer CameraBuffer : register(b3)
{
	float3 cameraPosition;
	float padding;
};

struct VS_INPUT
{
	float4 Pos : POSITION;
	float2 Tex : TEXCOORD0;
	float3 Norm : NORMAL;
};

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float3 Tex : TEXCOORD0;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.Tex = input.Pos;
	output.Pos = mul(input.Pos, World);
	output.Pos = mul(output.Pos, View);
	output.Pos = mul(output.Pos, Projection);
	output.Pos = output.Pos.xyww;
	return output;
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(PS_INPUT input) : SV_Target
{
	float fogFactor = 0.04f;
	float4 textureColor;
	float4 color;
	float4 fogColor;
	fogColor = float4(0.5f, 0.5f, 0.5f, 1.0f);
	textureColor = SkyMap.Sample(samLinear, input.Tex);
	color = fogFactor * textureColor + (1.0 - fogFactor) * fogColor;
	return color;
}
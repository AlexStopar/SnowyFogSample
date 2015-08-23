Texture2D txDiffuse : register(t0);
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

cbuffer FogBuffer : register(b4)
{
	float fogNear;
	float fogFar;
	float isFogOn;
	float fogPower;
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
	float2 Tex : TEXCOORD0;
	float fogFactor : FOG;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	//Calculate vs MVP matrix
	output.Pos = mul(input.Pos, World);
	output.Pos = mul(output.Pos, View);
	output.Pos = mul(output.Pos, Projection);
	//Set texture coordinates
	output.Tex = input.Tex;
	
	//vary fog based on linear depth value; cap at 1.0f
	float z = output.Pos.z / output.Pos.w;
	z = (z * 2.0f) - 1.0f;
	output.fogFactor = (2.0f * fogNear) / (fogFar + fogNear - (z * (fogFar - fogNear)));
	output.fogFactor *= fogPower;
	if (output.fogFactor > 1.0f) output.fogFactor = 1.0f;
	else if (output.fogFactor < 0.0f) output.fogFactor = 0.0f;
	return output;
}



//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(PS_INPUT input) : SV_Target
{
	float4 textureColor;
	float4 fogColor;
	float4 color;
	
	//Set fog color to a spooky grey
	fogColor = float4(0.5f, 0.5f, 0.5f, 1.0f);
	textureColor = txDiffuse.Sample(samLinear, input.Tex);
	color = ambientColor;
	
	color = color * textureColor;
	color = lerp(color, fogColor, input.fogFactor);
	color = saturate(color);
	return color;
}
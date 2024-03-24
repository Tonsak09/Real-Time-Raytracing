#include "ShaderInclude.hlsli"
#include "PBRFunctions.hlsli"

Texture2D Albedo : register(t0);
Texture2D NormalMap : register(t1);
Texture2D RoughnessMap : register(t2);
Texture2D MetalnessMap : register(t3);

SamplerState texSampler : register(s0);

// MUST match with lights.h 
#define MAX_LIGHTS 5

// Alignment matters!!!
cbuffer ExternalData : register(b0)
{
    float2 uvScale;
    float2 uvOffset;
    float3 cameraPosition;
    int lightCount;
    Light lights[MAX_LIGHTS];
}


float2 GetUV(VertexToPixel input)
{
    return input.uv;
}

/// <summary>
/// Lowers light strength over distance 
/// </summary>
float Attenuate(Light light, float3 worldPos)
{
	float dist = distance(light.position, worldPos);
	float att = saturate(1.0f - (dist * dist / (light.range * light.range)));
	return att * att;
}

float3 GetSpec(VertexToPixel input, float3 albedo, float metalness)
{
	return lerp(F0_NON_METAL, albedo, metalness);
}

float3 DirLight(Light light, VertexToPixel input, float roughness, float metalness, float3 albedo, float3 specColor)
{
	float3 lightDir = normalize(-light.directiton);
	float3 V = normalize(cameraPosition - input.worldPosition);


	//float specExponent = (1.0f - roughness) * MAX_SPECULAR_EXPONENT;
	float3 R = reflect(input.normal, lightDir);

	float3 diffuse = saturate(dot(input.normal, lightDir));
	float3 F;
	float3 spec = MicrofacetBRDF(input.normal, lightDir, V, roughness, specColor, F);

	// Calculate diffuse with energy conservation, including cutting diffuse for metals
	float3 balancedDiff = DiffuseEnergyConserve(diffuse, spec, metalness);
	// Combine the final diffuse and specular values for this light
	float3 total = (balancedDiff * albedo + spec) * light.intensity * light.color;


	return total;
}

float3 PointLight(Light light, VertexToPixel input, float roughness, float metalness, float3 albedo, float3 specColor)
{
	float3 difference = light.position - input.worldPosition;
	float mag = length(difference);


	if (mag > light.range)
		return float3(0, 0, 0);

	float3 lightDir = normalize(light.position - input.worldPosition);
	float3 V = normalize(cameraPosition - input.worldPosition);

	float atten = Attenuate(light, input.worldPosition);


	float3 diffuse = saturate(dot(input.normal, lightDir));
	float3 F;
	float3 spec = MicrofacetBRDF(input.normal, lightDir, V, roughness, specColor, F);

	float3 balancedDiff = DiffuseEnergyConserve(diffuse, spec, metalness);

	float3 total = (balancedDiff * albedo + spec) * light.intensity * light.color;

	return total;
}

float4 main(VertexToPixel input) : SV_TARGET
{
	//return (float4(input.uv, 0.0f, 1.0f));
	float roughness = RoughnessMap.Sample(texSampler, input.uv).r;
	float metalness = MetalnessMap.Sample(texSampler, input.uv).r;
	float3 albedo = Albedo.Sample(texSampler, input.uv).rgb;
	float3 specColor = GetSpec(input, albedo, metalness);

	float3 unpackedNormal = NormalMap.Sample(texSampler, input.uv).rgb * 2 - 1;
	unpackedNormal = normalize(unpackedNormal);

	// Simplifications include not re-normalizing the same vector more than once!
	float3 N = normalize(input.normal); // Must be normalized here or before
	float3 T = normalize(input.tangent); // Must be normalized here or before
	T = normalize(T - N * dot(T, N)); // Gram-Schmidt assumes T&N are normalized!
	float3 B = cross(T, N);
	float3x3 TBN = float3x3(T, B, N);

	// Assumes that input.normal is the normal later in the shader
	input.normal = mul(unpackedNormal, TBN); // Note multiplication order!

	float3 totalLight; 
	for (int i = 0; i < 4; i++)
	{
		totalLight += lights[i].type == 0 ? PointLight(lights[i], input, roughness, metalness, albedo, specColor) : DirLight(lights[i], input, roughness, metalness, albedo, specColor);
	}

	return float4(pow(totalLight, 1.0f / 2.2f), 1);

    return NormalMap.Sample(texSampler, GetUV(input));
}
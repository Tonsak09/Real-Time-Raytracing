#pragma once
#include <DirectXMath.h>
#include "Lights.h"

struct VertexShaderExternalData
{
	DirectX::XMFLOAT4X4 world, worldInvTranspose, view, projection;
};

struct PixelShaderExternalData
{
	DirectX::XMFLOAT2 uvScale, uvOffset;
	DirectX::XMFLOAT3 cameraPosition;
	int lightCount;
	Light lights[MAX_LIGHTS];
};

// Overall scene data for raytracing
struct RaytracingSceneData
{
	DirectX::XMFLOAT4X4 inverseViewProjection;
	DirectX::XMFLOAT3 cameraPosition;
	float pad0;
};

// Ensure this matches Raytracing shader define!
#define MAX_INSTANCES_PER_BLAS 100
struct RaytracingEntityData
{
	DirectX::XMFLOAT4 color[MAX_INSTANCES_PER_BLAS];
	DirectX::XMFLOAT4 lightHue[MAX_INSTANCES_PER_BLAS];
};
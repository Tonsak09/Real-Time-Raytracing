#pragma once
#include <DirectXMath.h>

#define MAX_LIGHTS 5

#define LIGHT_POINT		0
#define LIGHT_DIRECTION 1

struct Light
{
public:
	int type;
	DirectX::XMFLOAT3 directiton;
	float range;
	DirectX::XMFLOAT3 position;
	float intensity;
	DirectX::XMFLOAT3 color;
	float spotFalloff;
	DirectX::XMFLOAT3 padding;
};
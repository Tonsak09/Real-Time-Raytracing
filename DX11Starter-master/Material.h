#pragma once

#include "DXCore.h"
#include <DirectXMath.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include <memory>

using namespace DirectX;

class Material
{
private:
	XMFLOAT3 colorTint;
	XMFLOAT2 uvScale;
	XMFLOAT2 uvOffset;
	bool finalized;
	
	// Should these be in com ptrs?
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
	D3D12_CPU_DESCRIPTOR_HANDLE textureSRVsBySlot[4];
	D3D12_GPU_DESCRIPTOR_HANDLE finalGPUHandleForSRVs;

public: 
	Material(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState, XMFLOAT3 colorTint, XMFLOAT2 uvScale, XMFLOAT2 uvOffset);

	XMFLOAT3 GetColorTint();
	XMFLOAT2 GetuvScale();
	XMFLOAT2 GetuvOffset();
	bool GetFinalized();
	Microsoft::WRL::ComPtr<ID3D12PipelineState> GetPipelineState();
	D3D12_GPU_DESCRIPTOR_HANDLE GetFinalGPUHandleForTextures();

	void AddTexture(D3D12_CPU_DESCRIPTOR_HANDLE srv, int slot);
	void FinalizeMaterial();
};

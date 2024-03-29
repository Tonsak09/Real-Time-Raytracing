#include "Material.h" 
#include "DX12Helper.h"

Material::Material(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState, XMFLOAT4 colorTint, XMFLOAT2 uvScale, XMFLOAT2 uvOffset, XMFLOAT4 lightHue) :
	pipelineState(pipelineState), colorTint(colorTint), uvScale(uvScale), uvOffset(uvOffset), lightHue(lightHue)
{
	finalized = false;

	//textureSRVsBySlot[4];
}

void Material::AddTexture(D3D12_CPU_DESCRIPTOR_HANDLE srv, int slot)
{
	if (slot < 0 || slot >= 4)
		return;

	textureSRVsBySlot[slot] = srv;
}

void Material::FinalizeMaterial()
{
	if (finalized)
		return;

	DX12Helper& dx12Helper = DX12Helper::GetInstance();
	// Each much be done one at a time since they are each
	// their own heap. They are NOT a continous array of 
	// SRV's but a continous array of heaps 
	finalGPUHandleForSRVs = dx12Helper.HeapSRVsToDescHeap(1, textureSRVsBySlot[0]);
	dx12Helper.HeapSRVsToDescHeap(1, textureSRVsBySlot[1]);
	dx12Helper.HeapSRVsToDescHeap(1, textureSRVsBySlot[2]);
	dx12Helper.HeapSRVsToDescHeap(1, textureSRVsBySlot[3]);
	finalized = true;
}

XMFLOAT4 Material::GetColorTint()
{
	return colorTint;
}

XMFLOAT2 Material::GetuvScale()
{
	return uvScale;
}

XMFLOAT2 Material::GetuvOffset()
{
	return uvOffset;
}

XMFLOAT4 Material::GetLightHue()
{
	return lightHue;
}

bool Material::GetFinalized()
{
	return finalized;
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> Material::GetPipelineState()
{
	return pipelineState;
}

D3D12_GPU_DESCRIPTOR_HANDLE Material::GetFinalGPUHandleForTextures()
{
	return finalGPUHandleForSRVs;
}

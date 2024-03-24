#pragma once


#include <d3d12.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include "Vertex.h"

#include <fstream>
#include <vector>
#include <DirectXMath.h>

#include "MeshRaytracingData.h"

class Mesh
{
private:
	void ContructVIBuffers(Vertex vertices[], unsigned int indices[], unsigned int vertexCount, unsigned int indexCount);

	// Buffers that connect data to the GPU 
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vbView;
	D3D12_INDEX_BUFFER_VIEW ibView;

	int indicesCount;
	int vertexCount;

	void CalculateTangents(Vertex* verts, int numVerts, unsigned int* indices, int numIndices);

public:
	/// <summary>
	/// Create a mesh based on manually given vertex data
	/// </summary>
	Mesh(Vertex vertices[], unsigned int indices[], int vertexCount, int indexCount, bool constructTangents = true);
	/// <summary>
	/// Create a mesh based on a given obj file 
	/// </summary>
	Mesh(const wchar_t* file);
	~Mesh();

	//Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer();
	//Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer();
	D3D12_VERTEX_BUFFER_VIEW GetVertexView();
	D3D12_INDEX_BUFFER_VIEW GetIndexView();

	int GetVertexCount();
	int GetIndexCount();

	void Draw();

public:
	D3D12_VERTEX_BUFFER_VIEW GetVBView() { return vbView; } // Renamed
	D3D12_INDEX_BUFFER_VIEW GetIBView() { return ibView; } // Renamed
	Microsoft::WRL::ComPtr<ID3D12Resource> GetVBResource() { return vertexBuffer; }
	Microsoft::WRL::ComPtr<ID3D12Resource> GetIBResource() { return indexBuffer; }
	MeshRaytracingData GetRaytracingData() { return raytracingData; }
private:
	MeshRaytracingData raytracingData;
};


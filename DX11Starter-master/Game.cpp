#include "Game.h"
#include "Vertex.h"
#include "Input.h"
#include "PathHelpers.h"
#include "RaytracingHelper.h"

// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

#include "DX12Helper.h"

// For the DirectX Math library
using namespace DirectX;

// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// Direct3D itself, and our window, are not ready at this point!
//
// hInstance - the application's OS-level handle (unique ID)
// --------------------------------------------------------
Game::Game(HINSTANCE hInstance)
	: DXCore(
		hInstance,			// The application's handle
		L"DirectX Game",	// Text for the window's title bar (as a wide-character string)
		1280,				// Width of the window's client area
		720,				// Height of the window's client area
		false,				// Sync the framerate to the monitor refresh? (lock framerate)
		true)				// Show extra stats (fps) in title bar?
{
#if defined(DEBUG) || defined(_DEBUG)
	// Do we want a console window?  Probably only in debug mode
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.\n");
#endif

	ibView = {};
	vbView = {};

	fov = 1.0f;
}

// --------------------------------------------------------
// Destructor - Clean up anything our game has created:
//  - Delete all objects manually created within this class
//  - Release() all Direct3D objects created within this class
// --------------------------------------------------------
Game::~Game()
{
	// Call delete or delete[] on any objects or arrays you've
	// created using new or new[] within this class
	// - Note: this is unnecessary if using smart pointers

	// Call Release() on any Direct3D objects made within this class
	// - Note: this is unnecessary for D3D objects stored in ComPtrs

	// We need to wait here until the GPU
	// is actually done with its work
	DX12Helper::GetInstance().WaitForGPU();
	delete& RaytracingHelper::GetInstance();
}

// --------------------------------------------------------
// Called once per program, after Direct3D and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	// Attempt to initialize DXR
	RaytracingHelper::GetInstance().Initialize(
		windowWidth,
		windowHeight,
		device,
		commandQueue,
		commandList,
		FixPath(L"Raytracing.cso"));

	CreateRootSigAndPipelineState();
	CreateCamera();
	CreateGeometry();
	CreateLights();
}

// --------------------------------------------------------
// Loads the two basic shaders, then creates the root signature
// and pipeline state object for our very basic demo.
// --------------------------------------------------------
void Game::CreateRootSigAndPipelineState()
{
	// Blobs to hold raw shader byte code used in several steps below
	Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderByteCode;
	Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderByteCode;
	// Load shaders
	{
		// Read our compiled vertex shader code into a blob
		// - Essentially just "open the file and plop its contents here"
		D3DReadFileToBlob(FixPath(L"VertexShader.cso").c_str(), vertexShaderByteCode.GetAddressOf());

		D3DReadFileToBlob(FixPath(L"PixelShader.cso").c_str(), pixelShaderByteCode.GetAddressOf());
	}
	// Input layout 
	// THIS ORDER MATTERS! Match with VertexShaderInput
	const unsigned int inputElementCount = 4;
	D3D12_INPUT_ELEMENT_DESC inputElements[inputElementCount] = {};
	{
		inputElements[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		inputElements[0].SemanticName = "POSITION";
		inputElements[0].SemanticIndex = 0;

		inputElements[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElements[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		inputElements[1].SemanticName = "NORMAL";
		inputElements[1].SemanticIndex = 0;

		inputElements[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElements[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		inputElements[2].SemanticName = "TANGENT";
		inputElements[2].SemanticIndex = 0;

		inputElements[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElements[3].Format = DXGI_FORMAT_R32G32_FLOAT;
		inputElements[3].SemanticName = "TEXCOORD";
		inputElements[3].SemanticIndex = 0;
	}
	// Root Signature
	{
		// Describe the range of CBVs needed for the vertex shader
		D3D12_DESCRIPTOR_RANGE cbvRangeVS = {};
		cbvRangeVS.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		cbvRangeVS.NumDescriptors = 1;
		cbvRangeVS.BaseShaderRegister = 0;
		cbvRangeVS.RegisterSpace = 0;
		cbvRangeVS.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		// Describe the range of CBVs needed for the pixel shader
		D3D12_DESCRIPTOR_RANGE cbvRangePS = {};
		cbvRangePS.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		cbvRangePS.NumDescriptors = 1;
		cbvRangePS.BaseShaderRegister = 0;
		cbvRangePS.RegisterSpace = 0;
		cbvRangePS.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		// Create a range of SRV's for textures
		D3D12_DESCRIPTOR_RANGE srvRange = {};
		srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		srvRange.NumDescriptors = 4; // Set to max number of textures at once (match pixel shader!)
		
		srvRange.BaseShaderRegister = 0; // Starts at s0 (match pixel shader!)
		srvRange.RegisterSpace = 0;
		srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		// Create the root parameters
		D3D12_ROOT_PARAMETER rootParams[3] = {};

		// CBV table param for vertex shader
		rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[0].DescriptorTable.pDescriptorRanges = &cbvRangeVS;

		// CBV table param for pixel shader
		rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[1].DescriptorTable.pDescriptorRanges = &cbvRangePS;

		// SRV table param
		rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[2].DescriptorTable.pDescriptorRanges = &srvRange;

		// Create a single static sampler (available to all pixel shaders at the same slot)
		D3D12_STATIC_SAMPLER_DESC anisoWrap = {};
		anisoWrap.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		anisoWrap.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		anisoWrap.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		anisoWrap.Filter = D3D12_FILTER_ANISOTROPIC;
		anisoWrap.MaxAnisotropy = 16;
		anisoWrap.MaxLOD = D3D12_FLOAT32_MAX;
		anisoWrap.ShaderRegister = 0; // register(s0)
		anisoWrap.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		D3D12_STATIC_SAMPLER_DESC samplers[] = { anisoWrap };

		// Describe and serialize the root signature
		D3D12_ROOT_SIGNATURE_DESC rootSig = {};
		rootSig.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		rootSig.NumParameters = ARRAYSIZE(rootParams);
		rootSig.pParameters = rootParams;
		rootSig.NumStaticSamplers = ARRAYSIZE(samplers);
		rootSig.pStaticSamplers = samplers;


		ID3DBlob* serializedRootSig = 0;
		ID3DBlob* errors = 0;
		D3D12SerializeRootSignature(
			&rootSig,
			D3D_ROOT_SIGNATURE_VERSION_1,
			&serializedRootSig,
			&errors);
		// Check for errors during serialization
		if (errors != 0)
		{
			OutputDebugString((wchar_t*)errors->GetBufferPointer());
		}
		// Actually create the root sig
		device->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(rootSignature.GetAddressOf()));

	}
	// Pipeline state
	{
		// Describe the pipeline state
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		// -- Input assembler related ---
		psoDesc.InputLayout.NumElements = inputElementCount;
		psoDesc.InputLayout.pInputElementDescs = inputElements;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		// Root sig
		psoDesc.pRootSignature = rootSignature.Get();
		// -- Shaders (VS/PS) ---
		psoDesc.VS.pShaderBytecode = vertexShaderByteCode->GetBufferPointer();
		psoDesc.VS.BytecodeLength = vertexShaderByteCode->GetBufferSize();
		psoDesc.PS.pShaderBytecode = pixelShaderByteCode->GetBufferPointer();
		psoDesc.PS.BytecodeLength = pixelShaderByteCode->GetBufferSize();
		// -- Render targets ---
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;
		// -- States ---
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		psoDesc.RasterizerState.DepthClipEnable = true;
		psoDesc.DepthStencilState.DepthEnable = true;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
		psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		// -- Misc ---
		psoDesc.SampleMask = 0xffffffff;
		// Create the pipe state object
		device->CreateGraphicsPipelineState(&psoDesc,
			IID_PPV_ARGS(pipelineState.GetAddressOf()));
	}
}

// --------------------------------------------------------
// Creates the perspective camera and stores it 
// --------------------------------------------------------
void Game::CreateCamera()
{
	camera = std::make_shared<Camera>(
		0.0f, 0.0f, -10.0f,							// Origin
		1.0f,										// Move Speed 
		20.0f,										// Sprint Move Speed
		0.1f,										// Mouse Look Speed 
		fov,										// FOV
		(float)windowWidth / (float)windowHeight	// Aspect Ratio
	);
}

// --------------------------------------------------------
// Creates the geometry we're going to draw 
// --------------------------------------------------------
void Game::CreateGeometry()
{
	DX12Helper& dx12Helper = DX12Helper::GetInstance();
	// rootSig.pStaticSamplers[0];


	std::shared_ptr<Mesh> sphere = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/sphere.obj").c_str());
	std::shared_ptr<Mesh> helix = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/helix.obj").c_str());
	std::shared_ptr<Mesh> torus = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/torus.obj").c_str());

	double spawnRange = 10.0f;
	srand(time(0));

	// Spheres 
	for (int i = 0; i < 10; i++)
	{
		std::shared_ptr<Material> basicMat = std::make_shared<Material>(
			pipelineState,
			DirectX::XMFLOAT3(((double)rand()) / RAND_MAX, ((double)rand()) / RAND_MAX, ((double)rand()) / RAND_MAX), // Color
			DirectX::XMFLOAT2(1.0f, 1.0f),
			DirectX::XMFLOAT2(0.0f, 0.0f));


		basicMat->AddTexture(dx12Helper.LoadTexture(FixPath(L"../../Assets/Textures/Foil002_4K-JPG_Color.jpg").c_str()), 0);
		basicMat->AddTexture(dx12Helper.LoadTexture(FixPath(L"../../Assets/Textures/Foil002_4K-JPG_NormalDX.jpg").c_str()), 1);
		basicMat->AddTexture(dx12Helper.LoadTexture(FixPath(L"../../Assets/Textures/Foil002_4K-JPG_Roughness.jpg").c_str()), 2);
		basicMat->AddTexture(dx12Helper.LoadTexture(FixPath(L"../../Assets/Textures/Foil002_4K-JPG_Metalness.jpg").c_str()), 3);

		basicMat->FinalizeMaterial();

		double xRand = ((double)rand()) / RAND_MAX;
		double x = -spawnRange + (spawnRange - -spawnRange) * xRand;

		double zRand = ((double)rand()) / RAND_MAX;
		double z = -spawnRange + (spawnRange - -spawnRange) * zRand;

		// Set meshes to entities 
		entities.push_back(std::make_shared<Entity>(sphere, basicMat));
		entities[entities.size() - 1]->GetTransform()->SetPosition(x, 0.0f, z);
	}

	// Torus's
	for (int i = 0; i < 10; i++)
	{
		std::shared_ptr<Material> basicMat = std::make_shared<Material>(
			pipelineState,
			DirectX::XMFLOAT3(((double)rand()) / RAND_MAX, ((double)rand()) / RAND_MAX, ((double)rand()) / RAND_MAX), // Color
			DirectX::XMFLOAT2(1.0f, 1.0f),
			DirectX::XMFLOAT2(0.0f, 0.0f));


		basicMat->AddTexture(dx12Helper.LoadTexture(FixPath(L"../../Assets/Textures/Foil002_4K-JPG_Color.jpg").c_str()), 0);
		basicMat->AddTexture(dx12Helper.LoadTexture(FixPath(L"../../Assets/Textures/Foil002_4K-JPG_NormalDX.jpg").c_str()), 1);
		basicMat->AddTexture(dx12Helper.LoadTexture(FixPath(L"../../Assets/Textures/Foil002_4K-JPG_Roughness.jpg").c_str()), 2);
		basicMat->AddTexture(dx12Helper.LoadTexture(FixPath(L"../../Assets/Textures/Foil002_4K-JPG_Metalness.jpg").c_str()), 3);

		basicMat->FinalizeMaterial();

		double xRand = ((double)rand()) / RAND_MAX;
		double x = -spawnRange + (spawnRange - -spawnRange) * xRand;

		double zRand = ((double)rand()) / RAND_MAX;
		double z = -spawnRange + (spawnRange - -spawnRange) * zRand;

		// Set meshes to entities 
		entities.push_back(std::make_shared<Entity>(torus, basicMat));
		entities[entities.size() - 1]->GetTransform()->SetPosition(x, 0.0f, z);
		entities[entities.size() - 1]->GetTransform()->SetEulerRotation(
			-5.0 + (5.0 - -5.0) * ((double)rand()) / RAND_MAX,
			-5.0 + (5.0 - -5.0) * ((double)rand()) / RAND_MAX,
			-5.0 + (5.0 - -5.0) * ((double)rand()) / RAND_MAX);

	}

	for (int i = 0; i < 10; i++)
	{
		std::shared_ptr<Material> basicMat = std::make_shared<Material>(
			pipelineState,
			DirectX::XMFLOAT3(((double)rand()) / RAND_MAX, ((double)rand()) / RAND_MAX, ((double)rand()) / RAND_MAX), // Color
			DirectX::XMFLOAT2(1.0f, 1.0f),
			DirectX::XMFLOAT2(0.0f, 0.0f));


		basicMat->AddTexture(dx12Helper.LoadTexture(FixPath(L"../../Assets/Textures/Foil002_4K-JPG_Color.jpg").c_str()), 0);
		basicMat->AddTexture(dx12Helper.LoadTexture(FixPath(L"../../Assets/Textures/Foil002_4K-JPG_NormalDX.jpg").c_str()), 1);
		basicMat->AddTexture(dx12Helper.LoadTexture(FixPath(L"../../Assets/Textures/Foil002_4K-JPG_Roughness.jpg").c_str()), 2);
		basicMat->AddTexture(dx12Helper.LoadTexture(FixPath(L"../../Assets/Textures/Foil002_4K-JPG_Metalness.jpg").c_str()), 3);

		basicMat->FinalizeMaterial();

		double xRand = ((double)rand()) / RAND_MAX;
		double x = -spawnRange + (spawnRange - -spawnRange) * xRand;

		double zRand = ((double)rand()) / RAND_MAX;
		double z = -spawnRange + (spawnRange - -spawnRange) * zRand;

		// Set meshes to entities 
		entities.push_back(std::make_shared<Entity>(helix, basicMat));
		entities[entities.size() - 1]->GetTransform()->SetPosition(x, 0.0f, z);
		entities[entities.size() - 1]->GetTransform()->SetEulerRotation(
			-5.0 + (5.0 - -5.0) * ((double)rand()) / RAND_MAX,
			-5.0 + (5.0 - -5.0) * ((double)rand()) / RAND_MAX,
			-5.0 + (5.0 - -5.0) * ((double)rand()) / RAND_MAX);

	}

	

	//for (int i = 0; i < 1; i++)
	//{
	//	std::shared_ptr<Material> randMat = std::make_shared<Material>(
	//		pipelineState, 
	//		DirectX::XMFLOAT3(((double)rand()) / RAND_MAX, ((double)rand()) / RAND_MAX, ((double)rand()) / RAND_MAX), // Color
	//		DirectX::XMFLOAT2(1.0f, 1.0f),		 // UV Scale
	//		DirectX::XMFLOAT2(0.0f, 0.0f));		 // UV Offset 

	//	//basicMat->AddTexture(dx12Helper.LoadTexture(FixPath(L"../../Assets/Textures/Foil002_4K-JPG_Color.jpg").c_str()), 0);
	//	//basicMat->AddTexture(dx12Helper.LoadTexture(FixPath(L"../../Assets/Textures/Foil002_4K-JPG_NormalDX.jpg").c_str()), 1);
	//	//basicMat->AddTexture(dx12Helper.LoadTexture(FixPath(L"../../Assets/Textures/Foil002_4K-JPG_Roughness.jpg").c_str()), 2);
	//	//basicMat->AddTexture(dx12Helper.LoadTexture(FixPath(L"../../Assets/Textures/Foil002_4K-JPG_Metalness.jpg").c_str()), 3);

	//	randMat->FinalizeMaterial();
	//	entities.push_back(std::make_shared<Entity>(sphere, randMat));
	//}

	// Meshes create their own BLAS's; we just need to create the TLAS for the scene here
	RaytracingHelper::GetInstance().CreateTopLevelAccelerationStructureForScene(entities);
}

// --------------------------------------------------------
// Creates the lights for the scene 
// --------------------------------------------------------
void Game::CreateLights()
{
	/*
		DirectX::XMFLOAT3 directiton;
		float range;
		DirectX::XMFLOAT3 position;
		float intensity;
		DirectX::XMFLOAT3 color;
		float spotFalloff;
		DirectX::XMFLOAT3 padding;
	*/

	Light light1 = {};
	light1.type = LIGHT_POINT;
	light1.position = XMFLOAT3(5.0f, 0.0f, 0.0f);
	light1.color = XMFLOAT3(1.0f, 0.5f, 0.0f);
	light1.range = 20.0f;
	light1.spotFalloff = 0.3f;
	light1.intensity = 5.0f;

	Light light2 = {};
	light2.type = LIGHT_DIRECTION;
	light2.color = XMFLOAT3(0.0f, 0.5f, 0.5f);
	light2.directiton = XMFLOAT3(0.1f, -1.0f, 0.0f);
	light2.spotFalloff = 0.3f;
	light2.intensity = 5.0f;

	Light light3 = {};
	light3.type = LIGHT_DIRECTION;
	light3.color = XMFLOAT3(0.1f, 0.8f, 0.5f);
	light3.directiton = XMFLOAT3(0.0f, 1.0f, 0.2f);
	light3.spotFalloff = 0.3f;
	light3.intensity = 5.0f;

	Light light4 = {};
	light4.type = LIGHT_DIRECTION;
	light4.color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	light4.directiton = XMFLOAT3(0.0f, -1.0f, 0.2f);
	light4.spotFalloff = 0.3f;
	light4.intensity = 10.0f;

	lights.push_back(light1);
	lights.push_back(light2);
	lights.push_back(light3);
	lights.push_back(light4);
}


// --------------------------------------------------------
// Handle resizing to match the new window size.
//  - DXCore needs to resize the back buffer
//  - Eventually, we'll want to update our 3D camera
// --------------------------------------------------------
void Game::OnResize()
{
	// Handle base-level DX resize stuff
	DXCore::OnResize();
	camera->UpdateProjMatrix(fov, (float)windowWidth / (float)windowHeight);

	RaytracingHelper::GetInstance().ResizeOutputUAV(windowWidth, windowHeight);
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	// Example input checking: Quit if the escape key is pressed
	if (Input::GetInstance().KeyDown(VK_ESCAPE))
		Quit();

	camera->Update(deltaTime);

	// Temporary animations of entities 
	float lerp = InverseLerp(-1.0f, 1.0f, sin(totalTime));
	entities[0]->GetTransform()->SetPosition(0.0f, GetCurveByIndex(EASE_IN_BOUNCE, lerp) * 2.0f - 1.0f, 0.0f);
	entities[0]->GetTransform()->SetScale(GetCurveByIndex(EASE_IN_OUT_BOUNCE, lerp) + 0.5f, GetCurveByIndex(EASE_IN_OUT_BOUNCE, lerp) + 0.25f, 1.0f );

	entities[1]->GetTransform()->SetPosition(5.0f, GetCurveByIndex(EASE_IN_OUT_CUBIC, lerp) * 2.0f - 1.0f, 0.0f);
	entities[1]->GetTransform()->SetScale(1.0f, GetCurveByIndex(EASE_IN_OUT_CUBIC, lerp + 0.5f) , 1.0f);

	entities[2]->GetTransform()->SetPosition(-5.0f, GetCurveByIndex(EASE_IN_OUT_ELASTIC, lerp) * 2.0f - 1.0f, 0.0f);
	entities[2]->GetTransform()->SetScale(1.0f, GetCurveByIndex(EASE_IN_OUT_ELASTIC, lerp) + 0.1f, 1.0f);
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// Grab the current back buffer for this frame
	Microsoft::WRL::ComPtr<ID3D12Resource> currentBackBuffer = backBuffers[currentSwapBuffer];
	// Clearing the render target
	//{
	//	// Transition the back buffer from present to render target
	//	D3D12_RESOURCE_BARRIER rb = {};
	//	rb.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	//	rb.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	//	rb.Transition.pResource = currentBackBuffer.Get();
	//	rb.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	//	rb.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	//	rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	//	commandList->ResourceBarrier(1, &rb);
	//	// Background color (Cornflower Blue in this case) for clearing
	//	float color[] = { 0.4f, 0.6f, 0.75f, 1.0f };
	//	// Clear the RTV
	//	commandList->ClearRenderTargetView(
	//		rtvHandles[currentSwapBuffer],
	//		color,
	//		0, 0); // No scissor rectangles
	//	// Clear the depth buffer, too
	//	commandList->ClearDepthStencilView(
	//		dsvHandle,
	//		D3D12_CLEAR_FLAG_DEPTH,
	//		1.0f, // Max depth = 1.0f
	//		0, // Not clearing stencil, but need a value
	//		0, 0); // No scissor rects
	//}


	// Rendering here!
	{
		DX12Helper& dx12Helper = DX12Helper::GetInstance();

		// Set overall pipeline state
		commandList->SetPipelineState(pipelineState.Get());
		// Root sig (must happen before root descriptor table)
		commandList->SetGraphicsRootSignature(rootSignature.Get());
		// Set up other commands for rendering
		commandList->OMSetRenderTargets(1, &rtvHandles[currentSwapBuffer], true, &dsvHandle);
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorRect);
		
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap =
			dx12Helper.GetCBVSRVDescriptorHeap();
		commandList->SetDescriptorHeaps(1, descriptorHeap.GetAddressOf());

		// Don't need to remake vsData for each and every object 
		VertexShaderExternalData vsData = {};



		// Update raytracing accel structure
		RaytracingHelper::GetInstance(). // Does this create a top level each time? 
			CreateTopLevelAccelerationStructureForScene(entities);

		// Perform raytrace, including execution of command list
		RaytracingHelper::GetInstance().Raytrace(
			camera,
			backBuffers[currentSwapBuffer]);




		// Info shared by all entities 
		vsData.projection = *camera->GetProjMatrix();
		vsData.view = *camera->GetViewMatrix();

		// Draw each entity (per frame data)
		for (std::shared_ptr<Entity> entity : entities)
		{
			vsData.world = entity->GetTransform()->GetWorldMatrix();
			vsData.worldInvTranspose = entity->GetTransform()->GetWorldInverseTransposeMatrix();

			std::shared_ptr<Material> mat = entity->GetMaterial();
			commandList->SetPipelineState(mat->GetPipelineState().Get());

			// Set the SRV descriptor handle for this material's textures
			// Note: This assumes that descriptor table 2 is for textures (as per our root sig)
			commandList->SetGraphicsRootDescriptorTable(2, mat->GetFinalGPUHandleForTextures());



			// Pixel shader data and cbuffer setup
			{
				PixelShaderExternalData psData = {};
				psData.uvScale = mat->GetuvScale();
				psData.uvOffset = mat->GetuvOffset();
				psData.cameraPosition = *camera->GetTransform()->GetPosition().get();
				psData.lightCount = (int)lights.size();
				memcpy(psData.lights, &lights[0], sizeof(Light) * MAX_LIGHTS);
				// Send this to a chunk of the constant buffer heap
				// and grab the GPU handle for it so we can set it for this draw
					D3D12_GPU_DESCRIPTOR_HANDLE cbHandlePS =
					dx12Helper.FillNextConstantBufferAndGetGPUDescriptorHandle
				(
						(void*)(&psData), sizeof(PixelShaderExternalData));
				// Set this constant buffer handle
				// Note: This assumes that descriptor table 1 is the
				// place to put this particular descriptor. This
				// is based on how we set up our root signature.
				commandList->SetGraphicsRootDescriptorTable(1, cbHandlePS);
			}





			// Save to GPU 
			auto handle = dx12Helper.FillNextConstantBufferAndGetGPUDescriptorHandle(&vsData, sizeof(vsData));

			// Set the handle 
			commandList->SetGraphicsRootDescriptorTable(0, handle);

			// Set buffers 
			auto vertexView = entity->GetMesh()->GetVertexView();
			auto indexView = entity->GetMesh()->GetIndexView();
			commandList->IASetVertexBuffers(0, 1, &vertexView);
			commandList->IASetIndexBuffer(&indexView);

			commandList->DrawIndexedInstanced(entity->GetMesh()->GetIndexCount(), 1, 0, 0, 0);
		}
	}

	// Present
	{
		// Transition back to present
		D3D12_RESOURCE_BARRIER rb = {};
		rb.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		rb.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		rb.Transition.pResource = currentBackBuffer.Get();
		rb.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		rb.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &rb);
		// Must occur BEFORE present
		DX12Helper::GetInstance().CloseExecuteAndResetCommandList();
		// Present the current back buffer
		bool vsyncNecessary = vsync || !deviceSupportsTearing || isFullscreen;
		swapChain->Present(
			vsyncNecessary ? 1 : 0,
			vsyncNecessary ? 0 : DXGI_PRESENT_ALLOW_TEARING);
		// Figure out which buffer is next
		currentSwapBuffer++;
		if (currentSwapBuffer >= numBackBuffers)
			currentSwapBuffer = 0;
	}
}

float Game::InverseLerp(float a, float b, float v)
{
	return (v - a) / (b - a);
}
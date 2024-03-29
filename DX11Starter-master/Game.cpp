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
	std::shared_ptr<Mesh> cylinder = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/cylinder.obj").c_str());

	double spawnRange = 10.0f;
	srand(time(0));

	
	// Torus's
	for (int i = 0; i < 3; i++)
	{
		std::shared_ptr<Material> basicMat = std::make_shared<Material>(
			pipelineState,
			DirectX::XMFLOAT4(((double)rand()) / RAND_MAX * 10.0, ((double)rand()) / RAND_MAX * 10.0, ((double)rand()) / RAND_MAX * 10.0, ((double)rand()) / RAND_MAX), // Color
			DirectX::XMFLOAT2(1.0f, 1.0f),
			DirectX::XMFLOAT2(0.0f, 0.0f),
			XMFLOAT4(1.0, 1.0, 1.0, 0.0)); // Set to light sources 


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

	// Spheres 
	for (int i = 0; i < 2; i++)
	{
		std::shared_ptr<Material> basicMat = std::make_shared<Material>(
			pipelineState,
			DirectX::XMFLOAT4(((double)rand()) / RAND_MAX, ((double)rand()) / RAND_MAX, ((double)rand()) / RAND_MAX, ((double)rand()) / RAND_MAX), // Color
			DirectX::XMFLOAT2(1.0f, 1.0f),
			DirectX::XMFLOAT2(0.0f, 0.0f),
			XMFLOAT4(0.0, 0.0, 0.0, 0.0));


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

	// Cylinders 
	for (int i = 0; i < 2; i++)
	{
		std::shared_ptr<Material> basicMat = std::make_shared<Material>(
			pipelineState,
			DirectX::XMFLOAT4(((double)rand()) / RAND_MAX, ((double)rand()) / RAND_MAX, ((double)rand()) / RAND_MAX, ((double)rand()) / RAND_MAX), // Color
			DirectX::XMFLOAT2(1.0f, 1.0f),
			DirectX::XMFLOAT2(0.0f, 0.0f),
			XMFLOAT4(0.0, 0.0, 0.0, 0.0));


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
		entities.push_back(std::make_shared<Entity>(cylinder, basicMat));
		entities[entities.size() - 1]->GetTransform()->SetPosition(x, 0.0f, z);
	}

	
	// Helix 
	for (int i = 0; i < 5; i++)
	{
		std::shared_ptr<Material> basicMat = std::make_shared<Material>(
			pipelineState,
			DirectX::XMFLOAT4(((double)rand()) / RAND_MAX, ((double)rand()) / RAND_MAX, ((double)rand()) / RAND_MAX, ((double)rand()) / RAND_MAX), // Color
			DirectX::XMFLOAT2(1.0f, 1.0f),
			DirectX::XMFLOAT2(0.0f, 0.0f),
			XMFLOAT4(0.0, 0.0, 0.0, 0.0));


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

	

	// Ground
	std::shared_ptr<Material> mat = std::make_shared<Material>(
		pipelineState,
		DirectX::XMFLOAT4(((double)rand()) / RAND_MAX, ((double)rand()) / RAND_MAX, ((double)rand()) / RAND_MAX, 0.0f), // Color
		DirectX::XMFLOAT2(1.0f, 1.0f),
		DirectX::XMFLOAT2(0.0f, 0.0f),
		XMFLOAT4(0.0, 0.0, 0.0, 0.0));


	mat->AddTexture(dx12Helper.LoadTexture(FixPath(L"../../Assets/Textures/Foil002_4K-JPG_Color.jpg").c_str()), 0);
	mat->AddTexture(dx12Helper.LoadTexture(FixPath(L"../../Assets/Textures/Foil002_4K-JPG_NormalDX.jpg").c_str()), 1);
	mat->AddTexture(dx12Helper.LoadTexture(FixPath(L"../../Assets/Textures/Foil002_4K-JPG_Roughness.jpg").c_str()), 2);
	mat->AddTexture(dx12Helper.LoadTexture(FixPath(L"../../Assets/Textures/Foil002_4K-JPG_Metalness.jpg").c_str()), 3);

	mat->FinalizeMaterial();


	// Set meshes to entities 
	entities.push_back(std::make_shared<Entity>(cylinder, mat));
	entities[entities.size() - 1]->GetTransform()->SetPosition(0.0f, -3.0f, 0.0f);
	entities[entities.size() - 1]->GetTransform()->SetScale(1000.0f, 1.0f, 1000.0f);

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

	// Grab the helper
	DX12Helper& dx12Helper = DX12Helper::GetInstance();

	// Reset allocator associated with the current buffer
	// and set up the command list to use that allocator
	
	dx12Helper.WaitForGPU();
	commandAllocator->Reset();
	commandList->Reset(commandAllocator.Get(), 0);

	// Grab the current back buffer for this frame
	Microsoft::WRL::ComPtr<ID3D12Resource> currentBackBuffer = backBuffers[currentSwapBuffer];

	// Raytracing here!
	{
		// Update raytracing accel structure
		RaytracingHelper::GetInstance().CreateTopLevelAccelerationStructureForScene(entities);

		// Perform raytrace
		RaytracingHelper::GetInstance().Raytrace(camera, backBuffers[currentSwapBuffer]);
	}

	// Finish the frame
	{
		// Present the current back buffer
		bool vsyncNecessary = vsync || !deviceSupportsTearing || isFullscreen;
		swapChain->Present(
			vsyncNecessary ? 1 : 0,
			vsyncNecessary ? 0 : DXGI_PRESENT_ALLOW_TEARING);

		// Wait to proceed to the next frame until the associated buffer is ready
		currentSwapBuffer++;
		if (currentSwapBuffer >= numBackBuffers)
			currentSwapBuffer = 0;
	}
}

float Game::InverseLerp(float a, float b, float v)
{
	return (v - a) / (b - a);
}
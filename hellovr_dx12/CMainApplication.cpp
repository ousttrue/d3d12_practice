//========= Copyright Valve Corporation ============//
#include "CMainApplication.h"
#include "DX12RenderModel.h"
#include "SDLApplication.h"
#include "dprintf.h"
#include "Hmd.h"
#include <D3Dcompiler.h>
#include <stdio.h>
#include <string>
#include <cstdlib>

#include "lodepng.h"
#include "pathtools.h"

const std::string g_scene =
#include "shaders/scene.hlsl"
	;
const std::string g_companion =
#include "shaders/companion.hlsl"
	;
const std::string g_axes =
#include "shaders/axes.hlsl"
	;
const std::string g_rendermodel =
#include "shaders/rendermodel.hlsl"
	;

using Microsoft::WRL::ComPtr;

void ThreadSleep(unsigned long nMilliseconds)
{
	::Sleep(nMilliseconds);
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CMainApplication::CMainApplication(int msaa, float flSuperSampleScale)
	: m_nMSAASampleCount(msaa), m_flSuperSampleScale(flSuperSampleScale), m_sdl(new SDLApplication), m_hmd(new HMD),
	  m_iTrackedControllerCount(0), m_iTrackedControllerCount_Last(-1), m_iValidPoseCount(0), m_iValidPoseCount_Last(-1), m_strPoseClasses(""), m_bShowCubes(true), m_nFrameIndex(0), m_fenceEvent(NULL), m_nRTVDescriptorSize(0), m_nCBVSRVDescriptorSize(0), m_nDSVDescriptorSize(0)
{
	memset(m_pSceneConstantBufferData, 0, sizeof(m_pSceneConstantBufferData));
};

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CMainApplication::~CMainApplication()
{
	delete m_hmd;
	delete m_sdl;
	dprintf("Shutdown");
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CMainApplication::BInit(bool bDebugD3D12, int iSceneVolumeInit)
{
	// Loading the SteamVR Runtime
	if (!m_hmd->Initialize())
	{
		return false;
	}

	if (!m_sdl->Initialize(m_hmd->SystemName(), m_hmd->SerialNumber()))
	{
		return false;
	}

	// cube array
	m_iSceneVolumeWidth = iSceneVolumeInit;
	m_iSceneVolumeHeight = iSceneVolumeInit;
	m_iSceneVolumeDepth = iSceneVolumeInit;

	m_fScale = 0.3f;
	m_fScaleSpacing = 4.0f;

	m_uiVertcount = 0;

	if (!BInitD3D12(bDebugD3D12))
	{
		dprintf("%s - Unable to initialize D3D12!\n", __FUNCTION__);
		return false;
	}

	if (!BInitCompositor())
	{
		dprintf("%s - Failed to initialize VR Compositor!\n", __FUNCTION__);
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Initialize DX12. Returns true if DX12 has been successfully
//          initialized, false if shaders could not be created.
//          If failure occurred in a module other than shaders, the function
//          may return true or throw an error.
//-----------------------------------------------------------------------------
bool CMainApplication::BInitD3D12(bool bDebugD3D12)
{
	UINT nDXGIFactoryFlags = 0;

	// Debug layers if -dxdebug is specified
	if (bDebugD3D12)
	{
		ComPtr<ID3D12Debug> pDebugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugController))))
		{
			pDebugController->EnableDebugLayer();
			nDXGIFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}

	ComPtr<IDXGIFactory4> pFactory;
	if (FAILED(CreateDXGIFactory2(nDXGIFactoryFlags, IID_PPV_ARGS(&pFactory))))
	{
		dprintf("CreateDXGIFactory2 failed.\n");
		return false;
	}

	// Query OpenVR for the output adapter index
	int32_t nAdapterIndex = m_hmd->AdapterIndex();

	ComPtr<IDXGIAdapter1> pAdapter;
	if (FAILED(pFactory->EnumAdapters1(nAdapterIndex, &pAdapter)))
	{
		dprintf("Error enumerating DXGI adapter.\n");
	}
	DXGI_ADAPTER_DESC1 adapterDesc;
	pAdapter->GetDesc1(&adapterDesc);

	if (FAILED(D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_pDevice))))
	{
		dprintf("Failed to create D3D12 device with D3D12CreateDevice.\n");
		return false;
	}

	// Create the command queue
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	if (FAILED(m_pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pCommandQueue))))
	{
		printf("Failed to create D3D12 command queue.\n");
		return false;
	}

	// Create the swapchain
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = g_nFrameCount;
	swapChainDesc.Width = m_sdl->Width();
	swapChainDesc.Height = m_sdl->Height();
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	auto hWnd = reinterpret_cast<HWND>(m_sdl->HWND());
	ComPtr<IDXGISwapChain1> pSwapChain;
	if (FAILED(pFactory->CreateSwapChainForHwnd(m_pCommandQueue.Get(), hWnd, &swapChainDesc, nullptr, nullptr, &pSwapChain)))
	{
		dprintf("Failed to create DXGI swapchain.\n");
		return false;
	}

	pFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
	pSwapChain.As(&m_pSwapChain);
	m_nFrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	// Create descriptor heaps
	{
		m_nRTVDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		m_nDSVDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		m_nCBVSRVDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = NUM_RTVS;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		m_pDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_pRTVHeap));

		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = NUM_RTVS;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		m_pDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_pDSVHeap));

		D3D12_DESCRIPTOR_HEAP_DESC cbvSrvHeapDesc = {};
		cbvSrvHeapDesc.NumDescriptors = NUM_SRV_CBVS;
		cbvSrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		cbvSrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		m_pDevice->CreateDescriptorHeap(&cbvSrvHeapDesc, IID_PPV_ARGS(&m_pCBVSRVHeap));
	}

	// Create per-frame resources
	for (int nFrame = 0; nFrame < g_nFrameCount; nFrame++)
	{
		if (FAILED(m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pCommandAllocators[nFrame]))))
		{
			dprintf("Failed to create command allocators.\n");
			return false;
		}

		// Create swapchain render targets
		m_pSwapChain->GetBuffer(nFrame, IID_PPV_ARGS(&m_pSwapChainRenderTarget[nFrame]));

		// Create swapchain render target views
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart());
		rtvHandle.Offset(RTV_SWAPCHAIN0 + nFrame, m_nRTVDescriptorSize);
		m_pDevice->CreateRenderTargetView(m_pSwapChainRenderTarget[nFrame].Get(), nullptr, rtvHandle);
	}

	// Create constant buffer
	{
		m_pDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(1024 * 64),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_pSceneConstantBuffer));

		// Keep as persistently mapped buffer, store left eye in first 256 bytes, right eye in second
		UINT8 *pBuffer;
		CD3DX12_RANGE readRange(0, 0);
		m_pSceneConstantBuffer->Map(0, &readRange, reinterpret_cast<void **>(&pBuffer));
		// Left eye to first 256 bytes, right eye to second 256 bytes
		m_pSceneConstantBufferData[0] = pBuffer;
		m_pSceneConstantBufferData[1] = pBuffer + 256;

		// Left eye CBV
		CD3DX12_CPU_DESCRIPTOR_HANDLE cbvLeftEyeHandle(m_pCBVSRVHeap->GetCPUDescriptorHandleForHeapStart());
		cbvLeftEyeHandle.Offset(CBV_LEFT_EYE, m_nCBVSRVDescriptorSize);
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = m_pSceneConstantBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = (sizeof(Matrix4) + 255) & ~255; // Pad to 256 bytes
		m_pDevice->CreateConstantBufferView(&cbvDesc, cbvLeftEyeHandle);
		m_sceneConstantBufferView[0] = cbvLeftEyeHandle;

		// Right eye CBV
		CD3DX12_CPU_DESCRIPTOR_HANDLE cbvRightEyeHandle(m_pCBVSRVHeap->GetCPUDescriptorHandleForHeapStart());
		cbvRightEyeHandle.Offset(CBV_RIGHT_EYE, m_nCBVSRVDescriptorSize);
		cbvDesc.BufferLocation += 256;
		m_pDevice->CreateConstantBufferView(&cbvDesc, cbvRightEyeHandle);
		m_sceneConstantBufferView[1] = cbvRightEyeHandle;
	}

	// Create fence
	{
		memset(m_nFenceValues, 0, sizeof(m_nFenceValues));
		m_pDevice->CreateFence(m_nFenceValues[m_nFrameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence));
		m_nFenceValues[m_nFrameIndex]++;

		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	}

	if (!CreateAllShaders())
		return false;

	// Create command list
	m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocators[m_nFrameIndex].Get(), m_pScenePipelineState.Get(), IID_PPV_ARGS(&m_pCommandList));

	SetupTexturemaps();
	SetupScene();
	m_hmd->SetupCameras();
	SetupStereoRenderTargets();
	SetupCompanionWindow();
	SetupRenderModels();

	// Do any work that was queued up during loading
	m_pCommandList->Close();
	ID3D12CommandList *ppCommandLists[] = {m_pCommandList.Get()};
	m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Wait for it to finish
	m_pCommandQueue->Signal(m_pFence.Get(), m_nFenceValues[m_nFrameIndex]);
	m_pFence->SetEventOnCompletion(m_nFenceValues[m_nFrameIndex], m_fenceEvent);
	WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
	m_nFenceValues[m_nFrameIndex]++;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Initialize Compositor. Returns true if the compositor was
//          successfully initialized, false otherwise.
//-----------------------------------------------------------------------------
bool CMainApplication::BInitCompositor()
{
	vr::EVRInitError peError = vr::VRInitError_None;

	if (!vr::VRCompositor())
	{
		dprintf("Compositor initialization failed. See log file for details\n");
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CMainApplication::HandleInput()
{
	bool bRet = m_sdl->HandleInput(&m_bShowCubes);

	// Process SteamVR events
	vr::VREvent_t event;
	while (m_hmd->Hmd()->PollNextEvent(&event, sizeof(event)))
	{
		ProcessVREvent(event);
	}

	// Process SteamVR controller state
	m_hmd->Update();

	return bRet;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::RunMainLoop()
{
	bool bQuit = false;
	while (!bQuit)
	{
		bQuit = HandleInput();

		RenderFrame();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Processes a single VR event
//-----------------------------------------------------------------------------
void CMainApplication::ProcessVREvent(const vr::VREvent_t &event)
{
	switch (event.eventType)
	{
	case vr::VREvent_TrackedDeviceActivated:
	{
		SetupRenderModelForTrackedDevice(event.trackedDeviceIndex);
		dprintf("Device %u attached. Setting up render model.\n", event.trackedDeviceIndex);
	}
	break;
	case vr::VREvent_TrackedDeviceDeactivated:
	{
		dprintf("Device %u detached.\n", event.trackedDeviceIndex);
	}
	break;
	case vr::VREvent_TrackedDeviceUpdated:
	{
		dprintf("Device %u updated.\n", event.trackedDeviceIndex);
	}
	break;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::RenderFrame()
{
	if (m_hmd->Hmd())
	{
		m_pCommandAllocators[m_nFrameIndex]->Reset();

		m_pCommandList->Reset(m_pCommandAllocators[m_nFrameIndex].Get(), m_pScenePipelineState.Get());
		m_pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());

		ID3D12DescriptorHeap *ppHeaps[] = {m_pCBVSRVHeap.Get()};
		m_pCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

		UpdateControllerAxes();
		RenderStereoTargets();
		RenderCompanionWindow();

		m_pCommandList->Close();

		// Execute the command list.
		ID3D12CommandList *ppCommandLists[] = {m_pCommandList.Get()};
		m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		vr::VRTextureBounds_t bounds;
		bounds.uMin = 0.0f;
		bounds.uMax = 1.0f;
		bounds.vMin = 0.0f;
		bounds.vMax = 1.0f;

		vr::D3D12TextureData_t d3d12LeftEyeTexture = {m_leftEyeDesc.m_pTexture.Get(), m_pCommandQueue.Get(), 0};
		vr::Texture_t leftEyeTexture = {(void *)&d3d12LeftEyeTexture, vr::TextureType_DirectX12, vr::ColorSpace_Gamma};
		vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture, &bounds, vr::Submit_Default);

		vr::D3D12TextureData_t d3d12RightEyeTexture = {m_rightEyeDesc.m_pTexture.Get(), m_pCommandQueue.Get(), 0};
		vr::Texture_t rightEyeTexture = {(void *)&d3d12RightEyeTexture, vr::TextureType_DirectX12, vr::ColorSpace_Gamma};
		vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture, &bounds, vr::Submit_Default);
	}

	// Present
	m_pSwapChain->Present(0, 0);

	// Wait for completion
	{
		const UINT64 nCurrentFenceValue = m_nFenceValues[m_nFrameIndex];
		m_pCommandQueue->Signal(m_pFence.Get(), nCurrentFenceValue);

		m_nFrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();
		if (m_pFence->GetCompletedValue() < m_nFenceValues[m_nFrameIndex])
		{
			m_pFence->SetEventOnCompletion(m_nFenceValues[m_nFrameIndex], m_fenceEvent);
			WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
		}

		m_nFenceValues[m_nFrameIndex] = nCurrentFenceValue + 1;
	}

	// Spew out the controller and pose count whenever they change.
	if (m_iTrackedControllerCount != m_iTrackedControllerCount_Last || m_iValidPoseCount != m_iValidPoseCount_Last)
	{
		m_iValidPoseCount_Last = m_iValidPoseCount;
		m_iTrackedControllerCount_Last = m_iTrackedControllerCount;

		dprintf("PoseCount:%d(%s) Controllers:%d\n", m_iValidPoseCount, m_strPoseClasses.c_str(), m_iTrackedControllerCount);
	}

	m_hmd->UpdateHMDMatrixPose(m_iValidPoseCount, m_strPoseClasses);
}

//-----------------------------------------------------------------------------
// Purpose: Creates all the shaders used by HelloVR DX12
//-----------------------------------------------------------------------------
bool CMainApplication::CreateAllShaders()
{
	std::string sExecutableDirectory = Path_StripFilename(Path_GetExecutablePath());
	std::string strFullPath = Path_MakeAbsolute("../../hellovr_dx12/cube_texture.png", sExecutableDirectory);

	// Root signature
	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if (FAILED(m_pDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
		{
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		CD3DX12_DESCRIPTOR_RANGE1 ranges[2];
		CD3DX12_ROOT_PARAMETER1 rootParameters[2];

		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);
		rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);

		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, rootSignatureFlags);
		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error);
		m_pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature));
	}

	// Scene shader
	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;
		UINT compileFlags = 0;

		ComPtr<ID3DBlob> error;
		if (FAILED(D3DCompile(g_scene.data(), g_scene.size(), "scene", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, &error)))
		{
			dprintf("Failed compiling vertex shader 'scene':\n%s\n", (char *)error->GetBufferPointer());
			return false;
		}
		if (FAILED(D3DCompile(g_scene.data(), g_scene.size(), "scene", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, &error)))
		{
			dprintf("Failed compiling pixel shader 'scene':\n%s\n", (char *)error->GetBufferPointer());
			return false;
		}

		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
			{
				{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
				{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			};

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = {inputElementDescs, _countof(inputElementDescs)};
		psoDesc.pRootSignature = m_pRootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
		psoDesc.RasterizerState.MultisampleEnable = TRUE;
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleDesc.Count = m_nMSAASampleCount;
		psoDesc.SampleDesc.Quality = 0;
		if (FAILED(m_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pScenePipelineState))))
		{
			dprintf("Error creating D3D12 pipeline state.\n");
			return false;
		}
	}

	// Companion shader
	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;
		UINT compileFlags = 0;

		ComPtr<ID3DBlob> error;
		if (FAILED(D3DCompile(g_companion.data(), g_companion.size(), "companion", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, &error)))
		{
			dprintf("Failed compiling vertex shader 'companion':\n%s\n", (char *)error->GetBufferPointer());
			return false;
		}
		if (FAILED(D3DCompile(g_companion.data(), g_companion.size(), "companion", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, &error)))
		{
			dprintf("Failed compiling pixel shader 'comapanion':\n%s\n", (char *)error->GetBufferPointer());
			return false;
		}

		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
			{
				{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
				{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			};

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = {inputElementDescs, _countof(inputElementDescs)};
		psoDesc.pRootSignature = m_pRootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		if (FAILED(m_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pCompanionPipelineState))))
		{
			dprintf("Error creating D3D12 pipeline state.\n");
			return false;
		}
	}

	// Axes shader
	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;
		UINT compileFlags = 0;

		ComPtr<ID3DBlob> error;
		if (FAILED(D3DCompile(g_axes.data(), g_axes.size(), "axes", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, &error)))
		{
			dprintf("Failed compiling vertex shader 'axes':\n%s\n", (char *)error->GetBufferPointer());
			return false;
		}
		if (FAILED(D3DCompile(g_axes.data(), g_axes.size(), "axes", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, &error)))
		{
			dprintf("Failed compiling pixel shader 'axes':\n%s\n", (char *)error->GetBufferPointer());
			return false;
		}

		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
			{
				{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
				{"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			};

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = {inputElementDescs, _countof(inputElementDescs)};
		psoDesc.pRootSignature = m_pRootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
		psoDesc.RasterizerState.MultisampleEnable = TRUE;
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleDesc.Count = m_nMSAASampleCount;
		psoDesc.SampleDesc.Quality = 0;
		if (FAILED(m_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pAxesPipelineState))))
		{
			dprintf("Error creating D3D12 pipeline state.\n");
			return false;
		}
	}

	// Render Model shader
	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;
		UINT compileFlags = 0;

		ComPtr<ID3DBlob> error;
		if (FAILED(D3DCompile(g_rendermodel.data(), g_rendermodel.size(), "rendermodel", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, &error)))
		{
			dprintf("Failed compiling vertex shader 'rendermodel':\n%s\n", (char *)error->GetBufferPointer());
			return false;
		}
		if (FAILED(D3DCompile(g_rendermodel.data(), g_rendermodel.size(), "rendermodel", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, &error)))
		{
			dprintf("Failed compiling pixel shader 'rendermodel':\n%s\n", (char *)error->GetBufferPointer());
			return false;
		}

		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
			{
				{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
				{"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
				{"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			};

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = {inputElementDescs, _countof(inputElementDescs)};
		psoDesc.pRootSignature = m_pRootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
		psoDesc.RasterizerState.MultisampleEnable = TRUE;
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleDesc.Count = m_nMSAASampleCount;
		psoDesc.SampleDesc.Quality = 0;
		if (FAILED(m_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pRenderModelPipelineState))))
		{
			dprintf("Error creating D3D12 pipeline state.\n");
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CMainApplication::SetupTexturemaps()
{
	std::string sExecutableDirectory = Path_StripFilename(Path_GetExecutablePath());
	std::string strFullPath = Path_MakeAbsolute("../../hellovr_dx12/cube_texture.png", sExecutableDirectory);

	std::vector<unsigned char> imageRGBA;
	unsigned nImageWidth, nImageHeight;
	unsigned nError = lodepng::decode(imageRGBA, nImageWidth, nImageHeight, strFullPath.c_str());

	if (nError != 0)
		return false;

	// Store level 0
	std::vector<D3D12_SUBRESOURCE_DATA> mipLevelData;
	UINT8 *pBaseData = new UINT8[nImageWidth * nImageHeight * 4];
	memcpy(pBaseData, &imageRGBA[0], sizeof(UINT8) * nImageWidth * nImageHeight * 4);

	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = &pBaseData[0];
	textureData.RowPitch = nImageWidth * 4;
	textureData.SlicePitch = textureData.RowPitch * nImageHeight;
	mipLevelData.push_back(textureData);

	// Generate mipmaps for the image
	int nPrevImageIndex = 0;
	int nMipWidth = nImageWidth;
	int nMipHeight = nImageHeight;

	while (nMipWidth > 1 && nMipHeight > 1)
	{
		UINT8 *pNewImage;
		GenMipMapRGBA((UINT8 *)mipLevelData[nPrevImageIndex].pData, &pNewImage, nMipWidth, nMipHeight, &nMipWidth, &nMipHeight);

		D3D12_SUBRESOURCE_DATA mipData = {};
		mipData.pData = pNewImage;
		mipData.RowPitch = nMipWidth * 4;
		mipData.SlicePitch = textureData.RowPitch * nMipHeight;
		mipLevelData.push_back(mipData);

		nPrevImageIndex++;
	}

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = (UINT16)mipLevelData.size();
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.Width = nImageWidth;
	textureDesc.Height = nImageHeight;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	m_pDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
									   D3D12_HEAP_FLAG_NONE,
									   &textureDesc,
									   D3D12_RESOURCE_STATE_COPY_DEST,
									   nullptr,
									   IID_PPV_ARGS(&m_pTexture));

	// Create shader resource view
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_pCBVSRVHeap->GetCPUDescriptorHandleForHeapStart());
	srvHandle.Offset(SRV_TEXTURE_MAP, m_nCBVSRVDescriptorSize);
	m_pDevice->CreateShaderResourceView(m_pTexture.Get(), nullptr, srvHandle);
	m_textureShaderResourceView = srvHandle;

	const UINT64 nUploadBufferSize = GetRequiredIntermediateSize(m_pTexture.Get(), 0, textureDesc.MipLevels);

	// Create the GPU upload buffer.
	m_pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(nUploadBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_pTextureUploadHeap));

	UpdateSubresources(m_pCommandList.Get(), m_pTexture.Get(), m_pTextureUploadHeap.Get(), 0, 0, mipLevelData.size(), &mipLevelData[0]);
	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	// Free mip pointers
	for (size_t nMip = 0; nMip < mipLevelData.size(); nMip++)
	{
		delete[] mipLevelData[nMip].pData;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: generate next level mipmap for an RGBA image
//-----------------------------------------------------------------------------
void CMainApplication::GenMipMapRGBA(const UINT8 *pSrc, UINT8 **ppDst, int nSrcWidth, int nSrcHeight, int *pDstWidthOut, int *pDstHeightOut)
{
	*pDstWidthOut = nSrcWidth / 2;
	if (*pDstWidthOut <= 0)
	{
		*pDstWidthOut = 1;
	}
	*pDstHeightOut = nSrcHeight / 2;
	if (*pDstHeightOut <= 0)
	{
		*pDstHeightOut = 1;
	}

	*ppDst = new UINT8[4 * (*pDstWidthOut) * (*pDstHeightOut)];
	for (int y = 0; y < *pDstHeightOut; y++)
	{
		for (int x = 0; x < *pDstWidthOut; x++)
		{
			int nSrcIndex[4];
			float r = 0.0f;
			float g = 0.0f;
			float b = 0.0f;
			float a = 0.0f;

			nSrcIndex[0] = (((y * 2) * nSrcWidth) + (x * 2)) * 4;
			nSrcIndex[1] = (((y * 2) * nSrcWidth) + (x * 2 + 1)) * 4;
			nSrcIndex[2] = ((((y * 2) + 1) * nSrcWidth) + (x * 2)) * 4;
			nSrcIndex[3] = ((((y * 2) + 1) * nSrcWidth) + (x * 2 + 1)) * 4;

			// Sum all pixels
			for (int nSample = 0; nSample < 4; nSample++)
			{
				r += pSrc[nSrcIndex[nSample]];
				g += pSrc[nSrcIndex[nSample] + 1];
				b += pSrc[nSrcIndex[nSample] + 2];
				a += pSrc[nSrcIndex[nSample] + 3];
			}

			// Average results
			r /= 4.0;
			g /= 4.0;
			b /= 4.0;
			a /= 4.0;

			// Store resulting pixels
			(*ppDst)[(y * (*pDstWidthOut) + x) * 4] = (UINT8)(r);
			(*ppDst)[(y * (*pDstWidthOut) + x) * 4 + 1] = (UINT8)(g);
			(*ppDst)[(y * (*pDstWidthOut) + x) * 4 + 2] = (UINT8)(b);
			(*ppDst)[(y * (*pDstWidthOut) + x) * 4 + 3] = (UINT8)(a);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: create a sea of cubes
//-----------------------------------------------------------------------------
void CMainApplication::SetupScene()
{
	if (!m_hmd->Hmd())
		return;

	std::vector<float> vertdataarray;

	Matrix4 matScale;
	matScale.scale(m_fScale, m_fScale, m_fScale);
	Matrix4 matTransform;
	matTransform.translate(
		-((float)m_iSceneVolumeWidth * m_fScaleSpacing) / 2.f,
		-((float)m_iSceneVolumeHeight * m_fScaleSpacing) / 2.f,
		-((float)m_iSceneVolumeDepth * m_fScaleSpacing) / 2.f);

	Matrix4 mat = matScale * matTransform;

	for (int z = 0; z < m_iSceneVolumeDepth; z++)
	{
		for (int y = 0; y < m_iSceneVolumeHeight; y++)
		{
			for (int x = 0; x < m_iSceneVolumeWidth; x++)
			{
				AddCubeToScene(mat, vertdataarray);
				mat = mat * Matrix4().translate(m_fScaleSpacing, 0, 0);
			}
			mat = mat * Matrix4().translate(-((float)m_iSceneVolumeWidth) * m_fScaleSpacing, m_fScaleSpacing, 0);
		}
		mat = mat * Matrix4().translate(0, -((float)m_iSceneVolumeHeight) * m_fScaleSpacing, m_fScaleSpacing);
	}
	m_uiVertcount = vertdataarray.size() / 5;

	m_pDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
									   D3D12_HEAP_FLAG_NONE,
									   &CD3DX12_RESOURCE_DESC::Buffer(sizeof(float) * vertdataarray.size()),
									   D3D12_RESOURCE_STATE_GENERIC_READ,
									   nullptr,
									   IID_PPV_ARGS(&m_pSceneVertexBuffer));

	UINT8 *pMappedBuffer;
	CD3DX12_RANGE readRange(0, 0);
	m_pSceneVertexBuffer->Map(0, &readRange, reinterpret_cast<void **>(&pMappedBuffer));
	memcpy(pMappedBuffer, &vertdataarray[0], sizeof(float) * vertdataarray.size());
	m_pSceneVertexBuffer->Unmap(0, nullptr);

	m_sceneVertexBufferView.BufferLocation = m_pSceneVertexBuffer->GetGPUVirtualAddress();
	m_sceneVertexBufferView.StrideInBytes = sizeof(VertexDataScene);
	m_sceneVertexBufferView.SizeInBytes = sizeof(float) * vertdataarray.size();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::AddCubeVertex(float fl0, float fl1, float fl2, float fl3, float fl4, std::vector<float> &vertdata)
{
	vertdata.push_back(fl0);
	vertdata.push_back(fl1);
	vertdata.push_back(fl2);
	vertdata.push_back(fl3);
	vertdata.push_back(fl4);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::AddCubeToScene(Matrix4 mat, std::vector<float> &vertdata)
{
	// Matrix4 mat( outermat.data() );

	Vector4 A = mat * Vector4(0, 0, 0, 1);
	Vector4 B = mat * Vector4(1, 0, 0, 1);
	Vector4 C = mat * Vector4(1, 1, 0, 1);
	Vector4 D = mat * Vector4(0, 1, 0, 1);
	Vector4 E = mat * Vector4(0, 0, 1, 1);
	Vector4 F = mat * Vector4(1, 0, 1, 1);
	Vector4 G = mat * Vector4(1, 1, 1, 1);
	Vector4 H = mat * Vector4(0, 1, 1, 1);

	// triangles instead of quads
	AddCubeVertex(E.x, E.y, E.z, 0, 1, vertdata); //Front
	AddCubeVertex(F.x, F.y, F.z, 1, 1, vertdata);
	AddCubeVertex(G.x, G.y, G.z, 1, 0, vertdata);
	AddCubeVertex(G.x, G.y, G.z, 1, 0, vertdata);
	AddCubeVertex(H.x, H.y, H.z, 0, 0, vertdata);
	AddCubeVertex(E.x, E.y, E.z, 0, 1, vertdata);

	AddCubeVertex(B.x, B.y, B.z, 0, 1, vertdata); //Back
	AddCubeVertex(A.x, A.y, A.z, 1, 1, vertdata);
	AddCubeVertex(D.x, D.y, D.z, 1, 0, vertdata);
	AddCubeVertex(D.x, D.y, D.z, 1, 0, vertdata);
	AddCubeVertex(C.x, C.y, C.z, 0, 0, vertdata);
	AddCubeVertex(B.x, B.y, B.z, 0, 1, vertdata);

	AddCubeVertex(H.x, H.y, H.z, 0, 1, vertdata); //Top
	AddCubeVertex(G.x, G.y, G.z, 1, 1, vertdata);
	AddCubeVertex(C.x, C.y, C.z, 1, 0, vertdata);
	AddCubeVertex(C.x, C.y, C.z, 1, 0, vertdata);
	AddCubeVertex(D.x, D.y, D.z, 0, 0, vertdata);
	AddCubeVertex(H.x, H.y, H.z, 0, 1, vertdata);

	AddCubeVertex(A.x, A.y, A.z, 0, 1, vertdata); //Bottom
	AddCubeVertex(B.x, B.y, B.z, 1, 1, vertdata);
	AddCubeVertex(F.x, F.y, F.z, 1, 0, vertdata);
	AddCubeVertex(F.x, F.y, F.z, 1, 0, vertdata);
	AddCubeVertex(E.x, E.y, E.z, 0, 0, vertdata);
	AddCubeVertex(A.x, A.y, A.z, 0, 1, vertdata);

	AddCubeVertex(A.x, A.y, A.z, 0, 1, vertdata); //Left
	AddCubeVertex(E.x, E.y, E.z, 1, 1, vertdata);
	AddCubeVertex(H.x, H.y, H.z, 1, 0, vertdata);
	AddCubeVertex(H.x, H.y, H.z, 1, 0, vertdata);
	AddCubeVertex(D.x, D.y, D.z, 0, 0, vertdata);
	AddCubeVertex(A.x, A.y, A.z, 0, 1, vertdata);

	AddCubeVertex(F.x, F.y, F.z, 0, 1, vertdata); //Right
	AddCubeVertex(B.x, B.y, B.z, 1, 1, vertdata);
	AddCubeVertex(C.x, C.y, C.z, 1, 0, vertdata);
	AddCubeVertex(C.x, C.y, C.z, 1, 0, vertdata);
	AddCubeVertex(G.x, G.y, G.z, 0, 0, vertdata);
	AddCubeVertex(F.x, F.y, F.z, 0, 1, vertdata);
}

//-----------------------------------------------------------------------------
// Purpose: Update the vertex data for the controllers as X/Y/Z lines
//-----------------------------------------------------------------------------
void CMainApplication::UpdateControllerAxes()
{
	// Don't attempt to update controllers if input is not available
	if (!m_hmd->Hmd()->IsInputAvailable())
		return;

	std::vector<float> vertdataarray;

	m_uiControllerVertcount = 0;
	m_iTrackedControllerCount = 0;

	for (vr::TrackedDeviceIndex_t unTrackedDevice = vr::k_unTrackedDeviceIndex_Hmd + 1; unTrackedDevice < vr::k_unMaxTrackedDeviceCount; ++unTrackedDevice)
	{
		if (!m_hmd->Hmd()->IsTrackedDeviceConnected(unTrackedDevice))
			continue;

		if (m_hmd->Hmd()->GetTrackedDeviceClass(unTrackedDevice) != vr::TrackedDeviceClass_Controller)
			continue;

		m_iTrackedControllerCount += 1;

		if (!m_hmd->PoseIsValid(unTrackedDevice))
			continue;

		const Matrix4 &mat = m_hmd->DevicePose(unTrackedDevice);

		Vector4 center = mat * Vector4(0, 0, 0, 1);

		for (int i = 0; i < 3; ++i)
		{
			Vector3 color(0, 0, 0);
			Vector4 point(0, 0, 0, 1);
			point[i] += 0.05f; // offset in X, Y, Z
			color[i] = 1.0;	// R, G, B
			point = mat * point;
			vertdataarray.push_back(center.x);
			vertdataarray.push_back(center.y);
			vertdataarray.push_back(center.z);

			vertdataarray.push_back(color.x);
			vertdataarray.push_back(color.y);
			vertdataarray.push_back(color.z);

			vertdataarray.push_back(point.x);
			vertdataarray.push_back(point.y);
			vertdataarray.push_back(point.z);

			vertdataarray.push_back(color.x);
			vertdataarray.push_back(color.y);
			vertdataarray.push_back(color.z);

			m_uiControllerVertcount += 2;
		}

		Vector4 start = mat * Vector4(0, 0, -0.02f, 1);
		Vector4 end = mat * Vector4(0, 0, -39.f, 1);
		Vector3 color(.92f, .92f, .71f);

		vertdataarray.push_back(start.x);
		vertdataarray.push_back(start.y);
		vertdataarray.push_back(start.z);
		vertdataarray.push_back(color.x);
		vertdataarray.push_back(color.y);
		vertdataarray.push_back(color.z);

		vertdataarray.push_back(end.x);
		vertdataarray.push_back(end.y);
		vertdataarray.push_back(end.z);
		vertdataarray.push_back(color.x);
		vertdataarray.push_back(color.y);
		vertdataarray.push_back(color.z);
		m_uiControllerVertcount += 2;
	}

	// Setup the VB the first time through.
	if (m_pControllerAxisVertexBuffer == nullptr && vertdataarray.size() > 0)
	{
		// Make big enough to hold up to the max number
		size_t nSize = sizeof(float) * vertdataarray.size();
		nSize *= vr::k_unMaxTrackedDeviceCount;

		m_pDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(nSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_pControllerAxisVertexBuffer));

		m_controllerAxisVertexBufferView.BufferLocation = m_pControllerAxisVertexBuffer->GetGPUVirtualAddress();
		m_controllerAxisVertexBufferView.StrideInBytes = sizeof(float) * 6;
		m_controllerAxisVertexBufferView.SizeInBytes = sizeof(float) * vertdataarray.size();
	}

	// Update the VB data
	if (m_pControllerAxisVertexBuffer && vertdataarray.size() > 0)
	{
		UINT8 *pMappedBuffer;
		CD3DX12_RANGE readRange(0, 0);
		m_pControllerAxisVertexBuffer->Map(0, &readRange, reinterpret_cast<void **>(&pMappedBuffer));
		memcpy(pMappedBuffer, &vertdataarray[0], sizeof(float) * vertdataarray.size());
		m_pControllerAxisVertexBuffer->Unmap(0, nullptr);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Creates a frame buffer. Returns true if the buffer was set up.
//          Returns false if the setup failed.
//-----------------------------------------------------------------------------
bool CMainApplication::CreateFrameBuffer(int nWidth, int nHeight, FramebufferDesc &framebufferDesc, RTVIndex_t nRTVIndex)
{
	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	textureDesc.Width = nWidth;
	textureDesc.Height = nHeight;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = m_nMSAASampleCount;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	const float clearColor[] = {0.0f, 0.0f, 0.0f, 1.0f};

	// Create color target
	m_pDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
									   D3D12_HEAP_FLAG_NONE,
									   &textureDesc,
									   D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
									   &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, clearColor),
									   IID_PPV_ARGS(&framebufferDesc.m_pTexture));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart());
	rtvHandle.Offset(nRTVIndex, m_nRTVDescriptorSize);
	m_pDevice->CreateRenderTargetView(framebufferDesc.m_pTexture.Get(), nullptr, rtvHandle);
	framebufferDesc.m_renderTargetViewHandle = rtvHandle;

	// Create shader resource view
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_pCBVSRVHeap->GetCPUDescriptorHandleForHeapStart());
	srvHandle.Offset(SRV_LEFT_EYE + nRTVIndex, m_nCBVSRVDescriptorSize);
	m_pDevice->CreateShaderResourceView(framebufferDesc.m_pTexture.Get(), nullptr, srvHandle);

	// Create depth
	D3D12_RESOURCE_DESC depthDesc = textureDesc;
	depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	m_pDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
									   D3D12_HEAP_FLAG_NONE,
									   &depthDesc,
									   D3D12_RESOURCE_STATE_DEPTH_WRITE,
									   &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.0f, 0),
									   IID_PPV_ARGS(&framebufferDesc.m_pDepthStencil));

	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_pDSVHeap->GetCPUDescriptorHandleForHeapStart());
	dsvHandle.Offset(nRTVIndex, m_nDSVDescriptorSize);
	m_pDevice->CreateDepthStencilView(framebufferDesc.m_pDepthStencil.Get(), nullptr, dsvHandle);
	framebufferDesc.m_depthStencilViewHandle = dsvHandle;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CMainApplication::SetupStereoRenderTargets()
{
	if (!m_hmd->Hmd())
		return false;

	m_hmd->Hmd()->GetRecommendedRenderTargetSize(&m_nRenderWidth, &m_nRenderHeight);
	m_nRenderWidth = (uint32_t)(m_flSuperSampleScale * (float)m_nRenderWidth);
	m_nRenderHeight = (uint32_t)(m_flSuperSampleScale * (float)m_nRenderHeight);

	CreateFrameBuffer(m_nRenderWidth, m_nRenderHeight, m_leftEyeDesc, RTV_LEFT_EYE);
	CreateFrameBuffer(m_nRenderWidth, m_nRenderHeight, m_rightEyeDesc, RTV_RIGHT_EYE);
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::SetupCompanionWindow()
{
	if (!m_hmd->Hmd())
		return;

	std::vector<VertexDataWindow> vVerts;

	// left eye verts
	vVerts.push_back(VertexDataWindow(Vector2(-1, -1), Vector2(0, 1)));
	vVerts.push_back(VertexDataWindow(Vector2(0, -1), Vector2(1, 1)));
	vVerts.push_back(VertexDataWindow(Vector2(-1, 1), Vector2(0, 0)));
	vVerts.push_back(VertexDataWindow(Vector2(0, 1), Vector2(1, 0)));

	// right eye verts
	vVerts.push_back(VertexDataWindow(Vector2(0, -1), Vector2(0, 1)));
	vVerts.push_back(VertexDataWindow(Vector2(1, -1), Vector2(1, 1)));
	vVerts.push_back(VertexDataWindow(Vector2(0, 1), Vector2(0, 0)));
	vVerts.push_back(VertexDataWindow(Vector2(1, 1), Vector2(1, 0)));

	m_pDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
									   D3D12_HEAP_FLAG_NONE,
									   &CD3DX12_RESOURCE_DESC::Buffer(sizeof(VertexDataWindow) * vVerts.size()),
									   D3D12_RESOURCE_STATE_GENERIC_READ,
									   nullptr,
									   IID_PPV_ARGS(&m_pCompanionWindowVertexBuffer));

	UINT8 *pMappedBuffer;
	CD3DX12_RANGE readRange(0, 0);
	m_pCompanionWindowVertexBuffer->Map(0, &readRange, reinterpret_cast<void **>(&pMappedBuffer));
	memcpy(pMappedBuffer, &vVerts[0], sizeof(VertexDataWindow) * vVerts.size());
	m_pCompanionWindowVertexBuffer->Unmap(0, nullptr);

	m_companionWindowVertexBufferView.BufferLocation = m_pCompanionWindowVertexBuffer->GetGPUVirtualAddress();
	m_companionWindowVertexBufferView.StrideInBytes = sizeof(VertexDataWindow);
	m_companionWindowVertexBufferView.SizeInBytes = sizeof(VertexDataWindow) * vVerts.size();

	UINT16 vIndices[] = {0, 1, 3, 0, 3, 2, 4, 5, 7, 4, 7, 6};
	m_uiCompanionWindowIndexSize = _countof(vIndices);

	m_pDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
									   D3D12_HEAP_FLAG_NONE,
									   &CD3DX12_RESOURCE_DESC::Buffer(sizeof(vIndices)),
									   D3D12_RESOURCE_STATE_GENERIC_READ,
									   nullptr,
									   IID_PPV_ARGS(&m_pCompanionWindowIndexBuffer));

	m_pCompanionWindowIndexBuffer->Map(0, &readRange, reinterpret_cast<void **>(&pMappedBuffer));
	memcpy(pMappedBuffer, &vIndices[0], sizeof(vIndices));
	m_pCompanionWindowIndexBuffer->Unmap(0, nullptr);

	m_companionWindowIndexBufferView.BufferLocation = m_pCompanionWindowIndexBuffer->GetGPUVirtualAddress();
	m_companionWindowIndexBufferView.Format = DXGI_FORMAT_R16_UINT;
	m_companionWindowIndexBufferView.SizeInBytes = sizeof(vIndices);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::RenderStereoTargets()
{
	D3D12_VIEWPORT viewport = {0.0f, 0.0f, (FLOAT)m_nRenderWidth, (FLOAT)m_nRenderHeight, 0.0f, 1.0f};
	D3D12_RECT scissor = {0, 0, (LONG)m_nRenderWidth, (LONG)m_nRenderHeight};

	m_pCommandList->RSSetViewports(1, &viewport);
	m_pCommandList->RSSetScissorRects(1, &scissor);

	//----------//
	// Left Eye //
	//----------//
	// Transition to RENDER_TARGET
	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_leftEyeDesc.m_pTexture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
	m_pCommandList->OMSetRenderTargets(1, &m_leftEyeDesc.m_renderTargetViewHandle, FALSE, &m_leftEyeDesc.m_depthStencilViewHandle);

	const float clearColor[] = {0.0f, 0.0f, 0.0f, 1.0f};
	m_pCommandList->ClearRenderTargetView(m_leftEyeDesc.m_renderTargetViewHandle, clearColor, 0, nullptr);
	m_pCommandList->ClearDepthStencilView(m_leftEyeDesc.m_depthStencilViewHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0, 0, 0, nullptr);

	RenderScene(vr::Eye_Left);

	// Transition to SHADER_RESOURCE to submit to SteamVR
	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_leftEyeDesc.m_pTexture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	//-----------//
	// Right Eye //
	//-----------//
	// Transition to RENDER_TARGET
	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_rightEyeDesc.m_pTexture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
	m_pCommandList->OMSetRenderTargets(1, &m_rightEyeDesc.m_renderTargetViewHandle, FALSE, &m_rightEyeDesc.m_depthStencilViewHandle);

	m_pCommandList->ClearRenderTargetView(m_rightEyeDesc.m_renderTargetViewHandle, clearColor, 0, nullptr);
	m_pCommandList->ClearDepthStencilView(m_rightEyeDesc.m_depthStencilViewHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0, 0, 0, nullptr);

	RenderScene(vr::Eye_Right);

	// Transition to SHADER_RESOURCE to submit to SteamVR
	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_rightEyeDesc.m_pTexture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
}

//-----------------------------------------------------------------------------
// Purpose: Renders a scene with respect to nEye.
//-----------------------------------------------------------------------------
void CMainApplication::RenderScene(vr::Hmd_Eye nEye)
{
	if (m_bShowCubes)
	{
		m_pCommandList->SetPipelineState(m_pScenePipelineState.Get());

		// Select the CBV (left or right eye)
		CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(m_pCBVSRVHeap->GetGPUDescriptorHandleForHeapStart());
		cbvHandle.Offset(nEye, m_nCBVSRVDescriptorSize);
		m_pCommandList->SetGraphicsRootDescriptorTable(0, cbvHandle);

		// SRV is just after the left eye
		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(m_pCBVSRVHeap->GetGPUDescriptorHandleForHeapStart());
		srvHandle.Offset(SRV_TEXTURE_MAP, m_nCBVSRVDescriptorSize);
		m_pCommandList->SetGraphicsRootDescriptorTable(1, srvHandle);

		// Update the persistently mapped pointer to the CB data with the latest matrix
		memcpy(m_pSceneConstantBufferData[nEye], m_hmd->GetCurrentViewProjectionMatrix(nEye).get(), sizeof(Matrix4));

		// Draw
		m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_pCommandList->IASetVertexBuffers(0, 1, &m_sceneVertexBufferView);
		m_pCommandList->DrawInstanced(m_uiVertcount, 1, 0, 0);
	}

	bool bIsInputAvailable = m_hmd->Hmd()->IsInputAvailable();

	if (bIsInputAvailable && m_pControllerAxisVertexBuffer)
	{
		// draw the controller axis lines
		m_pCommandList->SetPipelineState(m_pAxesPipelineState.Get());

		m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		m_pCommandList->IASetVertexBuffers(0, 1, &m_controllerAxisVertexBufferView);
		m_pCommandList->DrawInstanced(m_uiControllerVertcount, 1, 0, 0);
	}

	// ----- Render Model rendering -----
	m_pCommandList->SetPipelineState(m_pRenderModelPipelineState.Get());
	for (uint32_t unTrackedDevice = 0; unTrackedDevice < vr::k_unMaxTrackedDeviceCount; unTrackedDevice++)
	{
		if (!m_rTrackedDeviceToRenderModel[unTrackedDevice])
			continue;

		if (!m_hmd->IsVisible(unTrackedDevice))
			continue;

		if (!m_hmd->PoseIsValid(unTrackedDevice))
			continue;

		if (!bIsInputAvailable && m_hmd->Hmd()->GetTrackedDeviceClass(unTrackedDevice) == vr::TrackedDeviceClass_Controller)
			continue;

		Matrix4 matMVP = m_hmd->GetCurrentViewProjectionMatrix(nEye) * m_hmd->DevicePose(unTrackedDevice);

		m_rTrackedDeviceToRenderModel[unTrackedDevice]->Draw(nEye, m_pCommandList.Get(), m_nCBVSRVDescriptorSize, matMVP);
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::RenderCompanionWindow()
{
	m_pCommandList->SetPipelineState(m_pCompanionPipelineState.Get());

	// Transition swapchain image to RENDER_TARGET
	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pSwapChainRenderTarget[m_nFrameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Bind current swapchain image
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart());
	rtvHandle.Offset(RTV_SWAPCHAIN0 + m_nFrameIndex, m_nRTVDescriptorSize);
	m_pCommandList->OMSetRenderTargets(1, &rtvHandle, 0, nullptr);

	auto w = m_sdl->Width();
	auto h = m_sdl->Height();
	D3D12_VIEWPORT viewport = {0.0f, 0.0f, (FLOAT)w, (FLOAT)h, 0.0f, 1.0f};
	D3D12_RECT scissor = {0, 0, (LONG)w, (LONG)h};

	m_pCommandList->RSSetViewports(1, &viewport);
	m_pCommandList->RSSetScissorRects(1, &scissor);

	// render left eye (first half of index array)
	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandleLeftEye(m_pCBVSRVHeap->GetGPUDescriptorHandleForHeapStart());
	srvHandleLeftEye.Offset(SRV_LEFT_EYE, m_nCBVSRVDescriptorSize);
	m_pCommandList->SetGraphicsRootDescriptorTable(1, srvHandleLeftEye);

	m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pCommandList->IASetVertexBuffers(0, 1, &m_companionWindowVertexBufferView);
	m_pCommandList->IASetIndexBuffer(&m_companionWindowIndexBufferView);
	m_pCommandList->DrawIndexedInstanced(m_uiCompanionWindowIndexSize / 2, 1, 0, 0, 0);

	// render right eye (second half of index array)
	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandleRightEye(m_pCBVSRVHeap->GetGPUDescriptorHandleForHeapStart());
	srvHandleRightEye.Offset(SRV_RIGHT_EYE, m_nCBVSRVDescriptorSize);
	m_pCommandList->SetGraphicsRootDescriptorTable(1, srvHandleRightEye);
	m_pCommandList->DrawIndexedInstanced(m_uiCompanionWindowIndexSize / 2, 1, (m_uiCompanionWindowIndexSize / 2), 0, 0);

	// Transition swapchain image to PRESENT
	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pSwapChainRenderTarget[m_nFrameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
}

//-----------------------------------------------------------------------------
// Purpose: Finds a render model we've already loaded or loads a new one
//-----------------------------------------------------------------------------
DX12RenderModel *CMainApplication::FindOrLoadRenderModel(vr::TrackedDeviceIndex_t unTrackedDeviceIndex, const char *pchRenderModelName)
{
	DX12RenderModel *pRenderModel = NULL;

	// load the model if we didn't find one
	if (!pRenderModel)
	{
		vr::RenderModel_t *pModel;
		vr::EVRRenderModelError error;
		while (1)
		{
			error = vr::VRRenderModels()->LoadRenderModel_Async(pchRenderModelName, &pModel);
			if (error != vr::VRRenderModelError_Loading)
				break;

			ThreadSleep(1);
		}

		if (error != vr::VRRenderModelError_None)
		{
			dprintf("Unable to load render model %s - %s\n", pchRenderModelName, vr::VRRenderModels()->GetRenderModelErrorNameFromEnum(error));
			return NULL; // move on to the next tracked device
		}

		vr::RenderModel_TextureMap_t *pTexture;
		while (1)
		{
			error = vr::VRRenderModels()->LoadTexture_Async(pModel->diffuseTextureId, &pTexture);
			if (error != vr::VRRenderModelError_Loading)
				break;

			ThreadSleep(1);
		}

		if (error != vr::VRRenderModelError_None)
		{
			dprintf("Unable to load render texture id:%d for render model %s\n", pModel->diffuseTextureId, pchRenderModelName);
			vr::VRRenderModels()->FreeRenderModel(pModel);
			return NULL; // move on to the next tracked device
		}

		pRenderModel = new DX12RenderModel(pchRenderModelName);
		if (!pRenderModel->BInit(m_pDevice.Get(), m_pCommandList.Get(), m_pCBVSRVHeap.Get(), unTrackedDeviceIndex, *pModel, *pTexture))
		{
			dprintf("Unable to create D3D12 model from render model %s\n", pchRenderModelName);
			delete pRenderModel;
			pRenderModel = NULL;
		}
		vr::VRRenderModels()->FreeRenderModel(pModel);
		vr::VRRenderModels()->FreeTexture(pTexture);
	}

	return pRenderModel;
}

//-----------------------------------------------------------------------------
// Purpose: Create/destroy D3D12 a Render Model for a single tracked device
//-----------------------------------------------------------------------------
void CMainApplication::SetupRenderModelForTrackedDevice(vr::TrackedDeviceIndex_t unTrackedDeviceIndex)
{
	if (unTrackedDeviceIndex >= vr::k_unMaxTrackedDeviceCount)
		return;

	// try to find a model we've already set up
	auto sRenderModelName = m_hmd->RenderModelName(unTrackedDeviceIndex);
	DX12RenderModel *pRenderModel = FindOrLoadRenderModel(unTrackedDeviceIndex, sRenderModelName.c_str());
	if (!pRenderModel)
	{
		std::string sTrackingSystemName = m_hmd->SystemName(unTrackedDeviceIndex);
		dprintf("Unable to load render model for tracked device %d (%s.%s)", unTrackedDeviceIndex, sTrackingSystemName.c_str(), sRenderModelName.c_str());
	}
	else
	{
		m_rTrackedDeviceToRenderModel[unTrackedDeviceIndex] = pRenderModel;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create/destroy D3D12 Render Models
//-----------------------------------------------------------------------------
void CMainApplication::SetupRenderModels()
{
	memset(m_rTrackedDeviceToRenderModel, 0, sizeof(m_rTrackedDeviceToRenderModel));

	if (!m_hmd->Hmd())
		return;

	for (uint32_t unTrackedDevice = vr::k_unTrackedDeviceIndex_Hmd + 1; unTrackedDevice < vr::k_unMaxTrackedDeviceCount; unTrackedDevice++)
	{
		if (!m_hmd->Hmd()->IsTrackedDeviceConnected(unTrackedDevice))
			continue;

		SetupRenderModelForTrackedDevice(unTrackedDevice);
	}
}
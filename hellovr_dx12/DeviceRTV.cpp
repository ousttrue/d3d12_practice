#include "DeviceRTV.h"

template <class T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

DeviceRTV::DeviceRTV(int frameCount)
	: m_frames(frameCount)
{
}

ComPtr<ID3D12Device> DeviceRTV::CreateDevice(const ComPtr<IDXGIFactory4> &factory, int adapterIndex)
{
	ComPtr<IDXGIAdapter1> adapter;
	if (FAILED(factory->EnumAdapters1(adapterIndex, &adapter)))
	{
		return nullptr;
	}

	if (FAILED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_pDevice))))
	{
		return nullptr;
	}

	D3D12_COMMAND_QUEUE_DESC queueDesc{
		.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
		.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
	};
	if (FAILED(m_pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pCommandQueue))))
	{
		return nullptr;
	}

	// Create fence
	{
		// memset(m_nFenceValues, 0, sizeof(m_nFenceValues));
		m_pDevice->CreateFence(CurrentFrame().m_nFenceValues++, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence));
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	}

	m_nRTVDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		.NumDescriptors = (int)RTVIndex_t::NUM_RTVS,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
	};
	m_pDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_pRTVHeap));

	m_nDSVDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
		.NumDescriptors = (int)RTVIndex_t::NUM_RTVS,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
	};
	m_pDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_pDSVHeap));

	return m_pDevice;
}

bool DeviceRTV::CreateSwapchain(const ComPtr<IDXGIFactory4> &pFactory, int width, int height, HWND hWnd)
{
	// Create the swapchain
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = (UINT)m_frames.size();
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> pSwapChain;
	if (FAILED(pFactory->CreateSwapChainForHwnd(m_pCommandQueue.Get(), hWnd, &swapChainDesc, nullptr, nullptr, &pSwapChain)))
	{
		return false;
	}

	pFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
	pSwapChain.As(&m_pSwapChain);
	m_nFrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	return true;
}

void DeviceRTV::Execute(const ComPtr<ID3D12CommandList> &commandList)
{
	m_pCommandQueue->ExecuteCommandLists(1, commandList.GetAddressOf());
}

void DeviceRTV::Sync()
{
	const UINT64 value = CurrentFrame().m_nFenceValues++;
	m_pCommandQueue->Signal(m_pFence.Get(), value);
	m_nFrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();
	if (m_pFence->GetCompletedValue() < value)
	{
		m_pFence->SetEventOnCompletion(value, m_fenceEvent);
		WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
	}
}

void DeviceRTV::Present()
{
	m_pSwapChain->Present(0, 0);
}

void DeviceRTV::SetupFrameResources(const ComPtr<ID3D12PipelineState> &pipelineState)
{
	for (int nFrame = 0; nFrame < m_frames.size(); nFrame++)
	{
		// Create per-frame resources
		auto &frame = m_frames[nFrame];
		if (FAILED(m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frame.m_pCommandAllocator))))
		{
			// dprintf("Failed to create command allocators.\n");
			return;
		}

		// Create swapchain render targets
		m_pSwapChain->GetBuffer(nFrame, IID_PPV_ARGS(&frame.m_pSwapChainRenderTarget));

		// Create swapchain render target views
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart());
		rtvHandle.Offset((int)RTVIndex_t::RTV_SWAPCHAIN0 + nFrame, m_nRTVDescriptorSize);
		m_pDevice->CreateRenderTargetView(frame.m_pSwapChainRenderTarget.Get(), nullptr, rtvHandle);
		m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
									 frame.m_pCommandAllocator.Get(),
									 pipelineState.Get(),
									 IID_PPV_ARGS(&frame.m_pCommandList));
	}
}

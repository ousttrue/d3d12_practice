#pragma once

class D3D
{
	// d3d12
	ComPtr<ID3D12Device> m_pDevice;
	ComPtr<ID3D12CommandQueue> m_pCommandQueue;
	HANDLE m_fenceEvent = NULL;
	ComPtr<ID3D12Fence> m_pFence;
	std::vector<UINT64> m_nFenceValues;

	// swapchain
	ComPtr<IDXGISwapChain3> m_pSwapChain;
	UINT m_nFrameIndex = 0;

public:
	D3D(int frameCount)
		: m_nFenceValues(frameCount, 0)
	{
	}

	const ComPtr<ID3D12Device> &Device() const { return m_pDevice; }
	const ComPtr<ID3D12CommandQueue> &Queue() const { return m_pCommandQueue; }
	const ComPtr<IDXGISwapChain3> &Swapchain() const { return m_pSwapChain; }
	UINT FrameIndex() const { return m_nFrameIndex; }

	ComPtr<ID3D12Device> CreateDevice(const ComPtr<IDXGIFactory4> &factory, int adapterIndex)
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
			m_pDevice->CreateFence(m_nFenceValues[m_nFrameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence));
			m_nFenceValues[m_nFrameIndex]++;

			m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		}

		return m_pDevice;
	}

	bool CreateSwapchain(const ComPtr<IDXGIFactory4> &pFactory, int width, int height, HWND hWnd)
	{
		// Create the swapchain
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = (UINT)m_nFenceValues.size();
		swapChainDesc.Width = width;
		swapChainDesc.Height = height;
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.SampleDesc.Count = 1;

		ComPtr<IDXGISwapChain1> pSwapChain;
		if (FAILED(pFactory->CreateSwapChainForHwnd(m_pCommandQueue.Get(), hWnd, &swapChainDesc, nullptr, nullptr, &pSwapChain)))
		{
			dprintf("Failed to create DXGI swapchain.\n");
			return false;
		}

		pFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
		pSwapChain.As(&m_pSwapChain);
		m_nFrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();

		return true;
	}

	void Execute(const ComPtr<ID3D12CommandList> &commandList)
	{
		m_pCommandQueue->ExecuteCommandLists(1, commandList.GetAddressOf());
	}

	void Sync()
	{
		const UINT64 value = m_nFenceValues[m_nFrameIndex]++;
		m_pCommandQueue->Signal(m_pFence.Get(), value);
		m_nFrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();
		if (m_pFence->GetCompletedValue() < value)
		{
			m_pFence->SetEventOnCompletion(value, m_fenceEvent);
			WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
		}
	}

	void Present()
	{
		m_pSwapChain->Present(0, 0);
	}
};

ComPtr<IDXGIFactory4> CreateFactory(bool isDebug)
{
	UINT flag = 0;
	if (isDebug)
	{
		ComPtr<ID3D12Debug> debug;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
		{
			debug->EnableDebugLayer();
			flag |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
	ComPtr<IDXGIFactory4> factory;
	if (FAILED(CreateDXGIFactory2(flag, IID_PPV_ARGS(&factory))))
	{
		return nullptr;
	}
	return factory;
}

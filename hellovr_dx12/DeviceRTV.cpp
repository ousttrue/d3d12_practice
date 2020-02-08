#include "DeviceRTV.h"

template <class T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

DeviceRTV::DeviceRTV(int frameCount)
	: m_nFenceValues(frameCount, 0)
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
		m_pDevice->CreateFence(m_nFenceValues[m_nFrameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence));
		m_nFenceValues[m_nFrameIndex]++;

		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	}

	return m_pDevice;
}

bool DeviceRTV::CreateSwapchain(const ComPtr<IDXGIFactory4> &pFactory, int width, int height, HWND hWnd)
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
	const UINT64 value = m_nFenceValues[m_nFrameIndex]++;
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

#pragma once
#include <d3d12.h>
#include "d3dx12.h"
#include <wrl/client.h>
#include <dxgi1_4.h>
#include <vector>

// Slots in the RenderTargetView descriptor heap
enum class RTVIndex_t
{
	RTV_LEFT_EYE = 0,
	RTV_RIGHT_EYE,
	RTV_SWAPCHAIN0,
	RTV_SWAPCHAIN1,
	NUM_RTVS
};

///
/// Manage ID3D12Device, IDXGISwapChain3 and RenderTarget, DSV
///
class DeviceRTV
{
	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	// d3d12
	ComPtr<ID3D12Device> m_pDevice;
	ComPtr<ID3D12CommandQueue> m_pCommandQueue;
	HANDLE m_fenceEvent = NULL;
	ComPtr<ID3D12Fence> m_pFence;

	// swapchain
	ComPtr<IDXGISwapChain3> m_pSwapChain;
	UINT m_nFrameIndex = 0;

	// RTV
	UINT m_nRTVDescriptorSize = 0;
	ComPtr<ID3D12DescriptorHeap> m_pRTVHeap;

	UINT m_nDSVDescriptorSize = 0;
	ComPtr<ID3D12DescriptorHeap> m_pDSVHeap;

	struct FrameResource
	{
		ComPtr<ID3D12Resource> m_pSwapChainRenderTarget;
		ComPtr<ID3D12CommandAllocator> m_pCommandAllocator;
		ComPtr<ID3D12GraphicsCommandList> m_pCommandList;
		UINT64 m_nFenceValues = 0;
	};
	std::vector<FrameResource> m_frames;

public:
	DeviceRTV(int frameCount);
	const ComPtr<ID3D12Device> &Device() const { return m_pDevice; }
	const ComPtr<ID3D12CommandQueue> &Queue() const { return m_pCommandQueue; }
	const ComPtr<IDXGISwapChain3> &Swapchain() const { return m_pSwapChain; }
	// UINT FrameIndex() const { return m_nFrameIndex; }

	D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle(RTVIndex_t index) const
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart());
		handle.Offset((int)index, m_nRTVDescriptorSize);
		return handle;
	}
	D3D12_CPU_DESCRIPTOR_HANDLE RTVHandleCurrent() const
	{
		return RTVHandle((RTVIndex_t)((int)RTVIndex_t::RTV_SWAPCHAIN0 + m_nFrameIndex));
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DSVHandle(RTVIndex_t index) const
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_pDSVHeap->GetCPUDescriptorHandleForHeapStart());
		handle.Offset((int)index, m_nDSVDescriptorSize);
		return handle;
	}

	ComPtr<ID3D12Device> CreateDevice(const ComPtr<IDXGIFactory4> &factory, int adapterIndex);
	bool CreateSwapchain(const ComPtr<IDXGIFactory4> &pFactory, int width, int height, HWND hWnd);
	void Execute(const ComPtr<ID3D12CommandList> &commandList);
	void Sync();
	void Present();
	void SetupFrameResources(const ComPtr<ID3D12PipelineState> &pipelineState);

	const FrameResource &CurrentFrame() const
	{
		return m_frames[m_nFrameIndex];
	}
	FrameResource &CurrentFrame()
	{
		return m_frames[m_nFrameIndex];
	}
};

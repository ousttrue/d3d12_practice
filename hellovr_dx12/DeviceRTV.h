#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <dxgi1_4.h>
#include <vector>

///
/// Manage ID3D12Device, IDXGISwapChain3 and RenderTarget, DSV
///
class DeviceRTV
{
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

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
	DeviceRTV(int frameCount);
	const ComPtr<ID3D12Device> &Device() const { return m_pDevice; }
	const ComPtr<ID3D12CommandQueue> &Queue() const { return m_pCommandQueue; }
	const ComPtr<IDXGISwapChain3> &Swapchain() const { return m_pSwapChain; }
	UINT FrameIndex() const { return m_nFrameIndex; }
	ComPtr<ID3D12Device> CreateDevice(const ComPtr<IDXGIFactory4> &factory, int adapterIndex);
	bool CreateSwapchain(const ComPtr<IDXGIFactory4> &pFactory, int width, int height, HWND hWnd);
	void Execute(const ComPtr<ID3D12CommandList> &commandList);
	void Sync();
	void Present();
};

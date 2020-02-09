#pragma once
#include <d3d12.h>
#include <wrl/client.h>

class Fence
{
    template<class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    HANDLE m_fenceEvent = NULL;
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValue = 1;

public:
    Fence();
    ~Fence();
    void Initialize(const ComPtr<ID3D12Device> &device);
    void Wait(const ComPtr<ID3D12CommandQueue> queue);
};

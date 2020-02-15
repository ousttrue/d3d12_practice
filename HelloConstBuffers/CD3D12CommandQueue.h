#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <list>
#include <functional>

class CD3D12CommandQueue
{
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    ComPtr<ID3D12CommandQueue> m_commandQueue;

    // Synchronization objects.
    HANDLE m_fenceEvent = NULL;
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValue = 0;

public:
    using CallbackList = std::list<std::function<void()>>;

    ~CD3D12CommandQueue();
    const ComPtr<ID3D12CommandQueue> &CommandQueue() const { return m_commandQueue; }
    void Initialize(const ComPtr<ID3D12Device> &device);
    void Execute(ID3D12CommandList *commandList);
    void SyncFence(const CallbackList &callbacks = CallbackList());
};

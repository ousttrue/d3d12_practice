#pragma
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <queue>
#include <functional>

struct UploadCommand
{
    std::shared_ptr<class ResourceItem> Item;
    const void *Data;
    UINT ByteLength;
    UINT Stride;
};

class Uploader
{
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;

    // Synchronization objects.
    HANDLE m_fenceEvent;
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValue;

    std::queue<UploadCommand> m_queue;
    using OnCompletedFunc = std::function<void()>;
    OnCompletedFunc m_callback;

public:
    void Initialize(const ComPtr<ID3D12Device> &device);
    void Update();
    void EnqueueUpload(const UploadCommand &command)
    {
        m_queue.push(command);
    }
};

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

    class CD3D12CommandQueue *m_queue = nullptr;
    class CommandList *m_commandList = nullptr;

    std::queue<UploadCommand> m_commands;
    using OnCompletedFunc = std::function<void()>;
    OnCompletedFunc m_callback;
    UINT64 m_callbackFenceValue = 0;

    // for upload staging
    ComPtr<ID3D12Resource> m_upload;

public:
    Uploader();
    ~Uploader();
    void Initialize(const ComPtr<ID3D12Device> &device);
    void Update(const ComPtr<ID3D12Device> &device);
    void EnqueueUpload(const UploadCommand &command)
    {
        m_commands.push(command);
    }
};

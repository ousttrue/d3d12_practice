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

#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <memory>

enum UploadStates
{
    None,
    Enqueued,
    Uploaded,
};

/// ID3D12Resource helper
///
/// * State maintenance
/// * Barrier helper
///
class ResourceItem : public std::enable_shared_from_this<ResourceItem>
{
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    ComPtr<ID3D12Resource> m_resource;

    D3D12_RESOURCE_STATES m_state = {};
    UploadStates m_upload = UploadStates::None;

    ResourceItem(const ComPtr<ID3D12Resource> &resource, D3D12_RESOURCE_STATES state);

public:
    D3D12_RESOURCE_STATES ResourceState() const { return m_state; }
    UploadStates UploadState() const { return m_upload; }
    const ComPtr<ID3D12Resource> &Resource() const { return m_resource; }
    void MapCopyUnmap(const void *p, UINT byteLength);
    void EnqueueTransition(class CommandList *commandList, D3D12_RESOURCE_STATES state);
    void EnqueueUpload(class CommandList *commandList, const ComPtr<ID3D12Resource> &upload, const void *p, UINT byteLength, UINT stride);
    static std::shared_ptr<ResourceItem> CreateUpload(const ComPtr<ID3D12Device> &device, UINT byteLength);
    static std::shared_ptr<ResourceItem> CreateDefault(const ComPtr<ID3D12Device> &device, UINT byteLength);
};

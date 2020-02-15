#include "ResourceItem.h"
#include "d3dx12.h"
#include "d3dhelper.h"
#include "CommandList.h"

ResourceItem::ResourceItem(
    const ComPtr<ID3D12Resource> &resource,
    D3D12_RESOURCE_STATES state)
    : m_resource(resource), m_state(state)
{
}

void ResourceItem::MapCopyUnmap(const void *p, UINT byteLength)
{
    // Copy the triangle data to the vertex buffer.
    UINT8 *begin;
    CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.
    ThrowIfFailed(m_resource->Map(0, &readRange, reinterpret_cast<void **>(&begin)));
    memcpy(begin, p, byteLength);
    m_resource->Unmap(0, nullptr);

    m_upload = UploadStates::Uploaded;
}

void ResourceItem::EnqueueTransition(CommandList *commandList, D3D12_RESOURCE_STATES state)
{
    commandList->Get()->ResourceBarrier(
        1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            m_resource.Get(),
            m_state,
            state));

    std::weak_ptr weak = shared_from_this();
    auto callback = [weak, state]() {
        auto shared = weak.lock();
        if (shared)
        {
            shared->m_state = state;
        }
    };

    commandList->AddOnCompleted(callback);
}

void ResourceItem::EnqueueUpload(CommandList *commandList,
                                 const ComPtr<ID3D12Resource> &upload, 
                                 const void *p, UINT byteLength, UINT stride)
{
    // Copy data to the intermediate upload heap and then schedule a copy
    // from the upload heap to the vertex buffer.
    D3D12_SUBRESOURCE_DATA vertexData = {
        .pData = p,
        .RowPitch = byteLength,
        .SlicePitch = stride,
    };
    UpdateSubresources<1>(commandList->Get(), m_resource.Get(), upload.Get(), 0, 0, 1, &vertexData);

    std::weak_ptr weak = shared_from_this();
    auto callback = [weak]() {
        auto shared = weak.lock();
        if (shared)
        {
            shared->m_upload = UploadStates::Uploaded;
        }
    };

    commandList->AddOnCompleted(callback);
}

std::shared_ptr<ResourceItem> ResourceItem::CreateUpload(const ComPtr<ID3D12Device> &device, UINT byteLength)
{
    ComPtr<ID3D12Resource> resource;
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteLength),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&resource)));

    return std::shared_ptr<ResourceItem>(
        new ResourceItem(resource, D3D12_RESOURCE_STATE_GENERIC_READ));
}

std::shared_ptr<ResourceItem> ResourceItem::CreateDefault(const ComPtr<ID3D12Device> &device, UINT byteLength)
{
    ComPtr<ID3D12Resource> resource;
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteLength),
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&resource)));

    return std::shared_ptr<ResourceItem>(
        new ResourceItem(resource, D3D12_RESOURCE_STATE_COPY_DEST));
}

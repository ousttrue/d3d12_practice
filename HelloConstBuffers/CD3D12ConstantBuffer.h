#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include "d3dx12.h"
#include "d3dhelper.h"

template <typename T>
class CD3D12ConstantBuffer
{
    Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
    UINT8 *m_pCbvDataBegin = nullptr;

public:
    T Data{};

    void Initialize(const Microsoft::WRL::ComPtr<ID3D12Device> &device,
                    const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> &cbvHeap)
    {
        ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(1024 * 64),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_resource)));

        // Describe and create a constant buffer view.
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {
            .BufferLocation = m_resource->GetGPUVirtualAddress(),
            .SizeInBytes = (sizeof(T) + 255) & ~255, // CB size is required to be 256-byte aligned.
        };
        device->CreateConstantBufferView(&cbvDesc, cbvHeap->GetCPUDescriptorHandleForHeapStart());

        // Map and initialize the constant buffer. We don't unmap this until the
        // app closes. Keeping things mapped for the lifetime of the resource is okay.
        CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(m_resource->Map(0, &readRange, reinterpret_cast<void **>(&m_pCbvDataBegin)));
    }

    void
    CopyToGpu()
    {
        memcpy(m_pCbvDataBegin, &Data, sizeof(T));
        // memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));
    }
};

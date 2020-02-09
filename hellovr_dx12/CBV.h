#pragma once
#include <d3d12.h>
#include "d3dx12.h"
#include <wrl/client.h>
#include <openvr.h>

// Slots in the ConstantBufferView/ShaderResourceView descriptor heap
enum CBVSRVIndex_t
{
    CBV_LEFT_EYE = 0,
    CBV_RIGHT_EYE,
    SRV_LEFT_EYE,
    SRV_RIGHT_EYE,
    SRV_TEXTURE_MAP,
    // Slot for texture in each possible render model
    SRV_TEXTURE_RENDER_MODEL0,
    SRV_TEXTURE_RENDER_MODEL_MAX = SRV_TEXTURE_RENDER_MODEL0 + vr::k_unMaxTrackedDeviceCount,
    // Slot for transform in each possible rendermodel
    CBV_LEFT_EYE_RENDER_MODEL0,
    CBV_LEFT_EYE_RENDER_MODEL_MAX = CBV_LEFT_EYE_RENDER_MODEL0 + vr::k_unMaxTrackedDeviceCount,
    CBV_RIGHT_EYE_RENDER_MODEL0,
    CBV_RIGHT_EYE_RENDER_MODEL_MAX = CBV_RIGHT_EYE_RENDER_MODEL0 + vr::k_unMaxTrackedDeviceCount,
    NUM_SRV_CBVS
};

class CBV
{
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    UINT m_nCBVSRVDescriptorSize = 0;
    ComPtr<ID3D12DescriptorHeap> m_pCBVSRVHeap;

public:
    const ComPtr<ID3D12DescriptorHeap> &Heap() const { return m_pCBVSRVHeap; }
    D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle(CBVSRVIndex_t index) const
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_pCBVSRVHeap->GetCPUDescriptorHandleForHeapStart());
        handle.Offset(index, m_nCBVSRVDescriptorSize);
        return handle;
    }
    D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle(CBVSRVIndex_t index) const
    {
        CD3DX12_GPU_DESCRIPTOR_HANDLE handle(m_pCBVSRVHeap->GetGPUDescriptorHandleForHeapStart());
        handle.Offset(index, m_nCBVSRVDescriptorSize);
        return handle;
    }

    // Create descriptor heaps
    bool Initialize(const ComPtr<ID3D12Device> &device);
};

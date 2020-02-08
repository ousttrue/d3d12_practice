#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <openvr.h>

class Models
{
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    class DX12RenderModel *m_rTrackedDeviceToRenderModel[vr::k_unMaxTrackedDeviceCount];
    UINT m_nCBVSRVDescriptorSize = 0;

public:
    void Draw(const ComPtr<ID3D12GraphicsCommandList> &pCommandList, vr::EVREye nEye, UINT unTrackedDevice, const class Matrix4 &matMVP);

    //-----------------------------------------------------------------------------
    // Purpose: Create/destroy D3D12 Render Models
    //-----------------------------------------------------------------------------
    void SetupRenderModels(class HMD *hmd,
                           const ComPtr<ID3D12Device> &device,
                           const ComPtr<ID3D12DescriptorHeap> &heap,
                           const ComPtr<ID3D12GraphicsCommandList> &pCommandList);

    //-----------------------------------------------------------------------------
    // Purpose: Create/destroy D3D12 a Render Model for a single tracked device
    //-----------------------------------------------------------------------------
    void SetupRenderModelForTrackedDevice(class HMD *hmd,
                                          const ComPtr<ID3D12Device> &device,
                                          const ComPtr<ID3D12DescriptorHeap> &heap,
                                          const ComPtr<ID3D12GraphicsCommandList> &pCommandList,
                                          vr::TrackedDeviceIndex_t unTrackedDeviceIndex);

private:
    //-----------------------------------------------------------------------------
    // Purpose: Finds a render model we've already loaded or loads a new one
    //-----------------------------------------------------------------------------
    DX12RenderModel *FindOrLoadRenderModel(
        const ComPtr<ID3D12Device> &device,
        const ComPtr<ID3D12DescriptorHeap> &heap,
        const ComPtr<ID3D12GraphicsCommandList> &pCommandList,
        vr::TrackedDeviceIndex_t unTrackedDeviceIndex, const char *pchRenderModelName);
};

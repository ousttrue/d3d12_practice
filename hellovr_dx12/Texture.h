#pragma once
#include <d3d12.h>
#include <wrl/client.h>

class Texture
{
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    ComPtr<ID3D12Resource> m_pTexture;
    ComPtr<ID3D12Resource> m_pTextureUploadHeap;

public:
    bool SetupTexturemaps(const ComPtr<ID3D12Device> &device,
                          const ComPtr<ID3D12GraphicsCommandList> &pCommandList,
                          D3D12_CPU_DESCRIPTOR_HANDLE srvHandle);
};

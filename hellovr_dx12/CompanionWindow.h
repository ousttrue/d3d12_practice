#pragma once
#include <d3d12.h>
#include <wrl/client.h>

class CompanionWindow
{
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    ComPtr<ID3D12Resource> m_pCompanionWindowVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_companionWindowVertexBufferView;
    ComPtr<ID3D12Resource> m_pCompanionWindowIndexBuffer;
    D3D12_INDEX_BUFFER_VIEW m_companionWindowIndexBufferView;

    unsigned int m_uiCompanionWindowIndexSize = 0;

public:
    void SetupCompanionWindow(const ComPtr<ID3D12Device> &device);

    void Draw(const ComPtr<ID3D12GraphicsCommandList> &pCommandList,
              D3D12_GPU_DESCRIPTOR_HANDLE srvHandleLeftEye,
              D3D12_GPU_DESCRIPTOR_HANDLE srvHandleRightEye);
};

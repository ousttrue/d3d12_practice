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

    int m_nMSAASampleCount = 0;
    struct FramebufferDesc
    {
        ComPtr<ID3D12Resource> m_pTexture;
        D3D12_CPU_DESCRIPTOR_HANDLE m_renderTargetViewHandle = {};
        ComPtr<ID3D12Resource> m_pDepthStencil;
        D3D12_CPU_DESCRIPTOR_HANDLE m_depthStencilViewHandle = {};
    };
    FramebufferDesc m_leftEyeDesc = {};
    FramebufferDesc m_rightEyeDesc = {};

public:
    CompanionWindow(int msaa)
        : m_nMSAASampleCount(msaa)
    {
    }

    const ComPtr<ID3D12Resource> &LeftEyeTexture() const
    {
        return m_leftEyeDesc.m_pTexture;
    }
    const ComPtr<ID3D12Resource> &RightEyeTexture() const
    {
        return m_rightEyeDesc.m_pTexture;
    }
    bool CreateFrameBuffer(const ComPtr<ID3D12Device> &device, int nWidth, int nHeight,
                           D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
                           D3D12_CPU_DESCRIPTOR_HANDLE srvHandle,
                           D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle,
                           FramebufferDesc &framebufferDesc);
    bool CreateFrameBufferLeft(const ComPtr<ID3D12Device> &device, int nWidth, int nHeight,
                               D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
                               D3D12_CPU_DESCRIPTOR_HANDLE srvHandle,
                               D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle)
    {
        return CreateFrameBuffer(device, nWidth, nHeight, rtvHandle, srvHandle, dsvHandle, m_leftEyeDesc);
    }
    bool CreateFrameBufferRight(const ComPtr<ID3D12Device> &device, int nWidth, int nHeight,
                                D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
                                D3D12_CPU_DESCRIPTOR_HANDLE srvHandle,
                                D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle)
    {
        return CreateFrameBuffer(device, nWidth, nHeight, rtvHandle, srvHandle, dsvHandle, m_rightEyeDesc);
    }

    void SetupCompanionWindow(const ComPtr<ID3D12Device> &device);
    void Draw(const ComPtr<ID3D12GraphicsCommandList> &pCommandList,
              D3D12_GPU_DESCRIPTOR_HANDLE srvHandleLeftEye,
              D3D12_GPU_DESCRIPTOR_HANDLE srvHandleRightEye);
    void BeginLeft(const ComPtr<ID3D12GraphicsCommandList> &pCommandList);
    void EndLeft(const ComPtr<ID3D12GraphicsCommandList> &pCommandList);
    void BeginRight(const ComPtr<ID3D12GraphicsCommandList> &pCommandList);
    void EndRight(const ComPtr<ID3D12GraphicsCommandList> &pCommandList);
};

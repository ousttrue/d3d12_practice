#pragma once
#include <d3d12.h>
#include <wrl/client.h>

class CompanionWindow
{
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    float m_flSuperSampleScale;
    int m_nMSAASampleCount;

    UINT32 m_nRenderWidth = 0;
    UINT32 m_nRenderHeight = 0;

    ComPtr<ID3D12Resource> m_pCompanionWindowVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_companionWindowVertexBufferView;
    ComPtr<ID3D12Resource> m_pCompanionWindowIndexBuffer;
    D3D12_INDEX_BUFFER_VIEW m_companionWindowIndexBufferView;

    unsigned int m_uiCompanionWindowIndexSize = 0;

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
    CompanionWindow(int msaa, float flSuperSampleScale)
        : m_nMSAASampleCount(msaa), m_flSuperSampleScale(flSuperSampleScale)
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

private:
    bool CreateFrameBuffer(const ComPtr<ID3D12Device> &device,
                           D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
                           D3D12_CPU_DESCRIPTOR_HANDLE srvHandle,
                           D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle,
                           FramebufferDesc &framebufferDesc);

public:
    void SetupCompanionWindow(const ComPtr<ID3D12Device> &device, UINT32 width, UINT32 height,
                              D3D12_CPU_DESCRIPTOR_HANDLE leftRtvHandle,
                              D3D12_CPU_DESCRIPTOR_HANDLE leftSrvHandle,
                              D3D12_CPU_DESCRIPTOR_HANDLE leftDsvHandle,
                              D3D12_CPU_DESCRIPTOR_HANDLE rightRtvHandle,
                              D3D12_CPU_DESCRIPTOR_HANDLE rightSrvHandle,
                              D3D12_CPU_DESCRIPTOR_HANDLE rightDsvHandle);
    void Draw(const ComPtr<ID3D12GraphicsCommandList> &pCommandList,
              D3D12_GPU_DESCRIPTOR_HANDLE srvHandleLeftEye,
              D3D12_GPU_DESCRIPTOR_HANDLE srvHandleRightEye);
    void BeginLeft(const ComPtr<ID3D12GraphicsCommandList> &pCommandList);
    void EndLeft(const ComPtr<ID3D12GraphicsCommandList> &pCommandList);
    void BeginRight(const ComPtr<ID3D12GraphicsCommandList> &pCommandList);
    void EndRight(const ComPtr<ID3D12GraphicsCommandList> &pCommandList);
};

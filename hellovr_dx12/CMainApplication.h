#pragma once
#include <openvr.h>
#include <vector>
#include <wrl/client.h>
#include <dxgi1_4.h>
#include "d3dx12.h"
#include "Matrices.h"


class CMainApplication
{
    class SDLApplication *m_sdl = nullptr;
    class HMD *m_hmd = nullptr;
    class DeviceRTV *m_d3d = nullptr;
    class Cubes *m_cubes = nullptr;
    class Models *m_models = nullptr;

    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
    CMainApplication(int msaa, float flSuperSampleScale, int volume);
    virtual ~CMainApplication();

    bool Initialize(bool bDebugD3D12);
    void RunMainLoop();

private:
    bool BInitD3D12();
    bool BInitCompositor();

    void SetupRenderModels(const ComPtr<ID3D12GraphicsCommandList> &pCommandList);

    bool HandleInput(const ComPtr<ID3D12GraphicsCommandList> &pCommandList);
    void ProcessVREvent(const vr::VREvent_t &event, const ComPtr<ID3D12GraphicsCommandList> &pCommandList);
    void RenderFrame(const ComPtr<ID3D12GraphicsCommandList> &pCommandList, const ComPtr<ID3D12Resource> &rtv);

    bool SetupTexturemaps(const ComPtr<ID3D12GraphicsCommandList> &pCommandList);

    void UpdateControllerAxes();

    bool SetupStereoRenderTargets();
    void SetupCompanionWindow();

    void RenderStereoTargets(const ComPtr<ID3D12GraphicsCommandList> &pCommandList);
    void RenderCompanionWindow(const ComPtr<ID3D12GraphicsCommandList> &pCommandList, const ComPtr<ID3D12Resource> &swapchainRTV);
    void RenderScene(vr::Hmd_Eye nEye, const ComPtr<ID3D12GraphicsCommandList> &pCommandList);
    bool CreateAllShaders();

private:
    int m_nMSAASampleCount;
    float m_flSuperSampleScale;
    int m_iTrackedControllerCount;
    int m_iTrackedControllerCount_Last;
    int m_iValidPoseCount;
    int m_iValidPoseCount_Last;
    bool m_bShowCubes;

    std::string m_strPoseClasses; // what classes we saw poses for this frame

    unsigned int m_uiCompanionWindowIndexSize;

    // D3D12 members
    static const int g_nFrameCount = 2; // Swapchain depth

    ComPtr<ID3D12DescriptorHeap> m_pCBVSRVHeap;
    ComPtr<ID3D12RootSignature> m_pRootSignature;
    ComPtr<ID3D12PipelineState> m_pScenePipelineState;
    ComPtr<ID3D12PipelineState> m_pCompanionPipelineState;
    ComPtr<ID3D12PipelineState> m_pAxesPipelineState;
    ComPtr<ID3D12PipelineState> m_pRenderModelPipelineState;
    ComPtr<ID3D12Resource> m_pSceneConstantBuffer;
    D3D12_CPU_DESCRIPTOR_HANDLE m_sceneConstantBufferView[2];
    UINT8 *m_pSceneConstantBufferData[2];
    UINT m_nCBVSRVDescriptorSize;

    ComPtr<ID3D12Resource> m_pTexture;
    ComPtr<ID3D12Resource> m_pTextureUploadHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE m_textureShaderResourceView;
    ComPtr<ID3D12Resource> m_pCompanionWindowVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_companionWindowVertexBufferView;
    ComPtr<ID3D12Resource> m_pCompanionWindowIndexBuffer;
    D3D12_INDEX_BUFFER_VIEW m_companionWindowIndexBufferView;
    ComPtr<ID3D12Resource> m_pControllerAxisVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_controllerAxisVertexBufferView;

    unsigned int m_uiControllerVertcount;

    struct VertexDataWindow
    {
        Vector2 position;
        Vector2 texCoord;

        VertexDataWindow(const Vector2 &pos, const Vector2 tex) : position(pos), texCoord(tex) {}
    };

    struct FramebufferDesc
    {
        ComPtr<ID3D12Resource> m_pTexture;
        CD3DX12_CPU_DESCRIPTOR_HANDLE m_renderTargetViewHandle;
        ComPtr<ID3D12Resource> m_pDepthStencil;
        CD3DX12_CPU_DESCRIPTOR_HANDLE m_depthStencilViewHandle;
    };
    FramebufferDesc m_leftEyeDesc;
    FramebufferDesc m_rightEyeDesc;

    bool CreateFrameBuffer(int nWidth, int nHeight, FramebufferDesc &framebufferDesc, bool isLeft);

    uint32_t m_nRenderWidth;
    uint32_t m_nRenderHeight;
};

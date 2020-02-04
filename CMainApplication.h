#pragma once
#include <openvr.h>
#include <vector>
#include <wrl/client.h>
#include <dxgi1_4.h>
#include "d3dx12.h"
#include "Matrices.h"

// Slots in the RenderTargetView descriptor heap
enum RTVIndex_t
{
    RTV_LEFT_EYE = 0,
    RTV_RIGHT_EYE,
    RTV_SWAPCHAIN0,
    RTV_SWAPCHAIN1,
    NUM_RTVS
};

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

class CMainApplication
{
public:
    CMainApplication(int msaa, float flSuperSampleScale);
    virtual ~CMainApplication();

    bool BInit(bool bDebugD3D12, int iSceneVolumeInit);
    bool BInitD3D12(bool bDebugD3D12);
    bool BInitCompositor();

    void SetupRenderModels();

    void RunMainLoop();
    bool HandleInput();
    void ProcessVREvent(const vr::VREvent_t &event);
    void RenderFrame();

    bool SetupTexturemaps();
    static void GenMipMapRGBA(const UINT8 *pSrc, UINT8 **ppDst, int nSrcWidth, int nSrcHeight, int *pDstWidthOut, int *pDstHeightOut);

    void SetupScene();
    void AddCubeToScene(Matrix4 mat, std::vector<float> &vertdata);
    void AddCubeVertex(float fl0, float fl1, float fl2, float fl3, float fl4, std::vector<float> &vertdata);

    void UpdateControllerAxes();

    bool SetupStereoRenderTargets();
    void SetupCompanionWindow();
    void SetupCameras();

    void RenderStereoTargets();
    void RenderCompanionWindow();
    void RenderScene(vr::Hmd_Eye nEye);

    Matrix4 GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye);
    Matrix4 GetHMDMatrixPoseEye(vr::Hmd_Eye nEye);
    Matrix4 GetCurrentViewProjectionMatrix(vr::Hmd_Eye nEye);
    void UpdateHMDMatrixPose();

    Matrix4 ConvertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t &matPose);

    bool CreateAllShaders();

    void SetupRenderModelForTrackedDevice(vr::TrackedDeviceIndex_t unTrackedDeviceIndex);
    class DX12RenderModel *FindOrLoadRenderModel(vr::TrackedDeviceIndex_t unTrackedDeviceIndex, const char *pchRenderModelName);

private:
    vr::IVRSystem *m_pHMD = nullptr;
    vr::IVRRenderModels *m_pRenderModels = nullptr;
    int m_nMSAASampleCount;
    float m_flSuperSampleScale;
    std::string m_strDriver;
    std::string m_strDisplay;
    vr::TrackedDevicePose_t m_rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
    Matrix4 m_rmat4DevicePose[vr::k_unMaxTrackedDeviceCount];
    bool m_rbShowTrackedDevice[vr::k_unMaxTrackedDeviceCount];

private: // SDL bookkeeping
    class SDLApplication *m_sdl = nullptr;

private:
    int m_iTrackedControllerCount;
    int m_iTrackedControllerCount_Last;
    int m_iValidPoseCount;
    int m_iValidPoseCount_Last;
    bool m_bShowCubes;

    std::string m_strPoseClasses;                        // what classes we saw poses for this frame
    char m_rDevClassChar[vr::k_unMaxTrackedDeviceCount]; // for each device, a character representing its class

    int m_iSceneVolumeWidth;
    int m_iSceneVolumeHeight;
    int m_iSceneVolumeDepth;
    float m_fScaleSpacing;
    float m_fScale;


    float m_fNearClip;
    float m_fFarClip;

    unsigned int m_uiVertcount;
    unsigned int m_uiCompanionWindowIndexSize;

    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    // D3D12 members
    UINT m_nFrameIndex;
    HANDLE m_fenceEvent;
    ComPtr<ID3D12Fence> m_pFence;
    static const int g_nFrameCount = 2; // Swapchain depth
    UINT64 m_nFenceValues[g_nFrameCount];
    ComPtr<ID3D12Device> m_pDevice;
    ComPtr<IDXGISwapChain3> m_pSwapChain;
    ComPtr<ID3D12Resource> m_pSwapChainRenderTarget[g_nFrameCount];
    ComPtr<ID3D12CommandQueue> m_pCommandQueue;
    ComPtr<ID3D12CommandAllocator> m_pCommandAllocators[g_nFrameCount];
    ComPtr<ID3D12GraphicsCommandList> m_pCommandList;
    ComPtr<ID3D12DescriptorHeap> m_pCBVSRVHeap;
    ComPtr<ID3D12DescriptorHeap> m_pRTVHeap;
    ComPtr<ID3D12DescriptorHeap> m_pDSVHeap;
    ComPtr<ID3D12RootSignature> m_pRootSignature;
    ComPtr<ID3D12PipelineState> m_pScenePipelineState;
    ComPtr<ID3D12PipelineState> m_pCompanionPipelineState;
    ComPtr<ID3D12PipelineState> m_pAxesPipelineState;
    ComPtr<ID3D12PipelineState> m_pRenderModelPipelineState;
    ComPtr<ID3D12Resource> m_pSceneConstantBuffer;
    D3D12_CPU_DESCRIPTOR_HANDLE m_sceneConstantBufferView[2];
    UINT8 *m_pSceneConstantBufferData[2];
    UINT m_nRTVDescriptorSize;
    UINT m_nDSVDescriptorSize;
    UINT m_nCBVSRVDescriptorSize;

    ComPtr<ID3D12Resource> m_pSceneVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_sceneVertexBufferView;
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

    Matrix4 m_mat4HMDPose;
    Matrix4 m_mat4eyePosLeft;
    Matrix4 m_mat4eyePosRight;

    Matrix4 m_mat4ProjectionCenter;
    Matrix4 m_mat4ProjectionLeft;
    Matrix4 m_mat4ProjectionRight;

    struct VertexDataScene
    {
        Vector3 position;
        Vector2 texCoord;
    };

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

    bool CreateFrameBuffer(int nWidth, int nHeight, FramebufferDesc &framebufferDesc, RTVIndex_t nRTVIndex);

    uint32_t m_nRenderWidth;
    uint32_t m_nRenderHeight;

    std::vector<DX12RenderModel *> m_vecRenderModels;
    DX12RenderModel *m_rTrackedDeviceToRenderModel[vr::k_unMaxTrackedDeviceCount];
};

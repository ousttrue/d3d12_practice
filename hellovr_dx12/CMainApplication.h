#pragma once
#include <wrl/client.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <openvr.h>
#include <string>

class CMainApplication
{
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    class SDLApplication *m_sdl = nullptr;
    class HMD *m_hmd = nullptr;
    class DeviceRTV *m_d3d = nullptr;
    class Cubes *m_cubes = nullptr;
    class Models *m_models = nullptr;
    class Axis *m_axis = nullptr;
    class CBV *m_cbv = nullptr;
    class CompanionWindow *m_companionWindow = nullptr;
    class Pipeline *m_pipeline = nullptr;
    bool m_bShowCubes;
    std::string m_strPoseClasses;       // what classes we saw poses for this frame
    ComPtr<ID3D12Resource> m_pSceneConstantBuffer;
    D3D12_CPU_DESCRIPTOR_HANDLE m_sceneConstantBufferView[2] = {};
    UINT8 *m_pSceneConstantBufferData[2] = {};
    ComPtr<ID3D12Resource> m_pTexture;
    ComPtr<ID3D12Resource> m_pTextureUploadHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE m_textureShaderResourceView;

public:
    CMainApplication(int msaa, float flSuperSampleScale, int volume);
    virtual ~CMainApplication();
    bool Initialize(bool bDebugD3D12);
    void RunMainLoop();

private:
    bool BInitD3D12();
    bool BInitCompositor();
    bool HandleInput(const ComPtr<ID3D12GraphicsCommandList> &pCommandList);
    void ProcessVREvent(const vr::VREvent_t &event, const ComPtr<ID3D12GraphicsCommandList> &pCommandList);
    void RenderFrame(const ComPtr<ID3D12GraphicsCommandList> &pCommandList, const ComPtr<ID3D12Resource> &rtv);
    bool SetupTexturemaps(const ComPtr<ID3D12GraphicsCommandList> &pCommandList);
    void RenderScene(vr::Hmd_Eye nEye, const ComPtr<ID3D12GraphicsCommandList> &pCommandList);
};

#pragma once
#include <wrl/client.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <openvr.h>
#include <string>
#include <memory>

class CMainApplication
{
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    std::unique_ptr<class SDLApplication> m_sdl;
    std::unique_ptr<class HMD> m_hmd;
    std::unique_ptr<class DeviceRTV> m_d3d;
    std::unique_ptr<class Cubes> m_cubes;
    std::unique_ptr<class Models> m_models;
    std::unique_ptr<class Axis> m_axis;
    std::unique_ptr<class CBV> m_cbv;
    std::unique_ptr<class CompanionWindow> m_companionWindow;
    std::unique_ptr<class Pipeline> m_pipeline;
    std::unique_ptr<class Texture> m_texture;
    bool m_bShowCubes = false;
    std::string m_strPoseClasses; // what classes we saw poses for this frame

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
    void RenderScene(vr::Hmd_Eye nEye, const ComPtr<ID3D12GraphicsCommandList> &pCommandList);
};

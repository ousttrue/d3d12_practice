//========= Copyright Valve Corporation ============//
#include "CMainApplication.h"
#include "CBV.h"
#include "SDLApplication.h"
#include "dprintf.h"
#include "Hmd.h"
#include "GenMipMapRGBA.h"
#include "Pipeline.h"
#include "DeviceRTV.h"
#include "Cubes.h"
#include "Models.h"
#include "Axis.h"
#include "CompanionWindow.h"
#include "Texture.h"

using Microsoft::WRL::ComPtr;


CMainApplication::CMainApplication(int msaa, float flSuperSampleScale, int iSceneVolumeInit)
    : m_pipeline(new Pipeline(msaa)), m_texture(new Texture),
      m_sdl(new SDLApplication),
      m_hmd(new HMD), m_d3d(new DeviceRTV),
      m_cbv(new CBV),
      m_models(new Models), m_axis(new Axis), m_cubes(new Cubes(iSceneVolumeInit)), m_companionWindow(new CompanionWindow(msaa, flSuperSampleScale)),
      m_bShowCubes(true)
{
}

CMainApplication::~CMainApplication()
{
    dprintf("Shutdown");
}

static ComPtr<IDXGIFactory4> CreateFactory(bool isDebug)
{
    UINT flag = 0;
    if (isDebug)
    {
        ComPtr<ID3D12Debug> debug;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
        {
            debug->EnableDebugLayer();
            flag |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
    ComPtr<IDXGIFactory4> factory;
    if (FAILED(CreateDXGIFactory2(flag, IID_PPV_ARGS(&factory))))
    {
        return nullptr;
    }
    return factory;
}

bool CMainApplication::Initialize(bool bDebugD3D12)
{
    // Loading the SteamVR Runtime
    if (!m_hmd->Initialize())
    {
        return false;
    }

    if (!m_sdl->Initialize(m_hmd->SystemName(), m_hmd->SerialNumber()))
    {
        return false;
    }

    //-----------------------------------------------------------------------------
    // Purpose: Initialize DX12. Returns true if DX12 has been successfully
    //          initialized, false if shaders could not be created.
    //          If failure occurred in a module other than shaders, the function
    //          may return true or throw an error.
    //-----------------------------------------------------------------------------
    auto factory = CreateFactory(bDebugD3D12);
    if (!factory)
    {
        return false;
    }

    // Query OpenVR for the output adapter index
    int32_t nAdapterIndex = m_hmd->AdapterIndex();

    if (!m_d3d->CreateDevice(factory, nAdapterIndex))
    {
        return false;
    }
    if (!m_d3d->CreateSwapchain(factory, m_sdl->Width(), m_sdl->Height(), (HWND)m_sdl->HWND()))
    {
        return false;
    }

    if (!m_cbv->Initialize(m_d3d->Device()))
    {
        return false;
    }

    if (!m_pipeline->CreateAllShaders(m_d3d->Device()))
    {
        return false;
    }

    // Create command list
    {
        ComPtr<ID3D12CommandAllocator> pAllocator;
        if (FAILED(m_d3d->Device()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pAllocator))))
        {
            return false;
        }

        ComPtr<ID3D12GraphicsCommandList> pCommandList;
        if (FAILED(m_d3d->Device()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                      pAllocator.Get(),
                                                      nullptr,
                                                      IID_PPV_ARGS(&pCommandList))))
        {
            return false;
        }

        m_texture->SetupTexturemaps(m_d3d->Device(), pCommandList, m_cbv->CpuHandle(SRV_TEXTURE_MAP));
        if (m_hmd->Hmd())
        {
            m_cubes->SetupScene(m_d3d->Device());
        }
        m_hmd->SetupCameras();

        if (m_hmd->Hmd())
        {
            uint32_t width, height;
            m_hmd->Hmd()->GetRecommendedRenderTargetSize(&width, &height);

            m_companionWindow->SetupCompanionWindow(m_d3d->Device(), width, height,
                                                    m_d3d->RTVHandle(RTVIndex_t::RTV_LEFT_EYE),
                                                    m_cbv->CpuHandle(SRV_LEFT_EYE),
                                                    m_d3d->DSVHandle(RTVIndex_t::RTV_LEFT_EYE),
                                                    m_d3d->RTVHandle(RTVIndex_t::RTV_RIGHT_EYE),
                                                    m_cbv->CpuHandle(SRV_RIGHT_EYE),
                                                    m_d3d->DSVHandle(RTVIndex_t::RTV_RIGHT_EYE));

            m_models->SetupRenderModels(m_hmd.get(), m_d3d->Device(), m_cbv->Heap(), pCommandList);
        }

        // Do any work that was queued up during loading
        pCommandList->Close();

        m_d3d->Execute(pCommandList);
        m_d3d->Sync();
    }

    {
        //-----------------------------------------------------------------------------
        // Purpose: Initialize Compositor. Returns true if the compositor was
        //          successfully initialized, false otherwise.
        //-----------------------------------------------------------------------------
        vr::EVRInitError peError = vr::VRInitError_None;
        if (!vr::VRCompositor())
        {
            dprintf("Compositor initialization failed. See log file for details\n");
            return false;
        }
    }

    return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CMainApplication::HandleInput(const ComPtr<ID3D12GraphicsCommandList> &pCommandList)
{
    bool bRet = m_sdl->HandleInput(&m_bShowCubes);

    // Process SteamVR events
    vr::VREvent_t event;
    while (m_hmd->Hmd()->PollNextEvent(&event, sizeof(event)))
    {
        ProcessVREvent(event, pCommandList);
    }

    // Process SteamVR controller state
    m_hmd->Update();

    return bRet;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::RunMainLoop()
{
    m_d3d->SetupFrameResources(m_pipeline->SceneState());
    bool bQuit = false;
    while (!bQuit)
    {
        auto &frame = m_d3d->CurrentFrame();
        bQuit = HandleInput(frame.m_pCommandList);
        frame.m_pCommandAllocator->Reset();
        frame.m_pCommandList->Reset(frame.m_pCommandAllocator.Get(), m_pipeline->SceneState().Get());
        RenderFrame(frame.m_pCommandList, frame.m_pSwapChainRenderTarget);
    }
}

//-----------------------------------------------------------------------------
// Purpose: Processes a single VR event
//-----------------------------------------------------------------------------
void CMainApplication::ProcessVREvent(const vr::VREvent_t &event, const ComPtr<ID3D12GraphicsCommandList> &pCommandList)
{
    switch (event.eventType)
    {
    case vr::VREvent_TrackedDeviceActivated:
    {
        m_models->SetupRenderModelForTrackedDevice(m_hmd.get(), m_d3d->Device(), m_cbv->Heap(), pCommandList, event.trackedDeviceIndex);
        dprintf("Device %u attached. Setting up render model.\n", event.trackedDeviceIndex);
    }
    break;
    case vr::VREvent_TrackedDeviceDeactivated:
    {
        dprintf("Device %u detached.\n", event.trackedDeviceIndex);
    }
    break;
    case vr::VREvent_TrackedDeviceUpdated:
    {
        dprintf("Device %u updated.\n", event.trackedDeviceIndex);
    }
    break;
    }
}

//-----------------------------------------------------------------------------
// 1. Render LeftEye to LeftRTV
// 2. Render RighttEye to RightRTV
// 3. Render LeftRTV and RightRTV to CompanionWindow(Swapchain)
// 4. Submit to OpenVR
//-----------------------------------------------------------------------------
void CMainApplication::RenderFrame(const ComPtr<ID3D12GraphicsCommandList> &pCommandList, const ComPtr<ID3D12Resource> &rtv)
{
    if (m_hmd->Hmd())
    {
        pCommandList->SetGraphicsRootSignature(m_pipeline->RootSignature().Get());
        pCommandList->SetDescriptorHeaps(1, m_cbv->Heap().GetAddressOf());

        m_axis->UpdateControllerAxes(m_hmd.get(), m_d3d->Device());

        {
            // RENDER

            // Left Eye //
            m_companionWindow->BeginLeft(pCommandList);
            RenderScene(vr::Eye_Left, pCommandList);
            m_companionWindow->EndLeft(pCommandList);

            // Right Eye //
            m_companionWindow->BeginRight(pCommandList);
            RenderScene(vr::Eye_Right, pCommandList);
            m_companionWindow->EndRight(pCommandList);
        }

        {
            pCommandList->SetPipelineState(m_pipeline->CompanionState().Get());

            // Transition swapchain image to RENDER_TARGET
            pCommandList->ResourceBarrier(
                1, &CD3DX12_RESOURCE_BARRIER::Transition(
                       rtv.Get(),
                       D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

            // Bind current swapchain image
            auto rtvHandle = m_d3d->RTVHandleCurrent();
            pCommandList->OMSetRenderTargets(1, &rtvHandle, 0, nullptr);

            auto w = m_sdl->Width();
            auto h = m_sdl->Height();
            D3D12_VIEWPORT viewport = {0.0f, 0.0f, (FLOAT)w, (FLOAT)h, 0.0f, 1.0f};
            D3D12_RECT scissor = {0, 0, (LONG)w, (LONG)h};

            pCommandList->RSSetViewports(1, &viewport);
            pCommandList->RSSetScissorRects(1, &scissor);

            // render left eye (first half of index array)
            auto srvHandleLeftEye = m_cbv->GpuHandle(SRV_LEFT_EYE);

            // render right eye (second half of index array)
            auto srvHandleRightEye = m_cbv->GpuHandle(SRV_RIGHT_EYE);

            m_companionWindow->Draw(pCommandList, srvHandleLeftEye, srvHandleRightEye);

            // Transition swapchain image to PRESENT
            pCommandList->ResourceBarrier(
                1, &CD3DX12_RESOURCE_BARRIER::Transition(
                       rtv.Get(),
                       D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
        }

        pCommandList->Close();

        // Execute the command list.
        m_d3d->Execute(pCommandList);

        vr::VRTextureBounds_t bounds;
        bounds.uMin = 0.0f;
        bounds.uMax = 1.0f;
        bounds.vMin = 0.0f;
        bounds.vMax = 1.0f;

        vr::D3D12TextureData_t d3d12LeftEyeTexture = {m_companionWindow->LeftEyeTexture().Get(), m_d3d->Queue().Get(), 0};
        vr::Texture_t leftEyeTexture = {(void *)&d3d12LeftEyeTexture, vr::TextureType_DirectX12, vr::ColorSpace_Gamma};
        vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture, &bounds, vr::Submit_Default);

        vr::D3D12TextureData_t d3d12RightEyeTexture = {m_companionWindow->RightEyeTexture().Get(), m_d3d->Queue().Get(), 0};
        vr::Texture_t rightEyeTexture = {(void *)&d3d12RightEyeTexture, vr::TextureType_DirectX12, vr::ColorSpace_Gamma};
        vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture, &bounds, vr::Submit_Default);
    }

    // Present
    m_d3d->Present();

    // Wait for completion
    m_d3d->Sync();

    auto m_iValidPoseCount = m_hmd->UpdateHMDMatrixPose();
}

//-----------------------------------------------------------------------------
// Purpose: Renders a scene with respect to nEye.
//-----------------------------------------------------------------------------
void CMainApplication::RenderScene(vr::Hmd_Eye nEye, const ComPtr<ID3D12GraphicsCommandList> &pCommandList)
{
    if (m_bShowCubes)
    {
        // setup
        pCommandList->SetPipelineState(m_pipeline->SceneState().Get());
        // update constant buffer
        m_cbv->Set(pCommandList, nEye, m_hmd->GetCurrentViewProjectionMatrix(nEye));
        // draw
        m_cubes->Draw(pCommandList);
    }

    bool bIsInputAvailable = m_hmd->Hmd()->IsInputAvailable();

    if (bIsInputAvailable)
    {
        // draw the controller axis lines
        pCommandList->SetPipelineState(m_pipeline->AxisState().Get());

        m_axis->Draw(pCommandList);
    }

    // ----- Render Model rendering -----
    pCommandList->SetPipelineState(m_pipeline->RenderModelState().Get());
    for (uint32_t unTrackedDevice = 0; unTrackedDevice < vr::k_unMaxTrackedDeviceCount; unTrackedDevice++)
    {
        if (!m_hmd->IsVisible(unTrackedDevice))
            continue;

        if (!m_hmd->PoseIsValid(unTrackedDevice))
            continue;

        if (!bIsInputAvailable && m_hmd->Hmd()->GetTrackedDeviceClass(unTrackedDevice) == vr::TrackedDeviceClass_Controller)
            continue;

        Matrix4 matMVP = m_hmd->GetCurrentViewProjectionMatrix(nEye) * m_hmd->DevicePose(unTrackedDevice);

        m_models->Draw(pCommandList, nEye, unTrackedDevice, matMVP);
    }
}

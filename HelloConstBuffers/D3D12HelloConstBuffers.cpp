#include "D3D12HelloConstBuffers.h"
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include "d3dx12.h"
#include "d3dhelper.h"
#include "CD3D12SwapChain.h"
#include "CD3D12Scene.h"

class Impl
{
    // Pipeline objects.
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    // Synchronization objects.
    HANDLE m_fenceEvent;
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValue;

    CD3D12SwapChain *m_rt = nullptr;
    CD3D12Scene *m_scene = nullptr;

public:
    Impl()
        : m_rt(new CD3D12SwapChain), m_scene(new CD3D12Scene)
    {
    }

    ~Impl()
    {
        // Ensure that the GPU is no longer referencing resources that are about to be
        // cleaned up by the destructor.
        WaitForPreviousFrame();

        delete m_scene;
        delete m_rt;
        CloseHandle(m_fenceEvent);
    }

    void OnInit(HWND hwnd, bool useWarpDevice)
    {
        ComPtr<IDXGIFactory4> factory;
        ThrowIfFailed(CreateDXGIFactory2(GetDxgiFactoryFlags(), IID_PPV_ARGS(&factory)));

        LoadPipeline(factory, useWarpDevice);
        m_rt->Initialize(factory, m_commandQueue, hwnd);
        m_scene->Initialize(m_device);
    }

    void OnSize(HWND hwnd, UINT width, UINT height)
    {
        WaitForPreviousFrame();
        m_rt->Resize(m_commandQueue, hwnd, width, height);
    }

    // Load the rendering pipeline dependencies.
    void LoadPipeline(const ComPtr<IDXGIFactory4> &factory, bool useWarpDevice)
    {
        if (useWarpDevice)
        {
            ComPtr<IDXGIAdapter> warpAdapter;
            ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));
            ThrowIfFailed(D3D12CreateDevice(
                warpAdapter.Get(),
                D3D_FEATURE_LEVEL_11_0,
                IID_PPV_ARGS(&m_device)));
        }
        else
        {
            ComPtr<IDXGIAdapter1> hardwareAdapter = GetHardwareAdapter(factory.Get());
            ThrowIfFailed(D3D12CreateDevice(
                hardwareAdapter.Get(),
                D3D_FEATURE_LEVEL_11_0,
                IID_PPV_ARGS(&m_device)));
        }

        // Describe and create the command queue.
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

        // Create synchronization objects and wait until assets have been uploaded to the GPU.
        {
            ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
            m_fenceValue = 1;

            // Create an event handle to use for frame synchronization.
            m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            if (m_fenceEvent == nullptr)
            {
                ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
            }

            // Wait for the command list to execute; we are reusing the same command
            // list in our main loop but for now, we just want to wait for setup to
            // complete before continuing.
            WaitForPreviousFrame();
        }
    }

    // Render the scene.
    void OnRender()
    {
        m_rt->Prepare(m_device);
        auto commandList = m_scene->Update(m_rt);
        m_commandQueue->ExecuteCommandLists(1, commandList.GetAddressOf());
        m_rt->Present();
        WaitForPreviousFrame();
    }

    void WaitForPreviousFrame()
    {
        // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
        // This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
        // sample illustrates how to use fences for efficient resource usage and to
        // maximize GPU utilization.

        // Signal and increment the fence value.
        const UINT64 fence = m_fenceValue;
        ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
        m_fenceValue++;

        // Wait until the previous frame is finished.
        if (m_fence->GetCompletedValue() < fence)
        {
            ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }
    }
};

D3D12HelloConstBuffers::D3D12HelloConstBuffers()
    : m_impl(new Impl)
{
}

D3D12HelloConstBuffers::~D3D12HelloConstBuffers()
{
    delete m_impl;
}

void D3D12HelloConstBuffers::OnSize(HWND hwnd, UINT width, UINT height)
{
    m_impl->OnSize(hwnd, width, height);
}

void D3D12HelloConstBuffers::OnInit(HWND hwnd, bool useWarpDevice)
{
    m_impl->OnInit(hwnd, useWarpDevice);
}

void D3D12HelloConstBuffers::OnFrame()
{
    m_impl->OnRender();
}

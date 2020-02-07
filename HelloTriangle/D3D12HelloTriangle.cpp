#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers.
#endif

#include <string>
const std::string g_shaders =
#include "shaders.hlsl"
    ;

#include "D3D12HelloTriangle.h"
#include "util.h"
#include "Scene.h"
#include "CommandBuilder.h"
#include <windows.h>
#include <DirectXMath.h>

class Impl
{
    bool m_useWarpDevice = false;

    Fence m_fence;
    Swapchain m_swapchain;
    CommandBuilder m_builder;
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    int m_width = 0;
    int m_height = 0;

public:
    Impl(bool useWarpDevice, UINT frameCount)
        : m_useWarpDevice(useWarpDevice), m_swapchain(frameCount)
    {
    }

    ~Impl()
    {
        // Ensure that the GPU is no longer referencing resources that are about to be
        // cleaned up by the destructor.
        m_fence.Wait(m_commandQueue);
    }

    void SetSize(int w, int h)
    {
        // TODO: update swapchain
        m_builder.SetSize(w, h);
    }

    void Render(HWND hWnd)
    {
        if (!m_device)
        {
            auto factory = CreateFactory();
            // create deice and device queue
            Initialize(factory);
            // create swapchain and rendertargets
            m_swapchain.Initialize(factory, m_commandQueue, hWnd, m_width, m_height);
            m_swapchain.CreateRenderTargets(m_device);
            // pipeline
            m_builder.Initialize(m_device);
            // Create synchronization objects and wait until assets have been uploaded to the GPU.
            m_fence.Initialize(m_device);
        }

        // Record all the commands we need to render the scene into the command list.
        auto commandList = m_builder.PopulateCommandList(
            m_swapchain.CurrentRTV(), m_swapchain.CurrentHandle());

        // Execute the command list.
        m_commandQueue->ExecuteCommandLists(1, commandList.GetAddressOf());

        // Present the frame.
        m_swapchain.Present();

        // sync
        m_fence.Wait(m_commandQueue);
    }

private:
    void Initialize(const ComPtr<IDXGIFactory4> &factory)
    {
        if (m_useWarpDevice)
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
            auto hardwareAdapter = GetHardwareAdapter(factory);

            ThrowIfFailed(D3D12CreateDevice(
                hardwareAdapter.Get(),
                D3D_FEATURE_LEVEL_11_0,
                IID_PPV_ARGS(&m_device)));
        }

        // Describe and create the command queue.
        D3D12_COMMAND_QUEUE_DESC queueDesc = {
            .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
            .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
        };
        ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));
    }
};

D3D12HelloTriangle::D3D12HelloTriangle(bool useWarpDevice, UINT frameCount)
    : m_impl(new Impl(useWarpDevice, frameCount))
{
}

D3D12HelloTriangle::~D3D12HelloTriangle()
{
    delete m_impl;
}

void D3D12HelloTriangle::Render(void *hWnd)
{
    m_impl->Render((HWND)hWnd);
}

void D3D12HelloTriangle::SetSize(int w, int h)
{
    m_impl->SetSize(w, h);
}

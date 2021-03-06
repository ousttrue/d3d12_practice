#include "D3DRenderer.h"
//#define DX12_ENABLE_DEBUG_LAYER
#include <assert.h>

struct FrameContext
{
    ComPtr<ID3D12CommandAllocator> CommandAllocator;
    UINT64 FenceValue = 0;
    ComPtr<ID3D12Resource> g_mainRenderTargetResource;
    D3D12_CPU_DESCRIPTOR_HANDLE g_rtvHandle = {};

    bool Initialize(const ComPtr<ID3D12Device> &device, const ComPtr<ID3D12DescriptorHeap> &rtvHeap, int i)
    {
        if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&CommandAllocator))))
        {
            return false;
        }

        auto rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        auto rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        rtvHandle.ptr += i * rtvDescriptorSize;
        g_rtvHandle = rtvHandle;

        return true;
    }
};

class Impl
{
private:
    std::vector<FrameContext> g_frameContext;
    UINT g_frameIndex = 0;

    ComPtr<ID3D12Device> g_pd3dDevice;
    ComPtr<ID3D12DescriptorHeap> g_pd3dRtvDescHeap;
    ComPtr<ID3D12DescriptorHeap> g_pd3dSrvDescHeap;
    ComPtr<ID3D12CommandQueue> g_pd3dCommandQueue;
    ComPtr<ID3D12GraphicsCommandList> g_pd3dCommandList;
    ComPtr<ID3D12Fence> g_fence;
    HANDLE g_fenceEvent = NULL;
    UINT64 g_fenceLastSignaledValue = 0;
    ComPtr<IDXGISwapChain3> g_pSwapChain;
    HANDLE g_hSwapChainWaitableObject = NULL;
    D3D12_RESOURCE_BARRIER barrier = {};

public:
    Impl(int frameCount)
        : g_frameContext(frameCount)
    {
    }
    ~Impl()
    {
        WaitForLastSubmittedFrame();
        if (g_hSwapChainWaitableObject != NULL)
        {
            CloseHandle(g_hSwapChainWaitableObject);
            g_hSwapChainWaitableObject = NULL;
        }
        if (g_fenceEvent)
        {
            CloseHandle(g_fenceEvent);
            g_fenceEvent = NULL;
        }
    }
    ID3D12Device *Device()
    {
        return g_pd3dDevice.Get();
    }
    ID3D12DescriptorHeap *SrvHeap() { return g_pd3dSrvDescHeap.Get(); }
    ID3D12GraphicsCommandList *CommandList() { return g_pd3dCommandList.Get(); }
    void OnSize(HWND hWnd, UINT w, UINT h)
    {
        if (g_pd3dDevice)
        {
            CleanupRenderTarget();
            ResizeSwapChain(hWnd, w, h);
            CreateRenderTarget();
        }
    }

    bool CreateDeviceD3D(HWND hWnd)
    {

        if (FAILED(D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&g_pd3dDevice))))
        {
            return false;
        }

        {
            D3D12_COMMAND_QUEUE_DESC desc{
                .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
                .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
                .NodeMask = 1,
            };
            if (FAILED(g_pd3dDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&g_pd3dCommandQueue))))
            {
                return false;
            }
        }

        if (FAILED(g_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence))))
        {
            return false;
        }
        g_fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (!g_fenceEvent)
        {
            return false;
        }

        {
            D3D12_DESCRIPTOR_HEAP_DESC desc{
                .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
                .NumDescriptors = (UINT)g_frameContext.size(),
                .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
                .NodeMask = 1,
            };
            if (FAILED(g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dRtvDescHeap))))
            {
                return false;
            }
        }

        {
            D3D12_DESCRIPTOR_HEAP_DESC desc{
                .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                .NumDescriptors = 1,
                .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
            };
            if (FAILED(g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap))))
            {
                return false;
            }
        }

        for (UINT i = 0; i < g_frameContext.size(); i++)
        {
            if (!g_frameContext[i].Initialize(g_pd3dDevice, g_pd3dRtvDescHeap, i))
            {
                return false;
            }
        }

        if (FAILED(g_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_frameContext[0].CommandAllocator.Get(), NULL, IID_PPV_ARGS(&g_pd3dCommandList))))
        {
            return false;
        }
        if (FAILED(g_pd3dCommandList->Close()))
        {
            return false;
        }

        {
            // Setup swap chain
            DXGI_SWAP_CHAIN_DESC1 sd{
                .Width = 0,
                .Height = 0,
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                .Stereo = FALSE,
                .SampleDesc = {
                    .Count = 1,
                    .Quality = 0,
                },
                .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
                .BufferCount = (UINT)g_frameContext.size(),
                .Scaling = DXGI_SCALING_STRETCH,
                .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
                .AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
                .Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT,
            };
            ComPtr<IDXGIFactory4> dxgiFactory;
            if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory))))
            {
                return false;
            }
            ComPtr<IDXGISwapChain1> swapChain1;
            if (FAILED(dxgiFactory->CreateSwapChainForHwnd(g_pd3dCommandQueue.Get(), hWnd, &sd, NULL, NULL, &swapChain1)))
            {
                return false;
            }
            if (FAILED(swapChain1.As(&g_pSwapChain)))
            {
                return false;
            }
            g_pSwapChain->SetMaximumFrameLatency((UINT)g_frameContext.size());
            g_hSwapChainWaitableObject = g_pSwapChain->GetFrameLatencyWaitableObject();
        }

        CreateRenderTarget();
        return true;
    }

    void CreateRenderTarget()
    {
        for (UINT i = 0; i < g_frameContext.size(); i++)
        {
            ComPtr<ID3D12Resource> pBackBuffer;
            g_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
            g_pd3dDevice->CreateRenderTargetView(pBackBuffer.Get(), NULL, g_frameContext[i].g_rtvHandle);
            g_frameContext[i].g_mainRenderTargetResource = pBackBuffer;
        }
    }

    void WaitForLastSubmittedFrame()
    {
        FrameContext *frameCtxt = &g_frameContext[g_frameIndex % g_frameContext.size()];

        UINT64 fenceValue = frameCtxt->FenceValue;
        if (fenceValue == 0)
            return; // No fence was signaled

        frameCtxt->FenceValue = 0;
        if (g_fence->GetCompletedValue() >= fenceValue)
            return;

        g_fence->SetEventOnCompletion(fenceValue, g_fenceEvent);
        WaitForSingleObject(g_fenceEvent, INFINITE);
    }

    void CleanupRenderTarget()
    {
        WaitForLastSubmittedFrame();

        for (UINT i = 0; i < g_frameContext.size(); i++)
        {
            if (g_frameContext[i].g_mainRenderTargetResource)
            {
                g_frameContext[i].g_mainRenderTargetResource.Reset();
            }
        }
    }

    FrameContext *WaitForNextFrameResources()
    {
        UINT nextFrameIndex = g_frameIndex + 1;
        g_frameIndex = nextFrameIndex;

        HANDLE waitableObjects[] = {g_hSwapChainWaitableObject, NULL};
        DWORD numWaitableObjects = 1;

        FrameContext *frameCtxt = &g_frameContext[nextFrameIndex % g_frameContext.size()];
        UINT64 fenceValue = frameCtxt->FenceValue;
        if (fenceValue != 0) // means no fence was signaled
        {
            frameCtxt->FenceValue = 0;
            g_fence->SetEventOnCompletion(fenceValue, g_fenceEvent);
            waitableObjects[1] = g_fenceEvent;
            numWaitableObjects = 2;
        }

        WaitForMultipleObjects(numWaitableObjects, waitableObjects, TRUE, INFINITE);

        return frameCtxt;
    }

    void ResizeSwapChain(HWND hWnd, int width, int height)
    {
        DXGI_SWAP_CHAIN_DESC1 sd;
        g_pSwapChain->GetDesc1(&sd);
        sd.Width = width;
        sd.Height = height;

        ComPtr<IDXGIFactory4> dxgiFactory;
        g_pSwapChain->GetParent(IID_PPV_ARGS(&dxgiFactory));

        ////////////////////
        // release !
        ////////////////////
        g_pSwapChain.Reset();
        CloseHandle(g_hSwapChainWaitableObject);

        ComPtr<IDXGISwapChain1> swapChain1;
        dxgiFactory->CreateSwapChainForHwnd(g_pd3dCommandQueue.Get(), hWnd, &sd, NULL, NULL, &swapChain1);
        swapChain1->QueryInterface(IID_PPV_ARGS(&g_pSwapChain));

        g_pSwapChain->SetMaximumFrameLatency((UINT)g_frameContext.size());

        g_hSwapChainWaitableObject = g_pSwapChain->GetFrameLatencyWaitableObject();
        assert(g_hSwapChainWaitableObject != NULL);
    }

    FrameContext *Begin(const float *clear_color)
    {
        auto frameCtxt = WaitForNextFrameResources();
        UINT backBufferIdx = g_pSwapChain->GetCurrentBackBufferIndex();
        frameCtxt->CommandAllocator->Reset();
        g_pd3dCommandList->Reset(frameCtxt->CommandAllocator.Get(), NULL);

        // barrier
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = g_frameContext[backBufferIdx].g_mainRenderTargetResource.Get();
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        g_pd3dCommandList->ResourceBarrier(1, &barrier);

        g_pd3dCommandList->ClearRenderTargetView(g_frameContext[backBufferIdx].g_rtvHandle, clear_color, 0, NULL);
        g_pd3dCommandList->OMSetRenderTargets(1, &g_frameContext[backBufferIdx].g_rtvHandle, FALSE, NULL);
        g_pd3dCommandList->SetDescriptorHeaps(1, g_pd3dSrvDescHeap.GetAddressOf());

        return frameCtxt;
    }

    void End(FrameContext *frameCtxt)
    {
        // barrier
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        g_pd3dCommandList->ResourceBarrier(1, &barrier);

        // execute
        g_pd3dCommandList->Close();
        g_pd3dCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList *const *)g_pd3dCommandList.GetAddressOf());

        // present sync
        g_pSwapChain->Present(1, 0); // Present with vsync
        //g_pSwapChain->Present(0, 0); // Present without vsync
        UINT64 fenceValue = g_fenceLastSignaledValue + 1;
        g_pd3dCommandQueue->Signal(g_fence.Get(), fenceValue);
        g_fenceLastSignaledValue = fenceValue;
        frameCtxt->FenceValue = fenceValue;
    }
};

//////////////////////////////////////////////////////////////////////////////
D3DRenderer::D3DRenderer(int frameCount)
    : m_impl(new Impl(frameCount))
{
}

D3DRenderer::~D3DRenderer()
{
    delete m_impl;
}

ID3D12Device *D3DRenderer::Device() { return m_impl->Device(); }
ID3D12DescriptorHeap *D3DRenderer::SrvHeap() { return m_impl->SrvHeap(); }
ID3D12GraphicsCommandList *D3DRenderer::CommandList() { return m_impl->CommandList(); }
bool D3DRenderer::CreateDeviceD3D(HWND hWnd)
{
    return m_impl->CreateDeviceD3D(hWnd);
}
void D3DRenderer::OnSize(HWND hWnd, UINT w, UINT h)
{
    m_impl->OnSize(hWnd, w, h);
}
void D3DRenderer::WaitForLastSubmittedFrame()
{
    m_impl->WaitForLastSubmittedFrame();
}
FrameContext *D3DRenderer::Begin(const float *clear_color)
{
    return m_impl->Begin(clear_color);
}
void D3DRenderer::End(FrameContext *frameCtxt)
{
    m_impl->End(frameCtxt);
}

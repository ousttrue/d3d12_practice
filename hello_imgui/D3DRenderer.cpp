#include "D3DRenderer.h"
//#define DX12_ENABLE_DEBUG_LAYER
#include <assert.h>

#ifdef DX12_ENABLE_DEBUG_LAYER
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

struct FrameContext
{
    ComPtr<ID3D12CommandAllocator> CommandAllocator;
    UINT64 FenceValue = 0;
    ComPtr<ID3D12Resource> g_mainRenderTargetResource;
    D3D12_CPU_DESCRIPTOR_HANDLE g_mainRenderTargetDescriptor = {};
};

class Impl
{
private:
    std::vector<FrameContext> g_frameContext;
    UINT g_frameIndex = 0;

    ID3D12Device *g_pd3dDevice = NULL;
    ID3D12DescriptorHeap *g_pd3dRtvDescHeap = NULL;
    ID3D12DescriptorHeap *g_pd3dSrvDescHeap = NULL;
    ID3D12CommandQueue *g_pd3dCommandQueue = NULL;
    ID3D12GraphicsCommandList *g_pd3dCommandList = NULL;
    ID3D12Fence *g_fence = NULL;
    HANDLE g_fenceEvent = NULL;
    UINT64 g_fenceLastSignaledValue = 0;
    IDXGISwapChain3 *g_pSwapChain = NULL;
    HANDLE g_hSwapChainWaitableObject = NULL;
    D3D12_RESOURCE_BARRIER barrier = {};

public:
    Impl(int frameCount)
        : g_frameContext(frameCount)
    {
    }
    ~Impl()
    {
        CleanupRenderTarget();
        if (g_pSwapChain)
        {
            g_pSwapChain->Release();
            g_pSwapChain = NULL;
        }
        if (g_hSwapChainWaitableObject != NULL)
        {
            CloseHandle(g_hSwapChainWaitableObject);
        }
        for (UINT i = 0; i < g_frameContext.size(); i++)
            if (g_frameContext[i].CommandAllocator)
            {
                g_frameContext[i].CommandAllocator.Reset();
            }
        if (g_pd3dCommandQueue)
        {
            g_pd3dCommandQueue->Release();
            g_pd3dCommandQueue = NULL;
        }
        if (g_pd3dCommandList)
        {
            g_pd3dCommandList->Release();
            g_pd3dCommandList = NULL;
        }
        if (g_pd3dRtvDescHeap)
        {
            g_pd3dRtvDescHeap->Release();
            g_pd3dRtvDescHeap = NULL;
        }
        if (g_pd3dSrvDescHeap)
        {
            g_pd3dSrvDescHeap->Release();
            g_pd3dSrvDescHeap = NULL;
        }
        if (g_fence)
        {
            g_fence->Release();
            g_fence = NULL;
        }
        if (g_fenceEvent)
        {
            CloseHandle(g_fenceEvent);
            g_fenceEvent = NULL;
        }
        if (g_pd3dDevice)
        {
            g_pd3dDevice->Release();
            g_pd3dDevice = NULL;
        }

#ifdef DX12_ENABLE_DEBUG_LAYER
        {
            ComPtr<IDXGIDebug1> pDebug;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
            {
                pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
            }
        }
#endif
    }
    ID3D12Device *Device()
    {
        return g_pd3dDevice;
    }
    ID3D12DescriptorHeap *SrvHeap() { return g_pd3dSrvDescHeap; }
    ID3D12GraphicsCommandList *CommandList() { return g_pd3dCommandList; }
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
        // Setup swap chain
        DXGI_SWAP_CHAIN_DESC1 sd;
        {
            ZeroMemory(&sd, sizeof(sd));
            sd.BufferCount = (UINT)g_frameContext.size();
            sd.Width = 0;
            sd.Height = 0;
            sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
            sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            sd.SampleDesc.Count = 1;
            sd.SampleDesc.Quality = 0;
            sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
            sd.Scaling = DXGI_SCALING_STRETCH;
            sd.Stereo = FALSE;
        }

#ifdef DX12_ENABLE_DEBUG_LAYER
        {
            ComPtr<ID3D12Debug> pdx12Debug;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pdx12Debug))))
            {
                pdx12Debug->EnableDebugLayer();
            }
        }
#endif

        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
        if (D3D12CreateDevice(NULL, featureLevel, IID_PPV_ARGS(&g_pd3dDevice)) != S_OK)
            return false;

        {
            D3D12_DESCRIPTOR_HEAP_DESC desc = {};
            desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            desc.NumDescriptors = (UINT)g_frameContext.size();
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            desc.NodeMask = 1;
            if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dRtvDescHeap)) != S_OK)
                return false;

            SIZE_T rtvDescriptorSize = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_pd3dRtvDescHeap->GetCPUDescriptorHandleForHeapStart();
            for (UINT i = 0; i < g_frameContext.size(); i++)
            {
                g_frameContext[i].g_mainRenderTargetDescriptor = rtvHandle;
                rtvHandle.ptr += rtvDescriptorSize;
            }
        }

        {
            D3D12_DESCRIPTOR_HEAP_DESC desc = {};
            desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            desc.NumDescriptors = 1;
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap)) != S_OK)
                return false;
        }

        {
            D3D12_COMMAND_QUEUE_DESC desc = {};
            desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            desc.NodeMask = 1;
            if (g_pd3dDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&g_pd3dCommandQueue)) != S_OK)
                return false;
        }

        for (UINT i = 0; i < g_frameContext.size(); i++)
            if (g_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_frameContext[i].CommandAllocator)) != S_OK)
                return false;

        if (g_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_frameContext[0].CommandAllocator.Get(), NULL, IID_PPV_ARGS(&g_pd3dCommandList)) != S_OK ||
            g_pd3dCommandList->Close() != S_OK)
            return false;

        if (g_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence)) != S_OK)
            return false;

        g_fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (g_fenceEvent == NULL)
            return false;

        {
            ComPtr<IDXGIFactory4> dxgiFactory;
            ComPtr<IDXGISwapChain1> swapChain1;
            if (CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)) != S_OK ||
                dxgiFactory->CreateSwapChainForHwnd(g_pd3dCommandQueue, hWnd, &sd, NULL, NULL, &swapChain1) != S_OK ||
                swapChain1->QueryInterface(IID_PPV_ARGS(&g_pSwapChain)) != S_OK)
                return false;
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
            g_pd3dDevice->CreateRenderTargetView(pBackBuffer.Get(), NULL, g_frameContext[i].g_mainRenderTargetDescriptor);
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
            if (g_frameContext[i].g_mainRenderTargetResource)
            {
                g_frameContext[i].g_mainRenderTargetResource.Reset();
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

        g_pSwapChain->Release();
        CloseHandle(g_hSwapChainWaitableObject);

        ComPtr<IDXGISwapChain1> swapChain1;
        dxgiFactory->CreateSwapChainForHwnd(g_pd3dCommandQueue, hWnd, &sd, NULL, NULL, &swapChain1);
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

        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = g_frameContext[backBufferIdx].g_mainRenderTargetResource.Get();
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

        g_pd3dCommandList->Reset(frameCtxt->CommandAllocator.Get(), NULL);
        g_pd3dCommandList->ResourceBarrier(1, &barrier);
        g_pd3dCommandList->ClearRenderTargetView(g_frameContext[backBufferIdx].g_mainRenderTargetDescriptor, clear_color, 0, NULL);
        g_pd3dCommandList->OMSetRenderTargets(1, &g_frameContext[backBufferIdx].g_mainRenderTargetDescriptor, FALSE, NULL);
        g_pd3dCommandList->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);

        return frameCtxt;
    }

    void End(FrameContext *frameCtxt)
    {
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        g_pd3dCommandList->ResourceBarrier(1, &barrier);
        g_pd3dCommandList->Close();

        g_pd3dCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList *const *)&g_pd3dCommandList);

        g_pSwapChain->Present(1, 0); // Present with vsync
        //g_pSwapChain->Present(0, 0); // Present without vsync

        UINT64 fenceValue = g_fenceLastSignaledValue + 1;
        g_pd3dCommandQueue->Signal(g_fence, fenceValue);
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

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers.
#endif

#include "D3D12HelloTriangle.h"
#include "util.h"
#include <windows.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include "d3dx12.h"

const std::string g_shaders =
#include "shaders.hlsl"
    ;

using namespace DirectX;

// const UINT FrameCount = 2;

struct Vertex
{
    XMFLOAT3 position;
    XMFLOAT4 color;
};

class Impl
{
    Fence m_fence;
    Swapchain m_swapchain;

    bool m_useWarpDevice = false;
    // Viewport dimensions.
    float m_aspectRatio = 1.0f;

    // // Pipeline objects.
    D3D12_VIEWPORT m_viewport = {};
    D3D12_RECT m_scissorRect = {};
    ComPtr<ID3D12Device> m_device;
    std::vector<ComPtr<ID3D12Resource>> m_renderTargets;
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12PipelineState> m_pipelineState;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    UINT m_rtvDescriptorSize = 0;

    // // App resources.
    ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

    UINT m_frameCount;

public:
    Impl(bool useWarpDevice, UINT frameCount)
        : m_useWarpDevice(useWarpDevice), m_frameCount(frameCount)
    {
        m_viewport.MinDepth = D3D12_MIN_DEPTH;
        m_viewport.MaxDepth = D3D12_MAX_DEPTH;
    }

    ~Impl()
    {
        // Ensure that the GPU is no longer referencing resources that are about to be
        // cleaned up by the destructor.
        m_fence.Wait(m_commandQueue);
    }

    void SetSize(int w, int h)
    {
        if (w == m_viewport.Width && h == m_viewport.Height)
        {
            auto a = 0;
            return;
        }
        m_viewport.Width = static_cast<float>(w);
        m_viewport.Height = static_cast<float>(h);
        m_aspectRatio = static_cast<float>(w) / static_cast<float>(h);

        m_scissorRect.right = static_cast<LONG>(w);
        m_scissorRect.bottom = static_cast<LONG>(h);
    }

    void Render(HWND hWnd)
    {
        if (m_viewport.Width == 0 || m_viewport.Height == 0)
        {
            return;
        }

        if (!m_commandList)
        {
            OnInit(hWnd);
        }

        // Record all the commands we need to render the scene into the command list.
        PopulateCommandList(m_swapchain.FrameIndex());

        // Execute the command list.
        ID3D12CommandList *ppCommandLists[] = {m_commandList.Get()};
        m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

        // Present the frame.
        m_swapchain.Present();
        m_fence.Wait(m_commandQueue);
        m_swapchain.UpdateFrameIndex();
    }

private:
    void PopulateCommandList(UINT frameIndex)
    {
        // Command list allocators can only be reset when the associated
        // command lists have finished execution on the GPU; apps should use
        // fences to determine GPU execution progress.
        ThrowIfFailed(m_commandAllocator->Reset());

        // However, when ExecuteCommandList() is called on a particular command
        // list, that command list can then be reset at any time and must be before
        // re-recording.
        ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));

        // Set necessary state.
        m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
        m_commandList->RSSetViewports(1, &m_viewport);
        m_commandList->RSSetScissorRects(1, &m_scissorRect);

        // Indicate that the back buffer will be used as a render target.
        m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[frameIndex].Get(),
                                                                                D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, m_rtvDescriptorSize);
        m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

        // Record commands.
        const float clearColor[] = {0.0f, 0.2f, 0.4f, 1.0f};
        m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
        m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
        m_commandList->DrawInstanced(3, 1, 0, 0);

        // Indicate that the back buffer will now be used to present.
        m_commandList->ResourceBarrier(1,
                                       &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

        ThrowIfFailed(m_commandList->Close());
    }

    // void WaitForPreviousFrame()
    // {
    //     // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
    //     // This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
    //     // sample illustrates how to use fences for efficient resource usage and to
    //     // maximize GPU utilization.

    //     m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    // }

private:
    // D3D12HelloTriangle(UINT width, UINT height, std::wstring name);
    // ~D3D12HelloTriangle();

    void OnInit(HWND hWnd)
    {
        LoadPipeline(hWnd);
        LoadAssets();
    }

    void LoadPipeline(HWND hWnd)
    {
        auto factory = CreateFactory();

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

        m_swapchain.Initialize(factory, m_commandQueue, m_frameCount,
                               hWnd, (UINT)m_viewport.Width, (UINT)m_viewport.Height);

        // Create descriptor heaps.
        {
            // Describe and create a render target view (RTV) descriptor heap.
            D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
            rtvHeapDesc.NumDescriptors = m_frameCount;
            rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

            m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        }

        // Create frame resources.
        {
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

            // Create a RTV for each frame.
            for (UINT n = 0; n < m_frameCount; n++)
            {
                auto resource = m_swapchain.GetResource(n);
                m_device->CreateRenderTargetView(resource.Get(), nullptr, rtvHandle);
                rtvHandle.Offset(1, m_rtvDescriptorSize);
                m_renderTargets.push_back(resource);
            }
        }

        ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
    }

    // Load the sample assets.
    void
    LoadAssets()
    {
        // Create an empty root signature.
        {
            CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
            rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

            ComPtr<ID3DBlob> signature;
            ComPtr<ID3DBlob> error;
            ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
            ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
        }

        // Create the pipeline state, which includes compiling and loading shaders.
        {
            ComPtr<ID3DBlob> vertexShader;
            ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
            // Enable better shader debugging with the graphics debugging tools.
            UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
            UINT compileFlags = 0;
#endif

            // ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
            // ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));
            ThrowIfFailed(D3DCompile(g_shaders.data(), g_shaders.size(), "shader", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
            ThrowIfFailed(D3DCompile(g_shaders.data(), g_shaders.size(), "shader", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

            // Define the vertex input layout.
            D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
                {
                    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                    {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

            // Describe and create the graphics pipeline state object (PSO).
            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
            psoDesc.InputLayout = {inputElementDescs, _countof(inputElementDescs)};
            psoDesc.pRootSignature = m_rootSignature.Get();
            psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
            psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
            psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
            psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
            psoDesc.DepthStencilState.DepthEnable = FALSE;
            psoDesc.DepthStencilState.StencilEnable = FALSE;
            psoDesc.SampleMask = UINT_MAX;
            psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            psoDesc.NumRenderTargets = 1;
            psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
            psoDesc.SampleDesc.Count = 1;
            ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
        }

        // Create the command list.
        ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));

        // Command lists are created in the recording state, but there is nothing
        // to record yet. The main loop expects it to be closed, so close it now.
        ThrowIfFailed(m_commandList->Close());

        // Create the vertex buffer.
        {
            // Define the geometry for a triangle.
            Vertex triangleVertices[] =
                {
                    {{0.0f, 0.25f * m_aspectRatio, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
                    {{0.25f, -0.25f * m_aspectRatio, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
                    {{-0.25f, -0.25f * m_aspectRatio, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}};

            const UINT vertexBufferSize = sizeof(triangleVertices);

            // Note: using upload heaps to transfer static data like vert buffers is not
            // recommended. Every time the GPU needs it, the upload heap will be marshalled
            // over. Please read up on Default Heap usage. An upload heap is used here for
            // code simplicity and because there are very few verts to actually transfer.
            ThrowIfFailed(m_device->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_vertexBuffer)));

            // Copy the triangle data to the vertex buffer.
            UINT8 *pVertexDataBegin;
            CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.
            ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void **>(&pVertexDataBegin)));
            memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
            m_vertexBuffer->Unmap(0, nullptr);

            // Initialize the vertex buffer view.
            m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
            m_vertexBufferView.StrideInBytes = sizeof(Vertex);
            m_vertexBufferView.SizeInBytes = vertexBufferSize;
        }

        // Create synchronization objects and wait until assets have been uploaded to the GPU.
        m_fence.Initialize(m_device);
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

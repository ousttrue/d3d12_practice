#include "D3D12HelloConstBuffers.h"
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include "d3dx12.h"
#include "d3dhelper.h"
#include "CD3D12SwapChain.h"
#include "CD3D12Scene.h"
#include "CommandList.h"
#include "Uploader.h"
#include "ResourceItem.h"
#include "CD3D12CommandQueue.h"
#include "Mesh.h"
#include "ScreenState.h"
#include <list>
#include <functional>

struct Vertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT4 color;
};
// Define the geometry for a triangle.
Vertex VERTICES[] =
    {
        {{0.0f, 0.25f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{0.25f, -0.25f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{-0.25f, -0.25f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}};
const UINT VERTICES_BYTE_SIZE = sizeof(VERTICES);
const UINT VERTEX_STRIDE = sizeof(VERTICES[0]);
UINT INDICES[] =
    {
        0, 1, 2};
const UINT INDICES_BYTE_SIZE = sizeof(INDICES);
const UINT INDEX_STRIDE = sizeof(INDICES[0]);


class Impl
{
    // Pipeline objects.
    ComPtr<ID3D12Device> m_device;

    CD3D12SwapChain *m_rt = nullptr;
    CD3D12Scene *m_scene = nullptr;
    Uploader *m_uploader = nullptr;
    CD3D12CommandQueue *m_queue = nullptr;

public:
    Impl()
        : m_rt(new CD3D12SwapChain),
          m_scene(new CD3D12Scene),
          m_uploader(new Uploader),
          m_queue(new CD3D12CommandQueue)
    {
    }

    ~Impl()
    {
        m_queue->SyncFence();
        delete m_uploader;
        delete m_scene;
        delete m_rt;
        delete m_queue;
    }

    void OnInit(HWND hwnd, bool useWarpDevice)
    {
        ComPtr<IDXGIFactory4> factory;
        ThrowIfFailed(CreateDXGIFactory2(GetDxgiFactoryFlags(), IID_PPV_ARGS(&factory)));

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

        m_queue->Initialize(m_device);
        m_rt->Initialize(factory, m_queue->CommandQueue(), hwnd);
        m_scene->Initialize(m_device);
        m_uploader->Initialize(m_device);

        // Create the vertex buffer.
        auto mesh = std::make_shared<Mesh>();
        m_scene->Mesh(mesh);

        auto isDynamic = false;
        if (isDynamic)
        {
            {
                auto vertexBuffer = ResourceItem::CreateUpload(m_device, VERTICES_BYTE_SIZE);
                vertexBuffer->MapCopyUnmap(VERTICES, VERTICES_BYTE_SIZE, VERTEX_STRIDE);
                mesh->VertexBuffer(vertexBuffer);
            }
            {
                auto indexBuffer = ResourceItem::CreateUpload(m_device, INDICES_BYTE_SIZE);
                indexBuffer->MapCopyUnmap(INDICES, INDICES_BYTE_SIZE, INDEX_STRIDE);
                mesh->IndexBuffer(indexBuffer);
            }
        }
        else
        {
            {
                auto vertexBuffer = ResourceItem::CreateDefault(m_device, VERTICES_BYTE_SIZE);
                m_uploader->EnqueueUpload({vertexBuffer, VERTICES, VERTICES_BYTE_SIZE, VERTEX_STRIDE});
                mesh->VertexBuffer(vertexBuffer);
            }
            {
                auto indexBuffer = ResourceItem::CreateDefault(m_device, INDICES_BYTE_SIZE);
                m_uploader->EnqueueUpload({indexBuffer, INDICES, INDICES_BYTE_SIZE, INDEX_STRIDE});
                mesh->IndexBuffer(indexBuffer);
            }
        }
    }

    ScreenState m_lastState = {};
    void OnFrame(HWND hWnd, const ScreenState &state)
    {
        if (m_lastState.Width != state.Width || m_lastState.Height != state.Height)
        {
            OnSize(hWnd, state.Width, state.Height);
        }
        m_lastState = state;
        OnRender();
    }

    void OnSize(HWND hwnd, UINT width, UINT height)
    {
        m_queue->SyncFence();
        m_rt->Resize(m_queue->CommandQueue(), hwnd, width, height);
        m_scene->UpdateProjection(m_rt->AspectRatio());
    }

    // Render the scene.
    void OnRender()
    {
        m_uploader->Update(m_device);

        m_rt->Prepare(m_device);
        auto commandList = m_scene->Update(m_rt);

        auto callbacks = commandList->Close();
        m_queue->Execute(commandList->Get());
        m_rt->Present();
        m_queue->SyncFence(callbacks);
    }
};

D3D12HelloConstBuffers::D3D12HelloConstBuffers(bool useWarpDevice)
    : m_useWarpDevice(useWarpDevice)
{
}

D3D12HelloConstBuffers::~D3D12HelloConstBuffers()
{
    if (m_impl)
    {
        delete m_impl;
    }
}

void D3D12HelloConstBuffers::OnFrame(void *hwnd, const struct ScreenState &state)
{
    if (!m_impl)
    {
        m_impl = new Impl();
        m_impl->OnInit((HWND)hwnd, m_useWarpDevice);
    }
    m_impl->OnFrame((HWND)hwnd, state);
}

#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <memory>

class CD3D12Scene
{
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12DescriptorHeap> m_cbvHeap;
    ComPtr<ID3D12PipelineState> m_pipelineState;
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    // App resources.
    std::shared_ptr<class ResourceItem> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView{};
    ComPtr<ID3D12Resource> m_indexBuffer;
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView{};
    ComPtr<ID3D12Resource> m_constantBuffer;

    // keep
    ComPtr<ID3D12Resource> m_upload;

    float m_near = 0.1f;
    float m_far = 10.0f;
    float m_fovY = 30.0f / 180.0f * DirectX::XM_PI;

    float m_x = 0;
    struct SceneConstantBuffer
    {
        DirectX::XMFLOAT4X4 world;
        DirectX::XMFLOAT4X4 view;
        DirectX::XMFLOAT4X4 projection;
    };
    SceneConstantBuffer m_constantBufferData{};
    UINT8 *m_pCbvDataBegin = nullptr;

public:
    bool Initialize(const ComPtr<ID3D12Device> &device);
    ComPtr<ID3D12CommandList> Update(class CD3D12SwapChain *rt)
    {
        OnUpdate();

        // Record all the commands we need to render the scene into the command list.
        return PopulateCommandList(rt);
    }
    void UpdateProjection(float aspectRatio);
    ComPtr<ID3D12CommandList> SetVertices(const ComPtr<ID3D12Device> &device,
                                          const void *vertices, UINT vertexBytes, UINT vertexStride,
                                          const void *indices, UINT indexBytes, DXGI_FORMAT indexStride,
                                          bool isDynamic);

private:
    void OnUpdate();
    ComPtr<ID3D12CommandList> PopulateCommandList(class CD3D12SwapChain *rt);
};

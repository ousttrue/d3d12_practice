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
    class CommandList *m_commandList = nullptr;
    // App resources.
    std::shared_ptr<class ResourceItem> m_vertexBuffer;
    // D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView{};
    std::shared_ptr<class ResourceItem> m_indexBuffer;
    // D3D12_INDEX_BUFFER_VIEW m_indexBufferView{};
    ComPtr<ID3D12Resource> m_constantBuffer;

    // keep
    std::shared_ptr<class ResourceItem> m_upload;

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
    CD3D12Scene();
    ~CD3D12Scene();
    const std::shared_ptr<class ResourceItem> &VertexBuffer() const { return m_vertexBuffer; }
    void VertexBuffer(const std::shared_ptr<ResourceItem> &item) { m_vertexBuffer = item; }
    bool Initialize(const ComPtr<ID3D12Device> &device);
    class CommandList *Update(class CD3D12SwapChain *rt)
    {
        OnUpdate();

        // Record all the commands we need to render the scene into the command list.
        return PopulateCommandList(rt);
    }
    void UpdateProjection(float aspectRatio);

private:
    void OnUpdate();
    class CommandList *PopulateCommandList(class CD3D12SwapChain *rt);
};

#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <DirectXMath.h>

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
    ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
    ComPtr<ID3D12Resource> m_constantBuffer;

    float m_x = 0;
    struct SceneConstantBuffer
    {
        DirectX::XMFLOAT4X4 world;
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

private:
    void OnUpdate();
    ComPtr<ID3D12CommandList> PopulateCommandList(class CD3D12SwapChain *rt);
};

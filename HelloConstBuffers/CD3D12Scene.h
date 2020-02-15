#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <memory>
#include "CD3D12ConstantBuffer.h"

class CD3D12Scene
{
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12DescriptorHeap> m_cbvHeap;
    ComPtr<ID3D12PipelineState> m_pipelineState;
    class CommandList *m_commandList = nullptr;
    std::shared_ptr<class Mesh> m_mesh;

    float m_x = 0;
    struct ModelConstantBuffer
    {
        DirectX::XMFLOAT4X4 world;
    };
    CD3D12ConstantBuffer<ModelConstantBuffer> m_modelConstant;

public:
    CD3D12Scene();
    ~CD3D12Scene();
    const ComPtr<ID3D12DescriptorHeap> &Heap() const { return m_cbvHeap; }
    void Mesh(const std::shared_ptr<Mesh> &mesh) { m_mesh = mesh; }
    bool Initialize(const ComPtr<ID3D12Device> &device);
    class CommandList *Update(class CD3D12SwapChain *rt)
    {
        OnUpdate();

        // Record all the commands we need to render the scene into the command list.
        return PopulateCommandList(rt);
    }

private:
    void OnUpdate();
    class CommandList *PopulateCommandList(class CD3D12SwapChain *rt);
};

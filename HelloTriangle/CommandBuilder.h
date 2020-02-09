#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>

class CommandBuilder
{
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    std::unique_ptr<class Scene> m_scene;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12PipelineState> m_pipelineState;
    ComPtr<ID3D12RootSignature> m_rootSignature;

    // Viewport dimensions.
    D3D12_VIEWPORT m_viewport = {
        .MinDepth = D3D12_MIN_DEPTH,
        .MaxDepth = D3D12_MAX_DEPTH,
    };
    float m_aspectRatio = 1.0f;
    D3D12_RECT m_scissorRect{};

public:
    CommandBuilder();
    void Initialize(const ComPtr<ID3D12Device> &device);
    void SetSize(int w, int h);
    ComPtr<ID3D12CommandList> PopulateCommandList(
        const ComPtr<ID3D12Resource> &rtv, const D3D12_CPU_DESCRIPTOR_HANDLE &rtvHandle);
};

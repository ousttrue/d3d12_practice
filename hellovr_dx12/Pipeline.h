#pragma once
#include <d3d12.h>
#include <wrl/client.h>

class Pipeline
{
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    int m_nMSAASampleCount;
    ComPtr<ID3D12RootSignature> m_pRootSignature;
    ComPtr<ID3D12PipelineState> m_pScenePipelineState;
    ComPtr<ID3D12PipelineState> m_pCompanionPipelineState;
    ComPtr<ID3D12PipelineState> m_pRenderModelPipelineState;
    ComPtr<ID3D12PipelineState> m_pAxesPipelineState;

public:
    Pipeline(int msaa);
    const ComPtr<ID3D12RootSignature> &RootSignature() const { return m_pRootSignature; }
    const ComPtr<ID3D12PipelineState> &SceneState() const { return m_pScenePipelineState; }
    const ComPtr<ID3D12PipelineState> &CompanionState() const { return m_pCompanionPipelineState; }
    const ComPtr<ID3D12PipelineState> &RenderModelState() const { return m_pRenderModelPipelineState; }
    const ComPtr<ID3D12PipelineState> &AxisState() const { return m_pAxesPipelineState; }

    //-----------------------------------------------------------------------------
    // Purpose: Creates all the shaders used by HelloVR DX12
    //-----------------------------------------------------------------------------
    bool CreateAllShaders(const ComPtr<ID3D12Device> &device);
};

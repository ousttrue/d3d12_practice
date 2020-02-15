#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include "CD3D12ConstantBuffer.h"

class Camera
{
    float m_near = 0.1f;
    float m_far = 10.0f;
    float m_fovY = 30.0f / 180.0f * DirectX::XM_PI;
    struct SceneConstantBuffer
    {
        DirectX::XMFLOAT4X4 view;
        DirectX::XMFLOAT4X4 projection;
    };
    CD3D12ConstantBuffer<SceneConstantBuffer> m_sceneConstant;

public:
    void Initialize(const ComPtr<ID3D12Device> &device,
                    const ComPtr<ID3D12DescriptorHeap> &cbvHeap);
    void UpdateProjection(float aspectRatio);
};

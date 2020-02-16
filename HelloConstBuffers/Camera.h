#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include "CD3D12ConstantBuffer.h"
#include "ScreenState.h"

class Camera
{
    // view
    float m_yaw = 0;
    float m_pitch = 0;
    DirectX::XMFLOAT3 m_translation = {0, 0, 7};

    // projection
    float m_near = 0.1f;
    float m_far = 100.0f;
    float m_fovY = 30.0f / 180.0f * DirectX::XM_PI;
    float m_aspectRatio = 1.0f;

    // buffer
    struct SceneConstantBuffer
    {
        DirectX::XMFLOAT4X4 view;
        DirectX::XMFLOAT4X4 projection;
    };
    CD3D12ConstantBuffer<SceneConstantBuffer> m_sceneConstant;

public:
    Camera();
    void Calc();
    void Initialize(const ComPtr<ID3D12Device> &device,
                    const ComPtr<ID3D12DescriptorHeap> &cbvHeap);
    void OnFrame(const ScreenState &state, const ScreenState &prev);
};

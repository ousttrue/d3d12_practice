#include "Camera.h"
using namespace DirectX;

Camera::Camera()
{
    Calc();
}

void Camera::Calc()
{
    {
        auto m = XMMatrixPerspectiveFovLH(m_fovY, m_aspectRatio, m_near, m_far);
        XMStoreFloat4x4(&m_sceneConstant.Data.projection, m);
    }

    {
        auto yaw = XMMatrixRotationY(m_yaw);
        auto pitch = XMMatrixRotationX(m_pitch);
        auto t = XMMatrixTranslation(m_translation.x, m_translation.y, m_translation.z);
        auto m = yaw * pitch * t;
        XMStoreFloat4x4(&m_sceneConstant.Data.view, m);
    }
}

void Camera::Initialize(const ComPtr<ID3D12Device> &device,
                        const ComPtr<ID3D12DescriptorHeap> &cbvHeap)
{
    m_sceneConstant.Initialize(device, cbvHeap, 0);
    m_sceneConstant.CopyToGpu();
}

void Camera::OnFrame(const ScreenState &state, const ScreenState &prev)
{
    bool changed = false;
    auto dx = state.X - prev.X;
    auto dy = state.Y - prev.Y;
    auto dt = state.DeltaSeconds(prev);
    if (state.Has(MouseButtonFlags::RightDown) && prev.Has(MouseButtonFlags::RightDown))
    {
        // right drag
        changed = true;
        auto f = 1.0f * dt;
        m_yaw -= dx * f;
        m_pitch -= dy * f;
    }
    if (state.Has(MouseButtonFlags::MiddleDown) && prev.Has(MouseButtonFlags::MiddleDown))
    {
        // Middle drag
        changed = true;
        const auto f = m_translation.z * (float)tan(m_fovY / 2) / state.Height * 2;

        m_translation.x += dx * f;
        m_translation.y -= dy * f;
    }
    if (state.Has(MouseButtonFlags::WheelMinus))
    {
        changed = true;
        m_translation.z *= 1.1f;
    }
    if (state.Has(MouseButtonFlags::WheelPlus))
    {
        changed = true;
        m_translation.z *= 0.9f;
    }
    if (!state.SameSize(prev))
    {
        changed = true;
        m_aspectRatio = state.AspectRatio();
    }

    if (changed)
    {
        Calc();
        m_sceneConstant.CopyToGpu();
    }
}

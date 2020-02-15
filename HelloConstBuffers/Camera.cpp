#include "Camera.h"
using namespace DirectX;

void Camera::Initialize(const ComPtr<ID3D12Device> &device,
                        const ComPtr<ID3D12DescriptorHeap> &cbvHeap)
{
    m_sceneConstant.Initialize(device, cbvHeap, 0);
}

void Camera::UpdateProjection(float aspectRatio)
{
    // projection
    {
        auto m = XMMatrixPerspectiveFovLH(m_fovY, aspectRatio, m_near, m_far);
        XMStoreFloat4x4(&m_sceneConstant.Data.projection, m);
    }

    // view
    {
        auto m = XMMatrixTranslation(0, 0, 1);
        XMStoreFloat4x4(&m_sceneConstant.Data.view, m);
    }
    m_sceneConstant.CopyToGpu();
}

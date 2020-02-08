#pragma once
#include <d3d12.h>
#include <wrl/client.h>

class Axis
{
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    unsigned int m_uiControllerVertcount = 0;
    int m_iTrackedControllerCount = 0;
    int m_iTrackedControllerCount_Last = -1;
    int m_iValidPoseCount = 0;
    int m_iValidPoseCount_Last = -1;
    ComPtr<ID3D12Resource> m_pControllerAxisVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_controllerAxisVertexBufferView = {};

public:
    //-----------------------------------------------------------------------------
    // Purpose: Update the vertex data for the controllers as X/Y/Z lines
    //-----------------------------------------------------------------------------
    void UpdateControllerAxes(HMD *hmd, const ComPtr<ID3D12Device> &device)
    {
        // Don't attempt to update controllers if input is not available
        if (!hmd->Hmd()->IsInputAvailable())
        {
            return;
        }

        std::vector<float> vertdataarray;
        m_uiControllerVertcount = 0;
        m_iTrackedControllerCount = 0;
        for (vr::TrackedDeviceIndex_t unTrackedDevice = vr::k_unTrackedDeviceIndex_Hmd + 1; unTrackedDevice < vr::k_unMaxTrackedDeviceCount; ++unTrackedDevice)
        {
            if (!hmd->Hmd()->IsTrackedDeviceConnected(unTrackedDevice))
                continue;

            if (hmd->Hmd()->GetTrackedDeviceClass(unTrackedDevice) != vr::TrackedDeviceClass_Controller)
                continue;

            m_iTrackedControllerCount += 1;

            if (!hmd->PoseIsValid(unTrackedDevice))
                continue;

            const Matrix4 &mat = hmd->DevicePose(unTrackedDevice);

            Vector4 center = mat * Vector4(0, 0, 0, 1);

            for (int i = 0; i < 3; ++i)
            {
                Vector3 color(0, 0, 0);
                Vector4 point(0, 0, 0, 1);
                point[i] += 0.05f; // offset in X, Y, Z
                color[i] = 1.0;    // R, G, B
                point = mat * point;
                vertdataarray.push_back(center.x);
                vertdataarray.push_back(center.y);
                vertdataarray.push_back(center.z);

                vertdataarray.push_back(color.x);
                vertdataarray.push_back(color.y);
                vertdataarray.push_back(color.z);

                vertdataarray.push_back(point.x);
                vertdataarray.push_back(point.y);
                vertdataarray.push_back(point.z);

                vertdataarray.push_back(color.x);
                vertdataarray.push_back(color.y);
                vertdataarray.push_back(color.z);

                m_uiControllerVertcount += 2;
            }

            Vector4 start = mat * Vector4(0, 0, -0.02f, 1);
            Vector4 end = mat * Vector4(0, 0, -39.f, 1);
            Vector3 color(.92f, .92f, .71f);

            vertdataarray.push_back(start.x);
            vertdataarray.push_back(start.y);
            vertdataarray.push_back(start.z);
            vertdataarray.push_back(color.x);
            vertdataarray.push_back(color.y);
            vertdataarray.push_back(color.z);

            vertdataarray.push_back(end.x);
            vertdataarray.push_back(end.y);
            vertdataarray.push_back(end.z);
            vertdataarray.push_back(color.x);
            vertdataarray.push_back(color.y);
            vertdataarray.push_back(color.z);
            m_uiControllerVertcount += 2;
        }

        // Setup the VB the first time through.
        if (m_pControllerAxisVertexBuffer == nullptr && vertdataarray.size() > 0)
        {
            // Make big enough to hold up to the max number
            size_t nSize = sizeof(float) * vertdataarray.size();
            nSize *= vr::k_unMaxTrackedDeviceCount;

            device->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(nSize),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_pControllerAxisVertexBuffer));

            m_controllerAxisVertexBufferView.BufferLocation = m_pControllerAxisVertexBuffer->GetGPUVirtualAddress();
            m_controllerAxisVertexBufferView.StrideInBytes = sizeof(float) * 6;
            m_controllerAxisVertexBufferView.SizeInBytes = (UINT)(sizeof(float) * vertdataarray.size());
        }

        // Update the VB data
        if (m_pControllerAxisVertexBuffer && vertdataarray.size() > 0)
        {
            UINT8 *pMappedBuffer;
            CD3DX12_RANGE readRange(0, 0);
            m_pControllerAxisVertexBuffer->Map(0, &readRange, reinterpret_cast<void **>(&pMappedBuffer));
            memcpy(pMappedBuffer, &vertdataarray[0], sizeof(float) * vertdataarray.size());
            m_pControllerAxisVertexBuffer->Unmap(0, nullptr);
        }

        //     // Spew out the controller and pose count whenever they change.
        //     if (m_iTrackedControllerCount != m_iTrackedControllerCount_Last || m_iValidPoseCount != m_iValidPoseCount_Last)
        //     {
        //         m_iValidPoseCount_Last = m_iValidPoseCount;
        //         m_iTrackedControllerCount_Last = m_iTrackedControllerCount;

        //         dprintf("PoseCount:%d(%s) Controllers:%d\n", m_iValidPoseCount, m_strPoseClasses.c_str(), m_iTrackedControllerCount);
        //     }
    }

    void Draw(const ComPtr<ID3D12GraphicsCommandList> &pCommandList)
    {
        if (m_pControllerAxisVertexBuffer)
        {
            pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
            pCommandList->IASetVertexBuffers(0, 1, &m_controllerAxisVertexBufferView);
            pCommandList->DrawInstanced(m_uiControllerVertcount, 1, 0, 0);
        }
    }
};

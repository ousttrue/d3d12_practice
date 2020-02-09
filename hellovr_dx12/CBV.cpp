#include "CBV.h"
#include "Matrices.h"

// Create descriptor heaps
bool CBV::Initialize(const ComPtr<ID3D12Device> &device)
{
    // heap
    m_nCBVSRVDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_DESCRIPTOR_HEAP_DESC cbvSrvHeapDesc = {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        .NumDescriptors = NUM_SRV_CBVS,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
    };
    device->CreateDescriptorHeap(&cbvSrvHeapDesc, IID_PPV_ARGS(&m_pCBVSRVHeap));

    // Create constant buffer
    device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(1024 * 64),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_pSceneConstantBuffer));

    // Map
    {
        // Keep as persistently mapped buffer, store left eye in first 256 bytes, right eye in second
        UINT8 *pBuffer;
        CD3DX12_RANGE readRange(0, 0);
        m_pSceneConstantBuffer->Map(0, &readRange, reinterpret_cast<void **>(&pBuffer));
        // Left eye to first 256 bytes, right eye to second 256 bytes
        m_pSceneConstantBufferData[0] = pBuffer;
        m_pSceneConstantBufferData[1] = pBuffer + 256;

        // Left eye CBV
        auto cbvLeftEyeHandle = CpuHandle(CBV_LEFT_EYE);
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {
            .BufferLocation = m_pSceneConstantBuffer->GetGPUVirtualAddress(),
            .SizeInBytes = (sizeof(Matrix4) + 255) & ~255, // Pad to 256 bytes
        };
        device->CreateConstantBufferView(&cbvDesc, cbvLeftEyeHandle);
        m_sceneConstantBufferView[0] = cbvLeftEyeHandle;

        // Right eye CBV
        auto cbvRightEyeHandle = CpuHandle(CBV_RIGHT_EYE);
        cbvDesc.BufferLocation += 256;
        device->CreateConstantBufferView(&cbvDesc, cbvRightEyeHandle);
        m_sceneConstantBufferView[1] = cbvRightEyeHandle;
    }

    return true;
}

void CBV::Set(const ComPtr<ID3D12GraphicsCommandList> &pCommandList, vr::Hmd_Eye nEye, const Matrix4 &pose)
{
    // Select the CBV (left or right eye)
    auto cbvHandle = GpuHandle((CBVSRVIndex_t)nEye);
    pCommandList->SetGraphicsRootDescriptorTable(0, cbvHandle);

    // SRV is just after the left eye
    auto srvHandle = GpuHandle(SRV_TEXTURE_MAP);
    pCommandList->SetGraphicsRootDescriptorTable(1, srvHandle);

    // Update the persistently mapped pointer to the CB data with the latest matrix
    memcpy(m_pSceneConstantBufferData[nEye], pose.get(), sizeof(Matrix4));
}

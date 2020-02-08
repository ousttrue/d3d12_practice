#include "Models.h"
#include "d3dx12.h"
#include "GenMipMapRGBA.h"
#include "CBV.h"
#include "dprintf.h"
#include "Matrices.h"
#include "Hmd.h"
#include <vector>

static void ThreadSleep(unsigned long nMilliseconds)
{
    ::Sleep(nMilliseconds);
}

class DX12RenderModel
{
    Microsoft::WRL::ComPtr<ID3D12Resource> m_pVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_pIndexBuffer;
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_pTexture;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_pTextureUploadHeap;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_pConstantBuffer;
    UINT8 *m_pConstantBufferData[2] = {nullptr, nullptr};
    size_t m_unVertexCount;
    vr::TrackedDeviceIndex_t m_unTrackedDeviceIndex;
    ID3D12DescriptorHeap *m_pCBVSRVHeap;
    std::string m_sModelName;

public:
    DX12RenderModel(const std::string &sRenderModelName)
        : m_sModelName(sRenderModelName)
    {
    }
    ~DX12RenderModel()
    {
    }
    const std::string &GetName() const { return m_sModelName; }

    bool BInit(ID3D12Device *pDevice,
               ID3D12GraphicsCommandList *pCommandList, ID3D12DescriptorHeap *pCBVSRVHeap,
               vr::TrackedDeviceIndex_t unTrackedDeviceIndex,
               const vr::RenderModel_t &vrModel,
               const vr::RenderModel_TextureMap_t &vrDiffuseTexture)
    {
        m_unTrackedDeviceIndex = unTrackedDeviceIndex;
        m_pCBVSRVHeap = pCBVSRVHeap;

        // Create and populate the vertex buffer
        {
            pDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                                             D3D12_HEAP_FLAG_NONE,
                                             &CD3DX12_RESOURCE_DESC::Buffer(sizeof(vr::RenderModel_Vertex_t) * vrModel.unVertexCount),
                                             D3D12_RESOURCE_STATE_GENERIC_READ,
                                             nullptr,
                                             IID_PPV_ARGS(&m_pVertexBuffer));

            UINT8 *pMappedBuffer;
            CD3DX12_RANGE readRange(0, 0);
            m_pVertexBuffer->Map(0, &readRange, reinterpret_cast<void **>(&pMappedBuffer));
            memcpy(pMappedBuffer, vrModel.rVertexData, sizeof(vr::RenderModel_Vertex_t) * vrModel.unVertexCount);
            m_pVertexBuffer->Unmap(0, nullptr);

            m_vertexBufferView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
            m_vertexBufferView.StrideInBytes = sizeof(vr::RenderModel_Vertex_t);
            m_vertexBufferView.SizeInBytes = sizeof(vr::RenderModel_Vertex_t) * vrModel.unVertexCount;
        }

        // Create and populate the index buffer
        {
            pDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                                             D3D12_HEAP_FLAG_NONE,
                                             &CD3DX12_RESOURCE_DESC::Buffer(sizeof(uint16_t) * vrModel.unTriangleCount * 3),
                                             D3D12_RESOURCE_STATE_GENERIC_READ,
                                             nullptr,
                                             IID_PPV_ARGS(&m_pIndexBuffer));

            UINT8 *pMappedBuffer;
            CD3DX12_RANGE readRange(0, 0);
            m_pIndexBuffer->Map(0, &readRange, reinterpret_cast<void **>(&pMappedBuffer));
            memcpy(pMappedBuffer, vrModel.rIndexData, sizeof(uint16_t) * vrModel.unTriangleCount * 3);
            m_pIndexBuffer->Unmap(0, nullptr);

            m_indexBufferView.BufferLocation = m_pIndexBuffer->GetGPUVirtualAddress();
            m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
            m_indexBufferView.SizeInBytes = sizeof(uint16_t) * vrModel.unTriangleCount * 3;
        }

        // create and populate the texture
        {
            int nImageWidth = vrDiffuseTexture.unWidth;
            int nImageHeight = vrDiffuseTexture.unHeight;
            std::vector<D3D12_SUBRESOURCE_DATA> mipLevelData;

            UINT8 *pBaseData = new UINT8[nImageWidth * nImageHeight * 4];
            memcpy(pBaseData, vrDiffuseTexture.rubTextureMapData, sizeof(UINT8) * nImageWidth * nImageHeight * 4);
            D3D12_SUBRESOURCE_DATA textureData = {};
            textureData.pData = &pBaseData[0];
            textureData.RowPitch = nImageWidth * 4;
            textureData.SlicePitch = textureData.RowPitch * nImageHeight;
            mipLevelData.push_back(textureData);

            // Generate mipmaps for the image
            int nPrevImageIndex = 0;
            int nMipWidth = nImageWidth;
            int nMipHeight = nImageHeight;

            while (nMipWidth > 1 && nMipHeight > 1)
            {
                UINT8 *pNewImage;
                ::GenMipMapRGBA((UINT8 *)mipLevelData[nPrevImageIndex].pData, &pNewImage, nMipWidth, nMipHeight, &nMipWidth, &nMipHeight);

                D3D12_SUBRESOURCE_DATA mipData = {};
                mipData.pData = pNewImage;
                mipData.RowPitch = nMipWidth * 4;
                mipData.SlicePitch = textureData.RowPitch * nMipHeight;
                mipLevelData.push_back(mipData);

                nPrevImageIndex++;
            }

            D3D12_RESOURCE_DESC textureDesc = {};
            textureDesc.MipLevels = (UINT16)mipLevelData.size();
            textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            textureDesc.Width = nImageWidth;
            textureDesc.Height = nImageHeight;
            textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
            textureDesc.DepthOrArraySize = 1;
            textureDesc.SampleDesc.Count = 1;
            textureDesc.SampleDesc.Quality = 0;
            textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

            pDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                                             D3D12_HEAP_FLAG_NONE,
                                             &textureDesc,
                                             D3D12_RESOURCE_STATE_COPY_DEST,
                                             nullptr,
                                             IID_PPV_ARGS(&m_pTexture));

            // Create shader resource view
            CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(pCBVSRVHeap->GetCPUDescriptorHandleForHeapStart());
            srvHandle.Offset(SRV_TEXTURE_RENDER_MODEL0 + unTrackedDeviceIndex, pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
            pDevice->CreateShaderResourceView(m_pTexture.Get(), nullptr, srvHandle);

            const UINT64 nUploadBufferSize = GetRequiredIntermediateSize(m_pTexture.Get(), 0, textureDesc.MipLevels);

            // Create the GPU upload buffer.
            pDevice->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(nUploadBufferSize),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_pTextureUploadHeap));

            UpdateSubresources(pCommandList, m_pTexture.Get(), m_pTextureUploadHeap.Get(), 0, 0, (UINT)mipLevelData.size(), &mipLevelData[0]);
            pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

            // Free mip pointers
            for (size_t nMip = 0; nMip < mipLevelData.size(); nMip++)
            {
                delete[] mipLevelData[nMip].pData;
            }
        }

        // Create a constant buffer to hold the transform (one for each eye)
        {
            pDevice->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(1024 * 64),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_pConstantBuffer));

            // Keep as persistently mapped buffer, store left eye in first 256 bytes, right eye in second
            UINT8 *pBuffer;
            CD3DX12_RANGE readRange(0, 0);
            m_pConstantBuffer->Map(0, &readRange, reinterpret_cast<void **>(&pBuffer));
            // Left eye to first 256 bytes, right eye to second 256 bytes
            m_pConstantBufferData[0] = pBuffer;
            m_pConstantBufferData[1] = pBuffer + 256;

            // Left eye CBV
            CD3DX12_CPU_DESCRIPTOR_HANDLE cbvLeftEyeHandle(m_pCBVSRVHeap->GetCPUDescriptorHandleForHeapStart());
            cbvLeftEyeHandle.Offset(CBV_LEFT_EYE_RENDER_MODEL0 + m_unTrackedDeviceIndex, pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
            cbvDesc.BufferLocation = m_pConstantBuffer->GetGPUVirtualAddress();
            cbvDesc.SizeInBytes = (sizeof(Matrix4) + 255) & ~255; // Pad to 256 bytes
            pDevice->CreateConstantBufferView(&cbvDesc, cbvLeftEyeHandle);

            // Right eye CBV
            CD3DX12_CPU_DESCRIPTOR_HANDLE cbvRightEyeHandle(m_pCBVSRVHeap->GetCPUDescriptorHandleForHeapStart());
            cbvRightEyeHandle.Offset(CBV_RIGHT_EYE_RENDER_MODEL0 + m_unTrackedDeviceIndex, pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
            cbvDesc.BufferLocation += 256;
            pDevice->CreateConstantBufferView(&cbvDesc, cbvRightEyeHandle);
        }

        m_unVertexCount = vrModel.unTriangleCount * 3;

        return true;
    }
    void Draw(vr::EVREye nEye, ID3D12GraphicsCommandList *pCommandList, UINT nCBVSRVDescriptorSize, const class Matrix4 &matMVP)
    {
        // Update the CB with the transform
        memcpy(m_pConstantBufferData[nEye], &matMVP, sizeof(matMVP));

        // Bind the CB
        int nStartOffset = (nEye == vr::Eye_Left) ? CBV_LEFT_EYE_RENDER_MODEL0 : CBV_RIGHT_EYE_RENDER_MODEL0;
        CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(m_pCBVSRVHeap->GetGPUDescriptorHandleForHeapStart());
        cbvHandle.Offset(nStartOffset + m_unTrackedDeviceIndex, nCBVSRVDescriptorSize);
        pCommandList->SetGraphicsRootDescriptorTable(0, cbvHandle);

        // Bind the texture
        CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(m_pCBVSRVHeap->GetGPUDescriptorHandleForHeapStart());
        srvHandle.Offset(SRV_TEXTURE_RENDER_MODEL0 + m_unTrackedDeviceIndex, nCBVSRVDescriptorSize);
        pCommandList->SetGraphicsRootDescriptorTable(1, srvHandle);

        // Bind the VB/IB and draw
        pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        pCommandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
        pCommandList->IASetIndexBuffer(&m_indexBufferView);
        pCommandList->DrawIndexedInstanced((UINT)m_unVertexCount, 1, 0, 0, 0);
    }
};

void Models::Draw(const ComPtr<ID3D12GraphicsCommandList> &pCommandList, vr::EVREye nEye, UINT unTrackedDevice, const Matrix4 &matMVP)
{
    auto model = m_rTrackedDeviceToRenderModel[unTrackedDevice];
    if (model)
    {
        model->Draw(nEye, pCommandList.Get(), m_nCBVSRVDescriptorSize, matMVP);
    }
}

//-----------------------------------------------------------------------------
// Purpose: Create/destroy D3D12 Render Models
//-----------------------------------------------------------------------------
void Models::SetupRenderModels(HMD *hmd,
                               const ComPtr<ID3D12Device> &device,
                               const ComPtr<ID3D12DescriptorHeap> &heap,
                               const ComPtr<ID3D12GraphicsCommandList> &pCommandList)
{
    m_nCBVSRVDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    memset(m_rTrackedDeviceToRenderModel, 0, sizeof(m_rTrackedDeviceToRenderModel));

    for (uint32_t unTrackedDevice = vr::k_unTrackedDeviceIndex_Hmd + 1; unTrackedDevice < vr::k_unMaxTrackedDeviceCount; unTrackedDevice++)
    {
        SetupRenderModelForTrackedDevice(hmd, device, heap, pCommandList, unTrackedDevice);
    }
}

//-----------------------------------------------------------------------------
// Purpose: Create/destroy D3D12 a Render Model for a single tracked device
//-----------------------------------------------------------------------------
void Models::SetupRenderModelForTrackedDevice(HMD *hmd,
                                              const ComPtr<ID3D12Device> &device,
                                              const ComPtr<ID3D12DescriptorHeap> &heap,
                                              const ComPtr<ID3D12GraphicsCommandList> &pCommandList,
                                              vr::TrackedDeviceIndex_t unTrackedDeviceIndex)
{
    if (unTrackedDeviceIndex >= vr::k_unMaxTrackedDeviceCount)
        return;
    if (!hmd->Hmd()->IsTrackedDeviceConnected(unTrackedDeviceIndex))
        return;

    // try to find a model we've already set up
    auto sRenderModelName = hmd->RenderModelName(unTrackedDeviceIndex);
    auto pRenderModel = FindOrLoadRenderModel(device, heap, pCommandList, unTrackedDeviceIndex, sRenderModelName.c_str());
    if (!pRenderModel)
    {
        std::string sTrackingSystemName = hmd->SystemName(unTrackedDeviceIndex);
        dprintf("Unable to load render model for tracked device %d (%s.%s)", unTrackedDeviceIndex, sTrackingSystemName.c_str(), sRenderModelName.c_str());
    }
    else
    {
        m_rTrackedDeviceToRenderModel[unTrackedDeviceIndex] = pRenderModel;
    }
}

//-----------------------------------------------------------------------------
// Purpose: Finds a render model we've already loaded or loads a new one
//-----------------------------------------------------------------------------
DX12RenderModel *Models::FindOrLoadRenderModel(
    const ComPtr<ID3D12Device> &device,
    const ComPtr<ID3D12DescriptorHeap> &heap,
    const ComPtr<ID3D12GraphicsCommandList> &pCommandList,
    vr::TrackedDeviceIndex_t unTrackedDeviceIndex, const char *pchRenderModelName)
{
    DX12RenderModel *pRenderModel = NULL;

    // load the model if we didn't find one
    if (!pRenderModel)
    {
        vr::RenderModel_t *pModel;
        vr::EVRRenderModelError error;
        while (1)
        {
            error = vr::VRRenderModels()->LoadRenderModel_Async(pchRenderModelName, &pModel);
            if (error != vr::VRRenderModelError_Loading)
                break;

            ThreadSleep(1);
        }

        if (error != vr::VRRenderModelError_None)
        {
            dprintf("Unable to load render model %s - %s\n", pchRenderModelName, vr::VRRenderModels()->GetRenderModelErrorNameFromEnum(error));
            return NULL; // move on to the next tracked device
        }

        vr::RenderModel_TextureMap_t *pTexture;
        while (1)
        {
            error = vr::VRRenderModels()->LoadTexture_Async(pModel->diffuseTextureId, &pTexture);
            if (error != vr::VRRenderModelError_Loading)
                break;

            ThreadSleep(1);
        }

        if (error != vr::VRRenderModelError_None)
        {
            dprintf("Unable to load render texture id:%d for render model %s\n", pModel->diffuseTextureId, pchRenderModelName);
            vr::VRRenderModels()->FreeRenderModel(pModel);
            return NULL; // move on to the next tracked device
        }

        pRenderModel = new DX12RenderModel(pchRenderModelName);
        if (!pRenderModel->BInit(device.Get(), pCommandList.Get(), heap.Get(), unTrackedDeviceIndex, *pModel, *pTexture))
        {
            dprintf("Unable to create D3D12 model from render model %s\n", pchRenderModelName);
            delete pRenderModel;
            pRenderModel = NULL;
        }
        vr::VRRenderModels()->FreeRenderModel(pModel);
        vr::VRRenderModels()->FreeTexture(pTexture);
    }

    return pRenderModel;
}

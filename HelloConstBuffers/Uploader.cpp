#include "Uploader.h"

void Uploader::Initialize(const ComPtr<ID3D12Device> &device)
{
}

void Uploader::Update()
{
    if (m_callback)
    {
        // check fence
    }
    else
    {
        // dequeue -> execute
    }
}

// CommandList *CD3D12Scene::SetVertices(const ComPtr<ID3D12Device> &device,
//                                       const void *vertices, UINT vertexBytes, UINT vertexStride,
//                                       const void *indices, UINT indexBytes, DXGI_FORMAT indexStride,
//                                       bool isDynamic)
// {
//     // Create the vertex buffer.
//     if (isDynamic)
//     {
//         m_vertexBuffer = ResourceItem::CreateUpload(device, vertexBytes);
//         if (!m_vertexBuffer)
//         {
//             throw;
//         }
//         m_vertexBuffer->MapCopyUnmap(vertices, vertexBytes);
//     }
//     else
//     {
//         m_vertexBuffer = ResourceItem::CreateDefault(device, vertexBytes);
//         if (!m_vertexBuffer)
//         {
//             throw;
//         }

//         // m_indexBuffer = ResourceItem::CreateDefault(device, indexBytes);
//         // if(!m_indexBuffer)
//         // {
//         //     throw;
//         // }

//         auto byteLength = (UINT)std::max(vertexBytes, indexBytes);
//         m_upload = ResourceItem::CreateUpload(device, byteLength);
//         if (!m_upload)
//         {
//             throw;
//         }

//         m_commandList->Reset(m_pipelineState);

//         {
//             // Copy data to the intermediate upload heap and then schedule a copy
//             // from the upload heap to the vertex buffer.
//             D3D12_SUBRESOURCE_DATA vertexData = {
//                 .pData = vertices,
//                 .RowPitch = vertexBytes,
//                 .SlicePitch = vertexStride,
//             };
//             UpdateSubresources<1>(m_commandList->Get(), m_vertexBuffer->Resource().Get(), m_upload->Resource().Get(), 0, 0, 1, &vertexData);

//             m_vertexBuffer->EnqueueTransition(m_commandList, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
//         }
//         // {
//         //     D3D12_SUBRESOURCE_DATA vertexData = {
//         //         .pData = indices,
//         //         .RowPitch = indexBytes,
//         //         .SlicePitch = 2,
//         //     };
//         //     UpdateSubresources<1>(m_commandList->Get(), m_indexBuffer.Get(), m_upload.Get(), 0, 0, 1, &vertexData);
//         //     m_commandList->Get()->ResourceBarrier(
//         //         1,
//         //         &CD3DX12_RESOURCE_BARRIER::Transition(
//         //             m_indexBuffer.Get(),
//         //             D3D12_RESOURCE_STATE_COPY_DEST,
//         //             D3D12_RESOURCE_STATE_INDEX_BUFFER));
//         // }
//     }

//     m_vertexBufferView = D3D12_VERTEX_BUFFER_VIEW{
//         .BufferLocation = m_vertexBuffer->Resource()->GetGPUVirtualAddress(),
//         .SizeInBytes = vertexBytes,
//         .StrideInBytes = vertexStride,
//     };

//     // m_indexBufferView = D3D12_INDEX_BUFFER_VIEW{
//     //     .BufferLocation = m_indexBuffer->GetGPUVirtualAddress(),
//     //     .SizeInBytes = indexBytes,
//     //     .Format = indexStride,
//     // };

//     if (isDynamic)
//     {
//         return nullptr;
//     }
//     else
//     {
//         return m_commandList;
//     }
// }

        // m_vertexBuffer->MapCopyUnmap(vertices, vertexBytes);
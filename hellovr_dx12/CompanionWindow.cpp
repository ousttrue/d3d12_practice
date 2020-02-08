#include "CompanionWindow.h"
#include "Matrices.h"
#include "d3dx12.h"
#include <vector>

struct VertexDataWindow
{
    Vector2 position;
    Vector2 texCoord;

    VertexDataWindow(const Vector2 &pos, const Vector2 tex) : position(pos), texCoord(tex) {}
};

void CompanionWindow::SetupCompanionWindow(const ComPtr<ID3D12Device> &device)
{
    std::vector<VertexDataWindow> vVerts;

    // left eye verts
    vVerts.push_back(VertexDataWindow(Vector2(-1, -1), Vector2(0, 1)));
    vVerts.push_back(VertexDataWindow(Vector2(0, -1), Vector2(1, 1)));
    vVerts.push_back(VertexDataWindow(Vector2(-1, 1), Vector2(0, 0)));
    vVerts.push_back(VertexDataWindow(Vector2(0, 1), Vector2(1, 0)));

    // right eye verts
    vVerts.push_back(VertexDataWindow(Vector2(0, -1), Vector2(0, 1)));
    vVerts.push_back(VertexDataWindow(Vector2(1, -1), Vector2(1, 1)));
    vVerts.push_back(VertexDataWindow(Vector2(0, 1), Vector2(0, 0)));
    vVerts.push_back(VertexDataWindow(Vector2(1, 1), Vector2(1, 0)));

    device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(sizeof(VertexDataWindow) * vVerts.size()),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_pCompanionWindowVertexBuffer));

    UINT8 *pMappedBuffer;
    CD3DX12_RANGE readRange(0, 0);
    m_pCompanionWindowVertexBuffer->Map(0, &readRange, reinterpret_cast<void **>(&pMappedBuffer));
    memcpy(pMappedBuffer, &vVerts[0], sizeof(VertexDataWindow) * vVerts.size());
    m_pCompanionWindowVertexBuffer->Unmap(0, nullptr);

    m_companionWindowVertexBufferView.BufferLocation = m_pCompanionWindowVertexBuffer->GetGPUVirtualAddress();
    m_companionWindowVertexBufferView.StrideInBytes = sizeof(VertexDataWindow);
    m_companionWindowVertexBufferView.SizeInBytes = (UINT)(sizeof(VertexDataWindow) * vVerts.size());

    UINT16 vIndices[] = {0, 1, 3, 0, 3, 2, 4, 5, 7, 4, 7, 6};
    m_uiCompanionWindowIndexSize = _countof(vIndices);

    device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                                    D3D12_HEAP_FLAG_NONE,
                                    &CD3DX12_RESOURCE_DESC::Buffer(sizeof(vIndices)),
                                    D3D12_RESOURCE_STATE_GENERIC_READ,
                                    nullptr,
                                    IID_PPV_ARGS(&m_pCompanionWindowIndexBuffer));

    m_pCompanionWindowIndexBuffer->Map(0, &readRange, reinterpret_cast<void **>(&pMappedBuffer));
    memcpy(pMappedBuffer, &vIndices[0], sizeof(vIndices));
    m_pCompanionWindowIndexBuffer->Unmap(0, nullptr);

    m_companionWindowIndexBufferView.BufferLocation = m_pCompanionWindowIndexBuffer->GetGPUVirtualAddress();
    m_companionWindowIndexBufferView.Format = DXGI_FORMAT_R16_UINT;
    m_companionWindowIndexBufferView.SizeInBytes = sizeof(vIndices);
}

void CompanionWindow::Draw(const ComPtr<ID3D12GraphicsCommandList> &pCommandList,
                           D3D12_GPU_DESCRIPTOR_HANDLE srvHandleLeftEye,
                           D3D12_GPU_DESCRIPTOR_HANDLE srvHandleRightEye)
{
    pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pCommandList->IASetVertexBuffers(0, 1, &m_companionWindowVertexBufferView);
    pCommandList->IASetIndexBuffer(&m_companionWindowIndexBufferView);

    pCommandList->SetGraphicsRootDescriptorTable(1, srvHandleLeftEye);
    pCommandList->DrawIndexedInstanced(m_uiCompanionWindowIndexSize / 2, 1, 0, 0, 0);

    pCommandList->SetGraphicsRootDescriptorTable(1, srvHandleRightEye);
    pCommandList->DrawIndexedInstanced(m_uiCompanionWindowIndexSize / 2, 1, (m_uiCompanionWindowIndexSize / 2), 0, 0);
}

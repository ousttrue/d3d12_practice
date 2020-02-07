#pragma once
#include <DirectXMath.h>

struct Vertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT4 color;
};
static Vertex g_triangleVertices[] =
    {
        {{0.0f, 0.25f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{0.25f, -0.25f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{-0.25f, -0.25f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}};

class Scene
{
    ComPtr<ID3D12Resource> m_vertexBuffer;

public:
    // Create the vertex buffer.
    void Initialize(const ComPtr<ID3D12Device> &device)
    {
        // Note: using upload heaps to transfer static data like vert buffers is not
        // recommended. Every time the GPU needs it, the upload heap will be marshalled
        // over. Please read up on Default Heap usage. An upload heap is used here for
        // code simplicity and because there are very few verts to actually transfer.
        ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(sizeof(g_triangleVertices)),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_vertexBuffer)));

        // Copy the triangle data to the vertex buffer.
        UINT8 *pVertexDataBegin;
        D3D12_RANGE readRange = {}; // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void **>(&pVertexDataBegin)));
        memcpy(pVertexDataBegin, g_triangleVertices, sizeof(g_triangleVertices));
        m_vertexBuffer->Unmap(0, nullptr);
    }

    void Draw(const ComPtr<ID3D12GraphicsCommandList> &commandList)
    {
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        D3D12_VERTEX_BUFFER_VIEW vertexBufferView{
            .BufferLocation = m_vertexBuffer->GetGPUVirtualAddress(),
            .SizeInBytes = sizeof(g_triangleVertices),
            .StrideInBytes = sizeof(Vertex),
        };
        commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

        commandList->DrawInstanced(3, 1, 0, 0);
    }
};
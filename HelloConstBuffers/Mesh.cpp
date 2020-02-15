#include "Mesh.h"
#include "ResourceItem.h"
#include "CommandList.h"

void Mesh::Command(CommandList *commandList)
{
    auto _commandList = commandList->Get();
    if (m_vertexBuffer)
    {
        auto state = m_vertexBuffer->ResourceState();
        if (state == D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER || state == D3D12_RESOURCE_STATE_GENERIC_READ)
        {
            _commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            // Initialize the vertex buffer view.
            _commandList->IASetVertexBuffers(0, 1, &m_vertexBuffer->VertexBufferView());
            _commandList->DrawInstanced(3, 1, 0, 0);
        }
        else if (state == D3D12_RESOURCE_STATE_COPY_DEST)
        {
            if (m_vertexBuffer->UploadState() == UploadStates::Uploaded)
            {
                m_vertexBuffer->EnqueueTransition(commandList, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            }
        }
    }
}
#include "Mesh.h"
#include "ResourceItem.h"
#include "CommandList.h"

void Mesh::IndexedCommand(CommandList *commandList)
{
    auto indexState = m_indexBuffer->State();
    if (indexState.State == D3D12_RESOURCE_STATE_COPY_DEST)
    {
        if (indexState.Upload == UploadStates::Uploaded)
        {
            m_indexBuffer->EnqueueTransition(commandList, D3D12_RESOURCE_STATE_INDEX_BUFFER);
        }
    }

    auto vertexState = m_vertexBuffer->State();
    if (vertexState.State == D3D12_RESOURCE_STATE_COPY_DEST)
    {
        if (vertexState.Upload == UploadStates::Uploaded)
        {
            m_vertexBuffer->EnqueueTransition(commandList, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        }
    }

    if(vertexState.Drawable() && indexState.Drawable())
    {
        auto _commandList = commandList->Get();
        _commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        _commandList->IASetVertexBuffers(0, 1, &m_vertexBuffer->VertexBufferView());
        _commandList->IASetIndexBuffer(&m_indexBuffer->IndexBufferView());
        _commandList->DrawIndexedInstanced(m_indexBuffer->Count(), 1, 0, 0, 0);
    }
}

void Mesh::NonIndexedCommand(CommandList *commandList)
{
    auto state = m_vertexBuffer->State();
    if (state.State == D3D12_RESOURCE_STATE_COPY_DEST)
    {
        if (state.Upload == UploadStates::Uploaded)
        {
            m_vertexBuffer->EnqueueTransition(commandList, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        }
    }

    if (state.Drawable())
    {
        auto _commandList = commandList->Get();
        _commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        _commandList->IASetVertexBuffers(0, 1, &m_vertexBuffer->VertexBufferView());
        _commandList->DrawInstanced(m_vertexBuffer->Count(), 1, 0, 0);
    }
}

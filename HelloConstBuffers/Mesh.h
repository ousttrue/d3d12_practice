#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>

class Mesh
{
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    std::shared_ptr<class ResourceItem> m_vertexBuffer;
    std::shared_ptr<class ResourceItem> m_indexBuffer;

public:
    void VertexBuffer(const std::shared_ptr<class ResourceItem> &item) { m_vertexBuffer = item; }
    void IndexBuffer(const std::shared_ptr<class ResourceItem> &item) { m_indexBuffer = item; }
    void Command(class CommandList *commandList)
    {
        if (m_indexBuffer && m_vertexBuffer)
        {
            Mesh::IndexedCommand(commandList);
        }
        else if(m_vertexBuffer)
        {
            Mesh::NonIndexedCommand(commandList);
        }
    }

private:
    void IndexedCommand(class CommandList *commandList);
    void NonIndexedCommand(class CommandList *commandList);
};

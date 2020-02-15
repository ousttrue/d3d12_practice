#include "Uploader.h"
#include "CD3D12CommandQueue.h"
#include "CommandList.h"
#include "ResourceItem.h"
#include "d3dx12.h"
#include "d3dhelper.h"

Uploader::Uploader()
    : m_queue(new CD3D12CommandQueue), m_commandList(new CommandList)
{
}

Uploader::~Uploader()
{
    m_queue->SyncFence();
    delete m_commandList;
    delete m_queue;
}

void Uploader::Initialize(const ComPtr<ID3D12Device> &device)
{
    m_queue->Initialize(device, D3D12_COMMAND_LIST_TYPE_COPY);
    m_commandList->Initialize(device, nullptr, D3D12_COMMAND_LIST_TYPE_COPY);
}

void Uploader::Update(const ComPtr<ID3D12Device> &device)
{
    if (!m_callback)
    {
        if (m_commands.empty())
        {
            return;
        }

        // dequeue -> execute
        auto command = m_commands.front();
        m_commands.pop();

        if (m_upload)
        {
            auto desc = m_upload->GetDesc();
            if (desc.Width * desc.Height < command.ByteLength)
            {
                // clear
                m_upload.Reset();
            }
        }
        if (!m_upload)
        {
            ThrowIfFailed(device->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(command.ByteLength),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_upload)));
        }

        m_commandList->Reset(nullptr);
        command.Item->EnqueueUpload(m_commandList, m_upload, command.Data, command.ByteLength, command.Stride);
        auto callbacks = m_commandList->Close();
        m_queue->Execute(m_commandList->Get());
        m_callbackFenceValue = m_queue->Signal();
        m_callback = [callbacks]() {
            for (auto &callback : callbacks)
            {
                callback();
            }
        };
    }
    else
    {
        // wait fence
        if (m_callbackFenceValue <= m_queue->FenceValue())
        {
            // callback is done
            m_callback();
            // clear
            m_callback = OnCompletedFunc();
        }
    }
}

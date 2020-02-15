#include "CommandList.h"
#include "d3dhelper.h"

void CommandList::Initialize(const ComPtr<ID3D12Device> &device, const ComPtr<ID3D12PipelineState> &ps, D3D12_COMMAND_LIST_TYPE type)
{
    ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&m_commandAllocator)));

    // Create the command list.
    ThrowIfFailed(device->CreateCommandList(0, type, m_commandAllocator.Get(), ps.Get(), IID_PPV_ARGS(&m_commandList)));

    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    ThrowIfFailed(m_commandList->Close());
}

void CommandList::Reset(const ComPtr<ID3D12PipelineState> &ps)
{
    ThrowIfFailed(m_commandAllocator->Reset());

    // However, when ExecuteCommandList() is called on a particular command
    // list, that command list can then be reset at any time and must be before
    // re-recording.
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), ps.Get()));
}

std::list<CommandList::OnCompletedFunc> CommandList::Close()
{
    m_commandList->Close();

    auto callbacks = m_callbacks;
    m_callbacks.clear();

    return callbacks;
}

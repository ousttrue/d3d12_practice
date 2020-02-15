#include "CD3D12CommandQueue.h"
#include "d3dhelper.h"

CD3D12CommandQueue::~CD3D12CommandQueue()
{
    // Ensure that the GPU is no longer referencing resources that are about to be
    // cleaned up by the destructor.
    SyncFence();

    CloseHandle(m_fenceEvent);
}

// Create synchronization objects and wait until assets have been uploaded to the GPU.
void CD3D12CommandQueue::Initialize(const ComPtr<ID3D12Device> &device)
{
    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {
        .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
        // .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
    };
    ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

    ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    m_fenceValue = 1;

    // Create an event handle to use for frame synchronization.
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (m_fenceEvent == nullptr)
    {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }

    // Wait for the command list to execute; we are reusing the same command
    // list in our main loop but for now, we just want to wait for setup to
    // complete before continuing.
    // SyncFence();
}

void CD3D12CommandQueue::Execute(ID3D12CommandList *commandList)
{
    ID3D12CommandList *list[] = {
        commandList};
    m_commandQueue->ExecuteCommandLists(_countof(list), list);
}

void CD3D12CommandQueue::SyncFence(const CallbackList &callbacks)
{
    // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
    // This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
    // sample illustrates how to use fences for efficient resource usage and to
    // maximize GPU utilization.

    // Signal and increment the fence value.
    const UINT64 fence = m_fenceValue;
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
    m_fenceValue++;

    // Wait until the previous frame is finished.
    if (m_fence->GetCompletedValue() < fence)
    {
        ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    for (auto &callback : callbacks)
    {
        callback();
    }
}

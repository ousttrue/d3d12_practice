#include "Fence.h"
#include "util.h"

Fence::Fence()
{
}
Fence::~Fence()
{
    CloseHandle(m_fenceEvent);
}

void Fence::Initialize(const ComPtr<ID3D12Device> &device)
{
    ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    // Create an event handle to use for frame synchronization.
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (m_fenceEvent == nullptr)
    {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }
}

void Fence::Wait(const ComPtr<ID3D12CommandQueue> queue)
{
    // Signal and increment the fence value.
    const UINT64 fence = m_fenceValue++;
    ThrowIfFailed(queue->Signal(m_fence.Get(), fence));

    // Wait until the previous frame is finished.
    if (m_fence->GetCompletedValue() < fence)
    {
        ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}

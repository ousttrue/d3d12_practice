#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <list>
#include <functional>

class CommandList
{
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;

public:
    using OnCompletedFunc = std::function<void()>;

private:
    std::list<OnCompletedFunc> m_callbacks;

public:
    ID3D12GraphicsCommandList *Get() const { return m_commandList.Get(); }
    void Initialize(const ComPtr<ID3D12Device> &device, const ComPtr<ID3D12PipelineState> &ps);
    void Reset(const ComPtr<ID3D12PipelineState> &ps);
    std::list<OnCompletedFunc> Close();
    void AddOnCompleted(const OnCompletedFunc &callback)
    {
        m_callbacks.push_back(callback);
    }
};

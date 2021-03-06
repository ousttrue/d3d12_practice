#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <DirectXMath.h>

class Scene
{
    template<class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    ComPtr<ID3D12Resource> m_vertexBuffer;

public:
    void Initialize(const ComPtr<ID3D12Device> &device);
    void Draw(const ComPtr<ID3D12GraphicsCommandList> &commandList);
};

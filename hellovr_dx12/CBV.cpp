#include "CBV.h"

// Create descriptor heaps
bool CBV::Initialize(const ComPtr<ID3D12Device> &device)
{
    m_nCBVSRVDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_DESCRIPTOR_HEAP_DESC cbvSrvHeapDesc = {};
    cbvSrvHeapDesc.NumDescriptors = NUM_SRV_CBVS;
    cbvSrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvSrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    device->CreateDescriptorHeap(&cbvSrvHeapDesc, IID_PPV_ARGS(&m_pCBVSRVHeap));

    return true;
}

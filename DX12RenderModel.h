#pragma once
#include <string>
#include <d3d12.h>
#include <wrl/client.h>
#include <openvr.h>

class DX12RenderModel
{
public:
	DX12RenderModel(const std::string &sRenderModelName);
	~DX12RenderModel();

	bool BInit(ID3D12Device *pDevice, ID3D12GraphicsCommandList *pCommandList, ID3D12DescriptorHeap *pCBVSRVHeap, vr::TrackedDeviceIndex_t unTrackedDeviceIndex, const vr::RenderModel_t &vrModel, const vr::RenderModel_TextureMap_t &vrDiffuseTexture);
	void Cleanup();
	void Draw(vr::EVREye nEye, ID3D12GraphicsCommandList *pCommandList, UINT nCBVSRVDescriptorSize, const class Matrix4 &matMVP);
	const std::string &GetName() const { return m_sModelName; }

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pIndexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pTexture;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pTextureUploadHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pConstantBuffer;
	UINT8 *m_pConstantBufferData[2];
	size_t m_unVertexCount;
	vr::TrackedDeviceIndex_t m_unTrackedDeviceIndex;
	ID3D12DescriptorHeap *m_pCBVSRVHeap;
	std::string m_sModelName;
};

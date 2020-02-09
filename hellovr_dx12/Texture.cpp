#include "Texture.h"
#include "pathtools.h"
#include "lodepng.h"
#include "d3dx12.h"
#include "GenMipMapRGBA.h"

bool Texture::SetupTexturemaps(const ComPtr<ID3D12Device> &device, const ComPtr<ID3D12GraphicsCommandList> &pCommandList,
                               D3D12_CPU_DESCRIPTOR_HANDLE srvHandle)
{
    std::string sExecutableDirectory = Path_StripFilename(Path_GetExecutablePath());
    std::string strFullPath = Path_MakeAbsolute("../../hellovr_dx12/cube_texture.png", sExecutableDirectory);

    std::vector<unsigned char> imageRGBA;
    unsigned nImageWidth, nImageHeight;
    unsigned nError = lodepng::decode(imageRGBA, nImageWidth, nImageHeight, strFullPath.c_str());

    if (nError != 0)
        return false;

    // Store level 0
    std::vector<D3D12_SUBRESOURCE_DATA> mipLevelData;
    UINT8 *pBaseData = new UINT8[nImageWidth * nImageHeight * 4];
    memcpy(pBaseData, &imageRGBA[0], sizeof(UINT8) * nImageWidth * nImageHeight * 4);

    D3D12_SUBRESOURCE_DATA textureData = {};
    textureData.pData = &pBaseData[0];
    textureData.RowPitch = nImageWidth * 4;
    textureData.SlicePitch = textureData.RowPitch * nImageHeight;
    mipLevelData.push_back(textureData);

    // Generate mipmaps for the image
    int nPrevImageIndex = 0;
    int nMipWidth = nImageWidth;
    int nMipHeight = nImageHeight;

    while (nMipWidth > 1 && nMipHeight > 1)
    {
        UINT8 *pNewImage;
        GenMipMapRGBA((UINT8 *)mipLevelData[nPrevImageIndex].pData, &pNewImage, nMipWidth, nMipHeight, &nMipWidth, &nMipHeight);

        D3D12_SUBRESOURCE_DATA mipData = {};
        mipData.pData = pNewImage;
        mipData.RowPitch = nMipWidth * 4;
        mipData.SlicePitch = textureData.RowPitch * nMipHeight;
        mipLevelData.push_back(mipData);

        nPrevImageIndex++;
    }

    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.MipLevels = (UINT16)mipLevelData.size();
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.Width = nImageWidth;
    textureDesc.Height = nImageHeight;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                                    D3D12_HEAP_FLAG_NONE,
                                    &textureDesc,
                                    D3D12_RESOURCE_STATE_COPY_DEST,
                                    nullptr,
                                    IID_PPV_ARGS(&m_pTexture));

    // D3D12_CPU_DESCRIPTOR_HANDLE m_textureShaderResourceView;
    // Create shader resource view
    // auto srvHandle = m_cbv->CpuHandle(SRV_TEXTURE_MAP);
    device->CreateShaderResourceView(m_pTexture.Get(), nullptr, srvHandle);
    // m_textureShaderResourceView = srvHandle;

    const UINT64 nUploadBufferSize = GetRequiredIntermediateSize(m_pTexture.Get(), 0, textureDesc.MipLevels);

    // Create the GPU upload buffer.
    device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(nUploadBufferSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_pTextureUploadHeap));

    UpdateSubresources(pCommandList.Get(), m_pTexture.Get(), m_pTextureUploadHeap.Get(), 0, 0, (UINT)mipLevelData.size(), &mipLevelData[0]);
    pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

    // Free mip pointers
    for (size_t nMip = 0; nMip < mipLevelData.size(); nMip++)
    {
        delete[] mipLevelData[nMip].pData;
    }
    return true;
}
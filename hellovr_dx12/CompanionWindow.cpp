#include "CompanionWindow.h"
#include "Matrices.h"
#include "d3dx12.h"
#include <vector>

struct VertexDataWindow
{
    Vector2 position;
    Vector2 texCoord;

    VertexDataWindow(const Vector2 &pos, const Vector2 tex) : position(pos), texCoord(tex) {}
};

void CompanionWindow::SetupCompanionWindow(const ComPtr<ID3D12Device> &device)
{
    std::vector<VertexDataWindow> vVerts;

    // left eye verts
    vVerts.push_back(VertexDataWindow(Vector2(-1, -1), Vector2(0, 1)));
    vVerts.push_back(VertexDataWindow(Vector2(0, -1), Vector2(1, 1)));
    vVerts.push_back(VertexDataWindow(Vector2(-1, 1), Vector2(0, 0)));
    vVerts.push_back(VertexDataWindow(Vector2(0, 1), Vector2(1, 0)));

    // right eye verts
    vVerts.push_back(VertexDataWindow(Vector2(0, -1), Vector2(0, 1)));
    vVerts.push_back(VertexDataWindow(Vector2(1, -1), Vector2(1, 1)));
    vVerts.push_back(VertexDataWindow(Vector2(0, 1), Vector2(0, 0)));
    vVerts.push_back(VertexDataWindow(Vector2(1, 1), Vector2(1, 0)));

    device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(sizeof(VertexDataWindow) * vVerts.size()),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_pCompanionWindowVertexBuffer));

    UINT8 *pMappedBuffer;
    CD3DX12_RANGE readRange(0, 0);
    m_pCompanionWindowVertexBuffer->Map(0, &readRange, reinterpret_cast<void **>(&pMappedBuffer));
    memcpy(pMappedBuffer, &vVerts[0], sizeof(VertexDataWindow) * vVerts.size());
    m_pCompanionWindowVertexBuffer->Unmap(0, nullptr);

    m_companionWindowVertexBufferView.BufferLocation = m_pCompanionWindowVertexBuffer->GetGPUVirtualAddress();
    m_companionWindowVertexBufferView.StrideInBytes = sizeof(VertexDataWindow);
    m_companionWindowVertexBufferView.SizeInBytes = (UINT)(sizeof(VertexDataWindow) * vVerts.size());

    UINT16 vIndices[] = {0, 1, 3, 0, 3, 2, 4, 5, 7, 4, 7, 6};
    m_uiCompanionWindowIndexSize = _countof(vIndices);

    device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                                    D3D12_HEAP_FLAG_NONE,
                                    &CD3DX12_RESOURCE_DESC::Buffer(sizeof(vIndices)),
                                    D3D12_RESOURCE_STATE_GENERIC_READ,
                                    nullptr,
                                    IID_PPV_ARGS(&m_pCompanionWindowIndexBuffer));

    m_pCompanionWindowIndexBuffer->Map(0, &readRange, reinterpret_cast<void **>(&pMappedBuffer));
    memcpy(pMappedBuffer, &vIndices[0], sizeof(vIndices));
    m_pCompanionWindowIndexBuffer->Unmap(0, nullptr);

    m_companionWindowIndexBufferView.BufferLocation = m_pCompanionWindowIndexBuffer->GetGPUVirtualAddress();
    m_companionWindowIndexBufferView.Format = DXGI_FORMAT_R16_UINT;
    m_companionWindowIndexBufferView.SizeInBytes = sizeof(vIndices);
}

void CompanionWindow::Draw(const ComPtr<ID3D12GraphicsCommandList> &pCommandList,
                           D3D12_GPU_DESCRIPTOR_HANDLE srvHandleLeftEye,
                           D3D12_GPU_DESCRIPTOR_HANDLE srvHandleRightEye)
{
    pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pCommandList->IASetVertexBuffers(0, 1, &m_companionWindowVertexBufferView);
    pCommandList->IASetIndexBuffer(&m_companionWindowIndexBufferView);

    pCommandList->SetGraphicsRootDescriptorTable(1, srvHandleLeftEye);
    pCommandList->DrawIndexedInstanced(m_uiCompanionWindowIndexSize / 2, 1, 0, 0, 0);

    pCommandList->SetGraphicsRootDescriptorTable(1, srvHandleRightEye);
    pCommandList->DrawIndexedInstanced(m_uiCompanionWindowIndexSize / 2, 1, (m_uiCompanionWindowIndexSize / 2), 0, 0);
}

//-----------------------------------------------------------------------------
// Purpose: Creates a frame buffer. Returns true if the buffer was set up.
//          Returns false if the setup failed.
//-----------------------------------------------------------------------------
bool CompanionWindow::CreateFrameBuffer(const ComPtr<ID3D12Device> &device,
                                        int nWidth, int nHeight,
                                        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
                                        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle,
                                        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle,
                                        FramebufferDesc &framebufferDesc)
{
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.MipLevels = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    textureDesc.Width = nWidth;
    textureDesc.Height = nHeight;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.SampleDesc.Count = m_nMSAASampleCount;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    const float clearColor[] = {0.0f, 0.0f, 0.0f, 1.0f};

    // Create color target
    device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                                    D3D12_HEAP_FLAG_NONE,
                                    &textureDesc,
                                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                                    &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, clearColor),
                                    IID_PPV_ARGS(&framebufferDesc.m_pTexture));

    device->CreateRenderTargetView(framebufferDesc.m_pTexture.Get(), nullptr, rtvHandle);
    framebufferDesc.m_renderTargetViewHandle = rtvHandle;

    // Create shader resource view
    device->CreateShaderResourceView(framebufferDesc.m_pTexture.Get(), nullptr, srvHandle);

    // Create depth
    D3D12_RESOURCE_DESC depthDesc = textureDesc;
    depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                                    D3D12_HEAP_FLAG_NONE,
                                    &depthDesc,
                                    D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                    &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.0f, 0),
                                    IID_PPV_ARGS(&framebufferDesc.m_pDepthStencil));

    device->CreateDepthStencilView(framebufferDesc.m_pDepthStencil.Get(), nullptr, dsvHandle);
    framebufferDesc.m_depthStencilViewHandle = dsvHandle;
    return true;
}

void CompanionWindow::BeginLeft(const ComPtr<ID3D12GraphicsCommandList> &pCommandList)
{
    // Transition to RENDER_TARGET
    pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_leftEyeDesc.m_pTexture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
    pCommandList->OMSetRenderTargets(1, &m_leftEyeDesc.m_renderTargetViewHandle, FALSE, &m_leftEyeDesc.m_depthStencilViewHandle);

    const float clearColor[] = {0.0f, 0.0f, 0.0f, 1.0f};
    pCommandList->ClearRenderTargetView(m_leftEyeDesc.m_renderTargetViewHandle, clearColor, 0, nullptr);
    pCommandList->ClearDepthStencilView(m_leftEyeDesc.m_depthStencilViewHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0, 0, 0, nullptr);
}

void CompanionWindow::EndLeft(const ComPtr<ID3D12GraphicsCommandList> &pCommandList)
{
    // Transition to SHADER_RESOURCE to submit to SteamVR
    pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_leftEyeDesc.m_pTexture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
}

void CompanionWindow::BeginRight(const ComPtr<ID3D12GraphicsCommandList> &pCommandList)
{
    // Transition to RENDER_TARGET
    pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_rightEyeDesc.m_pTexture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
    pCommandList->OMSetRenderTargets(1, &m_rightEyeDesc.m_renderTargetViewHandle, FALSE, &m_rightEyeDesc.m_depthStencilViewHandle);

    const float clearColor[] = {0.0f, 0.0f, 0.0f, 1.0f};
    pCommandList->ClearRenderTargetView(m_rightEyeDesc.m_renderTargetViewHandle, clearColor, 0, nullptr);
    pCommandList->ClearDepthStencilView(m_rightEyeDesc.m_depthStencilViewHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0, 0, 0, nullptr);
}

void CompanionWindow::EndRight(const ComPtr<ID3D12GraphicsCommandList> &pCommandList)
{
    // Transition to SHADER_RESOURCE to submit to SteamVR
    pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_rightEyeDesc.m_pTexture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
}

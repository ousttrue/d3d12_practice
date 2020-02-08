#include "Cubes.h"
#include "d3dx12.h"

Cubes::Cubes(int iSceneVolumeInit)
{
    // cube array
    m_iSceneVolumeWidth = iSceneVolumeInit;
    m_iSceneVolumeHeight = iSceneVolumeInit;
    m_iSceneVolumeDepth = iSceneVolumeInit;
}

//-----------------------------------------------------------------------------
// Purpose: create a sea of cubes
//-----------------------------------------------------------------------------
void Cubes::SetupScene(const ComPtr<ID3D12Device> &device)
{
    std::vector<float> vertdataarray;

    Matrix4 matScale;
    matScale.scale(m_fScale, m_fScale, m_fScale);
    Matrix4 matTransform;
    matTransform.translate(
        -((float)m_iSceneVolumeWidth * m_fScaleSpacing) / 2.f,
        -((float)m_iSceneVolumeHeight * m_fScaleSpacing) / 2.f,
        -((float)m_iSceneVolumeDepth * m_fScaleSpacing) / 2.f);

    Matrix4 mat = matScale * matTransform;

    for (int z = 0; z < m_iSceneVolumeDepth; z++)
    {
        for (int y = 0; y < m_iSceneVolumeHeight; y++)
        {
            for (int x = 0; x < m_iSceneVolumeWidth; x++)
            {
                AddCubeToScene(mat, vertdataarray);
                mat = mat * Matrix4().translate(m_fScaleSpacing, 0, 0);
            }
            mat = mat * Matrix4().translate(-((float)m_iSceneVolumeWidth) * m_fScaleSpacing, m_fScaleSpacing, 0);
        }
        mat = mat * Matrix4().translate(0, -((float)m_iSceneVolumeHeight) * m_fScaleSpacing, m_fScaleSpacing);
    }
    m_uiVertcount = (UINT)vertdataarray.size() / 5;

    device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                                    D3D12_HEAP_FLAG_NONE,
                                    &CD3DX12_RESOURCE_DESC::Buffer(sizeof(float) * vertdataarray.size()),
                                    D3D12_RESOURCE_STATE_GENERIC_READ,
                                    nullptr,
                                    IID_PPV_ARGS(&m_pSceneVertexBuffer));

    UINT8 *pMappedBuffer;
    CD3DX12_RANGE readRange(0, 0);
    m_pSceneVertexBuffer->Map(0, &readRange, reinterpret_cast<void **>(&pMappedBuffer));
    memcpy(pMappedBuffer, &vertdataarray[0], sizeof(float) * vertdataarray.size());
    m_pSceneVertexBuffer->Unmap(0, nullptr);

    m_sceneVertexBufferView.BufferLocation = m_pSceneVertexBuffer->GetGPUVirtualAddress();
    m_sceneVertexBufferView.StrideInBytes = sizeof(VertexDataScene);
    m_sceneVertexBufferView.SizeInBytes = (UINT)(sizeof(float) * vertdataarray.size());
}

void Cubes::Draw(const ComPtr<ID3D12GraphicsCommandList> &pCommandList)
{
    pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pCommandList->IASetVertexBuffers(0, 1, &m_sceneVertexBufferView);
    pCommandList->DrawInstanced(m_uiVertcount, 1, 0, 0);
}

void Cubes::AddCubeToScene(Matrix4 mat, std::vector<float> &vertdata)
{
    // Matrix4 mat( outermat.data() );

    Vector4 A = mat * Vector4(0, 0, 0, 1);
    Vector4 B = mat * Vector4(1, 0, 0, 1);
    Vector4 C = mat * Vector4(1, 1, 0, 1);
    Vector4 D = mat * Vector4(0, 1, 0, 1);
    Vector4 E = mat * Vector4(0, 0, 1, 1);
    Vector4 F = mat * Vector4(1, 0, 1, 1);
    Vector4 G = mat * Vector4(1, 1, 1, 1);
    Vector4 H = mat * Vector4(0, 1, 1, 1);

    // triangles instead of quads
    AddCubeVertex(E.x, E.y, E.z, 0, 1, vertdata); //Front
    AddCubeVertex(F.x, F.y, F.z, 1, 1, vertdata);
    AddCubeVertex(G.x, G.y, G.z, 1, 0, vertdata);
    AddCubeVertex(G.x, G.y, G.z, 1, 0, vertdata);
    AddCubeVertex(H.x, H.y, H.z, 0, 0, vertdata);
    AddCubeVertex(E.x, E.y, E.z, 0, 1, vertdata);

    AddCubeVertex(B.x, B.y, B.z, 0, 1, vertdata); //Back
    AddCubeVertex(A.x, A.y, A.z, 1, 1, vertdata);
    AddCubeVertex(D.x, D.y, D.z, 1, 0, vertdata);
    AddCubeVertex(D.x, D.y, D.z, 1, 0, vertdata);
    AddCubeVertex(C.x, C.y, C.z, 0, 0, vertdata);
    AddCubeVertex(B.x, B.y, B.z, 0, 1, vertdata);

    AddCubeVertex(H.x, H.y, H.z, 0, 1, vertdata); //Top
    AddCubeVertex(G.x, G.y, G.z, 1, 1, vertdata);
    AddCubeVertex(C.x, C.y, C.z, 1, 0, vertdata);
    AddCubeVertex(C.x, C.y, C.z, 1, 0, vertdata);
    AddCubeVertex(D.x, D.y, D.z, 0, 0, vertdata);
    AddCubeVertex(H.x, H.y, H.z, 0, 1, vertdata);

    AddCubeVertex(A.x, A.y, A.z, 0, 1, vertdata); //Bottom
    AddCubeVertex(B.x, B.y, B.z, 1, 1, vertdata);
    AddCubeVertex(F.x, F.y, F.z, 1, 0, vertdata);
    AddCubeVertex(F.x, F.y, F.z, 1, 0, vertdata);
    AddCubeVertex(E.x, E.y, E.z, 0, 0, vertdata);
    AddCubeVertex(A.x, A.y, A.z, 0, 1, vertdata);

    AddCubeVertex(A.x, A.y, A.z, 0, 1, vertdata); //Left
    AddCubeVertex(E.x, E.y, E.z, 1, 1, vertdata);
    AddCubeVertex(H.x, H.y, H.z, 1, 0, vertdata);
    AddCubeVertex(H.x, H.y, H.z, 1, 0, vertdata);
    AddCubeVertex(D.x, D.y, D.z, 0, 0, vertdata);
    AddCubeVertex(A.x, A.y, A.z, 0, 1, vertdata);

    AddCubeVertex(F.x, F.y, F.z, 0, 1, vertdata); //Right
    AddCubeVertex(B.x, B.y, B.z, 1, 1, vertdata);
    AddCubeVertex(C.x, C.y, C.z, 1, 0, vertdata);
    AddCubeVertex(C.x, C.y, C.z, 1, 0, vertdata);
    AddCubeVertex(G.x, G.y, G.z, 0, 0, vertdata);
    AddCubeVertex(F.x, F.y, F.z, 0, 1, vertdata);
}

void Cubes::AddCubeVertex(float fl0, float fl1, float fl2, float fl3, float fl4, std::vector<float> &vertdata)
{
    vertdata.push_back(fl0);
    vertdata.push_back(fl1);
    vertdata.push_back(fl2);
    vertdata.push_back(fl3);
    vertdata.push_back(fl4);
}

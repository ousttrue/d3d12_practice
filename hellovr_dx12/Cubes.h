#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include "Matrices.h"

class Cubes
{
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    int m_iSceneVolumeWidth;
    int m_iSceneVolumeHeight;
    int m_iSceneVolumeDepth;
    float m_fScaleSpacing = 4.0f;
    float m_fScale = 0.3f;
    unsigned int m_uiVertcount = 0;
    ComPtr<ID3D12Resource> m_pSceneVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_sceneVertexBufferView;
    struct VertexDataScene
    {
        Vector3 position;
        Vector2 texCoord;
    };

public:
    Cubes(int iSceneVolumeInit);
    //-----------------------------------------------------------------------------
    // Purpose: create a sea of cubes
    //-----------------------------------------------------------------------------
    void SetupScene(const ComPtr<ID3D12Device> &device);
    void Draw(const ComPtr<ID3D12GraphicsCommandList> &pCommandList);

private:
    void AddCubeToScene(Matrix4 mat, std::vector<float> &vertdata);
    void AddCubeVertex(float fl0, float fl1, float fl2, float fl3, float fl4, std::vector<float> &vertdata);
};

// dear imgui: standalone example application for DirectX 12
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
// FIXME: 64-bit only for now! (Because sizeof(ImTextureId) == sizeof(void*))

#include "D3DRenderer.h"
#include "Window.h"
#include <imgui.h>
#include "WindowImGui.h"
#include <tchar.h>
#include <vector>
#include <d3dcompiler.h>

#ifdef DX12_ENABLE_DEBUG_LAYER
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

static int const NUM_BACK_BUFFERS = 3;

static void EnableDebugLayer()
{
    ComPtr<ID3D12Debug> pdx12Debug;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pdx12Debug))))
    {
        pdx12Debug->EnableDebugLayer();
    }
}

static void ReportLiveObjects()
{
    ComPtr<IDXGIDebug1> pDebug;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
    {
        pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
    }
}

struct VERTEX_CONSTANT_BUFFER
{
    float mvp[4][4];
};

class Dx12ImGui
{
    // DirectX data
    ComPtr<ID3D12Device> g_pd3dDevice;
    ComPtr<ID3D10Blob> g_pVertexShaderBlob;
    ComPtr<ID3D10Blob> g_pPixelShaderBlob;
    ComPtr<ID3D12RootSignature> g_pRootSignature;
    ComPtr<ID3D12PipelineState> g_pPipelineState;
    DXGI_FORMAT g_RTVFormat = DXGI_FORMAT_UNKNOWN;
    ComPtr<ID3D12Resource> g_pFontTextureResource = NULL;
    D3D12_CPU_DESCRIPTOR_HANDLE g_hFontSrvCpuDescHandle = {};
    D3D12_GPU_DESCRIPTOR_HANDLE g_hFontSrvGpuDescHandle = {};

    struct FrameResources
    {
        ComPtr<ID3D12Resource> IndexBuffer;
        ComPtr<ID3D12Resource> VertexBuffer;
        int IndexBufferSize = 10000;
        int VertexBufferSize = 5000;
    };
    std::vector<FrameResources> g_pFrameResources;
    UINT g_numFramesInFlight = 0;
    UINT g_frameIndex = UINT_MAX;

public:
    ~Dx12ImGui()
    {
    }

    bool ImGui_ImplDX12_Init(ID3D12Device *device, int num_frames_in_flight, DXGI_FORMAT rtv_format, ID3D12DescriptorHeap *cbv_srv_heap,
                             D3D12_CPU_DESCRIPTOR_HANDLE font_srv_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE font_srv_gpu_desc_handle)
    {
        // Setup back-end capabilities flags
        ImGuiIO &io = ImGui::GetIO();
        io.BackendRendererName = "imgui_impl_dx12";
        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset; // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

        g_pd3dDevice = device;
        g_RTVFormat = rtv_format;
        g_hFontSrvCpuDescHandle = font_srv_cpu_desc_handle;
        g_hFontSrvGpuDescHandle = font_srv_gpu_desc_handle;
        g_pFrameResources.resize(num_frames_in_flight);
        g_numFramesInFlight = num_frames_in_flight;
        g_frameIndex = UINT_MAX;
        IM_UNUSED(cbv_srv_heap); // Unused in master branch (will be used by multi-viewports)

        return true;
    }

    void ImGui_ImplDX12_InvalidateDeviceObjects()
    {
        if (!g_pd3dDevice)
            return;

        g_pVertexShaderBlob.Reset();
        g_pPixelShaderBlob.Reset();
        g_pRootSignature.Reset();
        g_pPipelineState.Reset();
        g_pFontTextureResource.Reset();

        ImGuiIO &io = ImGui::GetIO();
        io.Fonts->TexID = NULL; // We copied g_pFontTextureView to io.Fonts->TexID so let's clear that as well.

        for (UINT i = 0; i < g_numFramesInFlight; i++)
        {
            FrameResources *fr = &g_pFrameResources[i];
            fr->IndexBuffer.Reset();
            fr->VertexBuffer.Reset();
        }
    }

    void ImGui_ImplDX12_NewFrame()
    {
        if (!g_pPipelineState)
        {
            ImGui_ImplDX12_CreateDeviceObjects();
        }
    }
    bool ImGui_ImplDX12_CreateDeviceObjects()
    {
        if (!g_pd3dDevice)
            return false;
        if (g_pPipelineState)
            ImGui_ImplDX12_InvalidateDeviceObjects();

        // Create the root signature
        {
            D3D12_DESCRIPTOR_RANGE descRange = {};
            descRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            descRange.NumDescriptors = 1;
            descRange.BaseShaderRegister = 0;
            descRange.RegisterSpace = 0;
            descRange.OffsetInDescriptorsFromTableStart = 0;

            D3D12_ROOT_PARAMETER param[2] = {};

            param[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
            param[0].Constants.ShaderRegister = 0;
            param[0].Constants.RegisterSpace = 0;
            param[0].Constants.Num32BitValues = 16;
            param[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

            param[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            param[1].DescriptorTable.NumDescriptorRanges = 1;
            param[1].DescriptorTable.pDescriptorRanges = &descRange;
            param[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

            D3D12_STATIC_SAMPLER_DESC staticSampler = {};
            staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            staticSampler.MipLODBias = 0.f;
            staticSampler.MaxAnisotropy = 0;
            staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
            staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
            staticSampler.MinLOD = 0.f;
            staticSampler.MaxLOD = 0.f;
            staticSampler.ShaderRegister = 0;
            staticSampler.RegisterSpace = 0;
            staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

            D3D12_ROOT_SIGNATURE_DESC desc = {};
            desc.NumParameters = _countof(param);
            desc.pParameters = param;
            desc.NumStaticSamplers = 1;
            desc.pStaticSamplers = &staticSampler;
            desc.Flags =
                D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

            ComPtr<ID3DBlob> blob = NULL;
            if (D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, NULL) != S_OK)
                return false;

            g_pd3dDevice->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&g_pRootSignature));
        }

        // By using D3DCompile() from <d3dcompiler.h> / d3dcompiler.lib, we introduce a dependency to a given version of d3dcompiler_XX.dll (see D3DCOMPILER_DLL_A)
        // If you would like to use this DX12 sample code but remove this dependency you can:
        //  1) compile once, save the compiled shader blobs into a file or source code and pass them to CreateVertexShader()/CreatePixelShader() [preferred solution]
        //  2) use code to detect any version of the DLL and grab a pointer to D3DCompile from the DLL.
        // See https://github.com/ocornut/imgui/pull/638 for sources and details.

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
        memset(&psoDesc, 0, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
        psoDesc.NodeMask = 1;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.pRootSignature = g_pRootSignature.Get();
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = g_RTVFormat;
        psoDesc.SampleDesc.Count = 1;
        psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

        // Create the vertex shader
        {
            static const char *vertexShader =
                "cbuffer vertexBuffer : register(b0) \
            {\
              float4x4 ProjectionMatrix; \
            };\
            struct VS_INPUT\
            {\
              float2 pos : POSITION;\
              float4 col : COLOR0;\
              float2 uv  : TEXCOORD0;\
            };\
            \
            struct PS_INPUT\
            {\
              float4 pos : SV_POSITION;\
              float4 col : COLOR0;\
              float2 uv  : TEXCOORD0;\
            };\
            \
            PS_INPUT main(VS_INPUT input)\
            {\
              PS_INPUT output;\
              output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\
              output.col = input.col;\
              output.uv  = input.uv;\
              return output;\
            }";

            D3DCompile(vertexShader, strlen(vertexShader), NULL, NULL, NULL, "main", "vs_5_0", 0, 0, &g_pVertexShaderBlob, NULL);
            if (g_pVertexShaderBlob == NULL) // NB: Pass ID3D10Blob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
                return false;
            psoDesc.VS = {g_pVertexShaderBlob->GetBufferPointer(), g_pVertexShaderBlob->GetBufferSize()};

            // Create the input layout
            static D3D12_INPUT_ELEMENT_DESC local_layout[] = {
                {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, (UINT)IM_OFFSETOF(ImDrawVert, pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, (UINT)IM_OFFSETOF(ImDrawVert, uv), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (UINT)IM_OFFSETOF(ImDrawVert, col), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            };
            psoDesc.InputLayout = {local_layout, 3};
        }

        // Create the pixel shader
        {
            static const char *pixelShader =
                "struct PS_INPUT\
            {\
              float4 pos : SV_POSITION;\
              float4 col : COLOR0;\
              float2 uv  : TEXCOORD0;\
            };\
            SamplerState sampler0 : register(s0);\
            Texture2D texture0 : register(t0);\
            \
            float4 main(PS_INPUT input) : SV_Target\
            {\
              float4 out_col = input.col * texture0.Sample(sampler0, input.uv); \
              return out_col; \
            }";

            D3DCompile(pixelShader, strlen(pixelShader), NULL, NULL, NULL, "main", "ps_5_0", 0, 0, &g_pPixelShaderBlob, NULL);
            if (g_pPixelShaderBlob == NULL) // NB: Pass ID3D10Blob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
                return false;
            psoDesc.PS = {g_pPixelShaderBlob->GetBufferPointer(), g_pPixelShaderBlob->GetBufferSize()};
        }

        // Create the blending setup
        {
            D3D12_BLEND_DESC &desc = psoDesc.BlendState;
            desc.AlphaToCoverageEnable = false;
            desc.RenderTarget[0].BlendEnable = true;
            desc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
            desc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
            desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
            desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
            desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
            desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
            desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        }

        // Create the rasterizer state
        {
            D3D12_RASTERIZER_DESC &desc = psoDesc.RasterizerState;
            desc.FillMode = D3D12_FILL_MODE_SOLID;
            desc.CullMode = D3D12_CULL_MODE_NONE;
            desc.FrontCounterClockwise = FALSE;
            desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
            desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
            desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
            desc.DepthClipEnable = true;
            desc.MultisampleEnable = FALSE;
            desc.AntialiasedLineEnable = FALSE;
            desc.ForcedSampleCount = 0;
            desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
        }

        // Create depth-stencil State
        {
            D3D12_DEPTH_STENCIL_DESC &desc = psoDesc.DepthStencilState;
            desc.DepthEnable = false;
            desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
            desc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
            desc.StencilEnable = false;
            desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
            desc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
            desc.BackFace = desc.FrontFace;
        }

        if (g_pd3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&g_pPipelineState)) != S_OK)
            return false;

        ImGui_ImplDX12_CreateFontsTexture();

        return true;
    }
    void ImGui_ImplDX12_CreateFontsTexture()
    {
        // Build texture atlas
        ImGuiIO &io = ImGui::GetIO();
        unsigned char *pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        // Upload texture to graphics system
        {
            D3D12_HEAP_PROPERTIES props;
            memset(&props, 0, sizeof(D3D12_HEAP_PROPERTIES));
            props.Type = D3D12_HEAP_TYPE_DEFAULT;
            props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

            D3D12_RESOURCE_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            desc.Alignment = 0;
            desc.Width = width;
            desc.Height = height;
            desc.DepthOrArraySize = 1;
            desc.MipLevels = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
            desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            desc.Flags = D3D12_RESOURCE_FLAG_NONE;

            ComPtr<ID3D12Resource> pTexture;
            g_pd3dDevice->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc,
                                                  D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&pTexture));

            UINT uploadPitch = (width * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);
            UINT uploadSize = height * uploadPitch;
            desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            desc.Alignment = 0;
            desc.Width = uploadSize;
            desc.Height = 1;
            desc.DepthOrArraySize = 1;
            desc.MipLevels = 1;
            desc.Format = DXGI_FORMAT_UNKNOWN;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
            desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            desc.Flags = D3D12_RESOURCE_FLAG_NONE;

            props.Type = D3D12_HEAP_TYPE_UPLOAD;
            props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

            ComPtr<ID3D12Resource> uploadBuffer;
            HRESULT hr = g_pd3dDevice->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc,
                                                               D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&uploadBuffer));
            IM_ASSERT(SUCCEEDED(hr));

            void *mapped = NULL;
            D3D12_RANGE range = {0, uploadSize};
            hr = uploadBuffer->Map(0, &range, &mapped);
            IM_ASSERT(SUCCEEDED(hr));
            for (int y = 0; y < height; y++)
                memcpy((void *)((uintptr_t)mapped + y * uploadPitch), pixels + y * width * 4, width * 4);
            uploadBuffer->Unmap(0, &range);

            D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
            srcLocation.pResource = uploadBuffer.Get();
            srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            srcLocation.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srcLocation.PlacedFootprint.Footprint.Width = width;
            srcLocation.PlacedFootprint.Footprint.Height = height;
            srcLocation.PlacedFootprint.Footprint.Depth = 1;
            srcLocation.PlacedFootprint.Footprint.RowPitch = uploadPitch;

            D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
            dstLocation.pResource = pTexture.Get();
            dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            dstLocation.SubresourceIndex = 0;

            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = pTexture.Get();
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

            ComPtr<ID3D12Fence> fence;
            hr = g_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
            IM_ASSERT(SUCCEEDED(hr));

            HANDLE event = CreateEvent(0, 0, 0, 0);
            IM_ASSERT(event != NULL);

            D3D12_COMMAND_QUEUE_DESC queueDesc = {};
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            queueDesc.NodeMask = 1;

            ComPtr<ID3D12CommandQueue> cmdQueue;
            hr = g_pd3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&cmdQueue));
            IM_ASSERT(SUCCEEDED(hr));

            ComPtr<ID3D12CommandAllocator> cmdAlloc;
            hr = g_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAlloc));
            IM_ASSERT(SUCCEEDED(hr));

            ComPtr<ID3D12GraphicsCommandList> cmdList;
            hr = g_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc.Get(), NULL, IID_PPV_ARGS(&cmdList));
            IM_ASSERT(SUCCEEDED(hr));

            cmdList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, NULL);
            cmdList->ResourceBarrier(1, &barrier);

            hr = cmdList->Close();
            IM_ASSERT(SUCCEEDED(hr));

            cmdQueue->ExecuteCommandLists(1, (ID3D12CommandList *const *)cmdList.GetAddressOf());
            hr = cmdQueue->Signal(fence.Get(), 1);
            IM_ASSERT(SUCCEEDED(hr));

            fence->SetEventOnCompletion(1, event);
            WaitForSingleObject(event, INFINITE);

            CloseHandle(event);

            // Create texture view
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
            ZeroMemory(&srvDesc, sizeof(srvDesc));
            srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = desc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            g_pd3dDevice->CreateShaderResourceView(pTexture.Get(), &srvDesc, g_hFontSrvCpuDescHandle);
            g_pFontTextureResource = pTexture;
        }

        // Store our identifier
        static_assert(sizeof(ImTextureID) >= sizeof(g_hFontSrvGpuDescHandle.ptr), "Can't pack descriptor handle into TexID, 32-bit not supported yet.");
        io.Fonts->TexID = (ImTextureID)g_hFontSrvGpuDescHandle.ptr;
    }
    void ImGui_ImplDX12_SetupRenderState(ImDrawData *draw_data, ID3D12GraphicsCommandList *ctx, FrameResources *fr)
    {
        // Setup orthographic projection matrix into our constant buffer
        // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right).
        VERTEX_CONSTANT_BUFFER vertex_constant_buffer;
        {
            float L = draw_data->DisplayPos.x;
            float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
            float T = draw_data->DisplayPos.y;
            float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
            float mvp[4][4] =
                {
                    {2.0f / (R - L), 0.0f, 0.0f, 0.0f},
                    {0.0f, 2.0f / (T - B), 0.0f, 0.0f},
                    {0.0f, 0.0f, 0.5f, 0.0f},
                    {(R + L) / (L - R), (T + B) / (B - T), 0.5f, 1.0f},
                };
            memcpy(&vertex_constant_buffer.mvp, mvp, sizeof(mvp));
        }

        // Setup viewport
        D3D12_VIEWPORT vp;
        memset(&vp, 0, sizeof(D3D12_VIEWPORT));
        vp.Width = draw_data->DisplaySize.x;
        vp.Height = draw_data->DisplaySize.y;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = vp.TopLeftY = 0.0f;
        ctx->RSSetViewports(1, &vp);

        // Bind shader and vertex buffers
        unsigned int stride = sizeof(ImDrawVert);
        unsigned int offset = 0;
        D3D12_VERTEX_BUFFER_VIEW vbv;
        memset(&vbv, 0, sizeof(D3D12_VERTEX_BUFFER_VIEW));
        vbv.BufferLocation = fr->VertexBuffer->GetGPUVirtualAddress() + offset;
        vbv.SizeInBytes = fr->VertexBufferSize * stride;
        vbv.StrideInBytes = stride;
        ctx->IASetVertexBuffers(0, 1, &vbv);
        D3D12_INDEX_BUFFER_VIEW ibv;
        memset(&ibv, 0, sizeof(D3D12_INDEX_BUFFER_VIEW));
        ibv.BufferLocation = fr->IndexBuffer->GetGPUVirtualAddress();
        ibv.SizeInBytes = fr->IndexBufferSize * sizeof(ImDrawIdx);
        ibv.Format = sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
        ctx->IASetIndexBuffer(&ibv);
        ctx->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->SetPipelineState(g_pPipelineState.Get());
        ctx->SetGraphicsRootSignature(g_pRootSignature.Get());
        ctx->SetGraphicsRoot32BitConstants(0, 16, &vertex_constant_buffer, 0);

        // Setup blend factor
        const float blend_factor[4] = {0.f, 0.f, 0.f, 0.f};
        ctx->OMSetBlendFactor(blend_factor);
    }
    void ImGui_ImplDX12_RenderDrawData(ImDrawData *draw_data, ID3D12GraphicsCommandList *ctx)
    {
        // Avoid rendering when minimized
        if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
            return;

        // FIXME: I'm assuming that this only gets called once per frame!
        // If not, we can't just re-allocate the IB or VB, we'll have to do a proper allocator.
        g_frameIndex = g_frameIndex + 1;
        FrameResources *fr = &g_pFrameResources[g_frameIndex % g_numFramesInFlight];

        // Create and grow vertex/index buffers if needed
        if (fr->VertexBuffer == NULL || fr->VertexBufferSize < draw_data->TotalVtxCount)
        {
            fr->VertexBuffer.Reset();
            fr->VertexBufferSize = draw_data->TotalVtxCount + 5000;
            D3D12_HEAP_PROPERTIES props;
            memset(&props, 0, sizeof(D3D12_HEAP_PROPERTIES));
            props.Type = D3D12_HEAP_TYPE_UPLOAD;
            props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            D3D12_RESOURCE_DESC desc;
            memset(&desc, 0, sizeof(D3D12_RESOURCE_DESC));
            desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            desc.Width = fr->VertexBufferSize * sizeof(ImDrawVert);
            desc.Height = 1;
            desc.DepthOrArraySize = 1;
            desc.MipLevels = 1;
            desc.Format = DXGI_FORMAT_UNKNOWN;
            desc.SampleDesc.Count = 1;
            desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            desc.Flags = D3D12_RESOURCE_FLAG_NONE;
            if (g_pd3dDevice->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&fr->VertexBuffer)) < 0)
                return;
        }
        if (fr->IndexBuffer == NULL || fr->IndexBufferSize < draw_data->TotalIdxCount)
        {
            fr->IndexBuffer.Reset();
            fr->IndexBufferSize = draw_data->TotalIdxCount + 10000;
            D3D12_HEAP_PROPERTIES props;
            memset(&props, 0, sizeof(D3D12_HEAP_PROPERTIES));
            props.Type = D3D12_HEAP_TYPE_UPLOAD;
            props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            D3D12_RESOURCE_DESC desc;
            memset(&desc, 0, sizeof(D3D12_RESOURCE_DESC));
            desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            desc.Width = fr->IndexBufferSize * sizeof(ImDrawIdx);
            desc.Height = 1;
            desc.DepthOrArraySize = 1;
            desc.MipLevels = 1;
            desc.Format = DXGI_FORMAT_UNKNOWN;
            desc.SampleDesc.Count = 1;
            desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            desc.Flags = D3D12_RESOURCE_FLAG_NONE;
            if (g_pd3dDevice->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&fr->IndexBuffer)) < 0)
                return;
        }

        // Upload vertex/index data into a single contiguous GPU buffer
        void *vtx_resource, *idx_resource;
        D3D12_RANGE range;
        memset(&range, 0, sizeof(D3D12_RANGE));
        if (fr->VertexBuffer->Map(0, &range, &vtx_resource) != S_OK)
            return;
        if (fr->IndexBuffer->Map(0, &range, &idx_resource) != S_OK)
            return;
        ImDrawVert *vtx_dst = (ImDrawVert *)vtx_resource;
        ImDrawIdx *idx_dst = (ImDrawIdx *)idx_resource;
        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList *cmd_list = draw_data->CmdLists[n];
            memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
            vtx_dst += cmd_list->VtxBuffer.Size;
            idx_dst += cmd_list->IdxBuffer.Size;
        }
        fr->VertexBuffer->Unmap(0, &range);
        fr->IndexBuffer->Unmap(0, &range);

        // Setup desired DX state
        ImGui_ImplDX12_SetupRenderState(draw_data, ctx, fr);

        // Render command lists
        // (Because we merged all buffers into a single one, we maintain our own offset into them)
        int global_vtx_offset = 0;
        int global_idx_offset = 0;
        ImVec2 clip_off = draw_data->DisplayPos;
        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList *cmd_list = draw_data->CmdLists[n];
            for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
            {
                const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[cmd_i];
                if (pcmd->UserCallback != NULL)
                {
                    // User callback, registered via ImDrawList::AddCallback()
                    // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                    if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                        ImGui_ImplDX12_SetupRenderState(draw_data, ctx, fr);
                    else
                        pcmd->UserCallback(cmd_list, pcmd);
                }
                else
                {
                    // Apply Scissor, Bind texture, Draw
                    const D3D12_RECT r = {(LONG)(pcmd->ClipRect.x - clip_off.x), (LONG)(pcmd->ClipRect.y - clip_off.y), (LONG)(pcmd->ClipRect.z - clip_off.x), (LONG)(pcmd->ClipRect.w - clip_off.y)};
                    ctx->SetGraphicsRootDescriptorTable(1, *(D3D12_GPU_DESCRIPTOR_HANDLE *)&pcmd->TextureId);
                    ctx->RSSetScissorRects(1, &r);
                    ctx->DrawIndexedInstanced(pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
                }
            }
            global_idx_offset += cmd_list->IdxBuffer.Size;
            global_vtx_offset += cmd_list->VtxBuffer.Size;
        }
    }
};

int run()
{
    Window window(L"ImGui Example");
    auto hwnd = window.Create(L"Dear ImGui DirectX12 Example", 1280, 800);
    if (!hwnd)
    {
        return 1;
    }

    D3DRenderer renderer(NUM_BACK_BUFFERS);
    if (!renderer.CreateDeviceD3D(hwnd))
    {
        return 2;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    WindowImGui imgui;
    imgui.Init(hwnd);

    Dx12ImGui dx12;
    dx12.ImGui_ImplDX12_Init(renderer.Device(), NUM_BACK_BUFFERS,
                             DXGI_FORMAT_R8G8B8A8_UNORM, renderer.SrvHeap(),
                             renderer.SrvHeap()->GetCPUDescriptorHandleForHeapStart(),
                             renderer.SrvHeap()->GetGPUDescriptorHandleForHeapStart());

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.txt' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    ScreenState state;
    while (window.Update(&state))
    {
        // Start the Dear ImGui frame
        dx12.ImGui_ImplDX12_NewFrame();
        imgui.NewFrame(hwnd, state);
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");          // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window); // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);             // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float *)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // Rendering
        {
            auto frameCtxt = renderer.Begin(&clear_color.x);
            ImGui::Render();
            dx12.ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), renderer.CommandList());
            renderer.End(frameCtxt);
        }
    }

    renderer.WaitForLastSubmittedFrame();
    // ImGui_ImplDX12_Shutdown();
    ImGui::DestroyContext();

    return 0;
}

int main(int, char **)
{
#ifdef DX12_ENABLE_DEBUG_LAYER
    EnableDebugLayer();
#endif
    auto ret = run();
#ifdef DX12_ENABLE_DEBUG_LAYER
    ReportLiveObjects();
#endif
    return ret;
}

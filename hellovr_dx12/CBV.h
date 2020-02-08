#pragma once

// Slots in the ConstantBufferView/ShaderResourceView descriptor heap
enum CBVSRVIndex_t
{
    CBV_LEFT_EYE = 0,
    CBV_RIGHT_EYE,
    SRV_LEFT_EYE,
    SRV_RIGHT_EYE,
    SRV_TEXTURE_MAP,
    // Slot for texture in each possible render model
    SRV_TEXTURE_RENDER_MODEL0,
    SRV_TEXTURE_RENDER_MODEL_MAX = SRV_TEXTURE_RENDER_MODEL0 + vr::k_unMaxTrackedDeviceCount,
    // Slot for transform in each possible rendermodel
    CBV_LEFT_EYE_RENDER_MODEL0,
    CBV_LEFT_EYE_RENDER_MODEL_MAX = CBV_LEFT_EYE_RENDER_MODEL0 + vr::k_unMaxTrackedDeviceCount,
    CBV_RIGHT_EYE_RENDER_MODEL0,
    CBV_RIGHT_EYE_RENDER_MODEL_MAX = CBV_RIGHT_EYE_RENDER_MODEL0 + vr::k_unMaxTrackedDeviceCount,
    NUM_SRV_CBVS
};

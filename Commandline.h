#pragma once

struct Commandline
{
    Commandline(int argc, char *argv[]);

    bool m_bDebugD3D12 = false;
    bool m_bVerbose = false;
    bool m_bPerf = false;
    bool m_bVblank = false;
    int m_nMSAASampleCount = 4;
    // Optional scaling factor to render with supersampling (defaults off, use -scale)
    float m_flSuperSampleScale = 1.0f;
    // if you want something other than the default 20x20x20
    int m_iSceneVolumeInit = 20;
};

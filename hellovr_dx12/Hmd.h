#pragma once
#include "Matrices.h"
#include <openvr.h>
#include <string>
#include "stdint.h"

class HMD
{
    vr::IVRSystem *m_pHMD = nullptr;

    Matrix4 m_mat4HMDPose;
    Matrix4 m_mat4eyePosLeft;
    Matrix4 m_mat4eyePosRight;

    Matrix4 m_mat4ProjectionCenter;
    Matrix4 m_mat4ProjectionLeft;
    Matrix4 m_mat4ProjectionRight;
    float m_fNearClip = 0.1f;
    float m_fFarClip = 30.0f;

    vr::TrackedDevicePose_t m_rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
    Matrix4 m_rmat4DevicePose[vr::k_unMaxTrackedDeviceCount];
    bool m_rbShowTrackedDevice[vr::k_unMaxTrackedDeviceCount];
    char m_rDevClassChar[vr::k_unMaxTrackedDeviceCount]; // for each device, a character representing its class

public:
    HMD();
    ~HMD();
    bool Initialize();
    vr::IVRSystem *Hmd() const { return m_pHMD; }
    std::string SystemName(uint32_t unTrackedDeviceIndex = 0) const;
    std::string SerialNumber(uint32_t unTrackedDeviceIndex = 0) const;
    std::string RenderModelName(uint32_t unTrackedDeviceIndex) const;
    int AdapterIndex() const;

    void SetupCameras();
    Matrix4 GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye);
    Matrix4 GetHMDMatrixPoseEye(vr::Hmd_Eye nEye);
    Matrix4 GetCurrentViewProjectionMatrix(vr::Hmd_Eye nEye);
    void UpdateHMDMatrixPose(int &m_iValidPoseCount, std::string &m_strPoseClasses);
    bool PoseIsValid(uint32_t unTrackedDevice);
    bool IsVisible(uint32_t unTrackedDevice) const
    {
        return m_rbShowTrackedDevice[unTrackedDevice];
    }
    const Matrix4 &DevicePose(uint32_t unTrackedDevice) const { return m_rmat4DevicePose[unTrackedDevice]; }
    void Update();
};

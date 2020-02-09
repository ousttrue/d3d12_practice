#include "Hmd.h"

//-----------------------------------------------------------------------------
// Purpose: Converts a SteamVR matrix to our local matrix class
//-----------------------------------------------------------------------------
static Matrix4 ConvertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t &matPose)
{
    Matrix4 matrixObj(
        matPose.m[0][0], matPose.m[1][0], matPose.m[2][0], 0.0,
        matPose.m[0][1], matPose.m[1][1], matPose.m[2][1], 0.0,
        matPose.m[0][2], matPose.m[1][2], matPose.m[2][2], 0.0,
        matPose.m[0][3], matPose.m[1][3], matPose.m[2][3], 1.0f);
    return matrixObj;
}

//-----------------------------------------------------------------------------
// Purpose: Helper to get a string from a tracked device property and turn it
//			into a std::string
//-----------------------------------------------------------------------------
static std::string GetTrackedDeviceString(vr::IVRSystem *pHmd, vr::TrackedDeviceIndex_t unDevice,
                                          vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError = NULL)
{
    uint32_t unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, NULL, 0, peError);
    if (unRequiredBufferLen == 0)
        return "";

    char *pchBuffer = new char[unRequiredBufferLen];
    unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
    std::string sResult = pchBuffer;
    delete[] pchBuffer;
    return sResult;
}

HMD::HMD()
{
    for (int nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; ++nDevice)
    {
        m_rbShowTrackedDevice[nDevice] = true;
    }
    // other initialization tasks are done in BInit
    memset(m_rDevClassChar, 0, sizeof(m_rDevClassChar));
}

HMD::~HMD()
{
    if (m_pHMD)
    {
        vr::VR_Shutdown();
        m_pHMD = NULL;
    }
}

bool HMD::Initialize()
{
    vr::EVRInitError eError = vr::VRInitError_None;
    m_pHMD = vr::VR_Init(&eError, vr::VRApplication_Scene);

    if (eError != vr::VRInitError_None)
    {
        m_pHMD = NULL;
        char buf[1024];
        sprintf_s(buf, sizeof(buf), "Unable to init VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
        return false;
    }

    return true;
}

std::string HMD::SystemName(uint32_t unTrackedDeviceIndex) const
{
    return GetTrackedDeviceString(m_pHMD, unTrackedDeviceIndex, vr::Prop_TrackingSystemName_String);
}

std::string HMD::SerialNumber(uint32_t unTrackedDeviceIndex) const
{
    return GetTrackedDeviceString(m_pHMD, unTrackedDeviceIndex, vr::Prop_SerialNumber_String);
}

std::string HMD::RenderModelName(uint32_t unTrackedDeviceIndex) const
{
    return GetTrackedDeviceString(m_pHMD, unTrackedDeviceIndex, vr::Prop_RenderModelName_String);
}

int HMD::AdapterIndex() const
{
    int nAdapterIndex;
    m_pHMD->GetDXGIOutputInfo(&nAdapterIndex);
    return nAdapterIndex;
}

Matrix4 HMD::GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye)
{
    if (!m_pHMD)
        return Matrix4();

    vr::HmdMatrix44_t mat = m_pHMD->GetProjectionMatrix(nEye, m_fNearClip, m_fFarClip);

    return Matrix4(
        mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
        mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
        mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
        mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]);
}

//-----------------------------------------------------------------------------
// Purpose: Gets an HMDMatrixPoseEye with respect to nEye.
//-----------------------------------------------------------------------------
Matrix4 HMD::GetHMDMatrixPoseEye(vr::Hmd_Eye nEye)
{
    if (!m_pHMD)
        return Matrix4();

    vr::HmdMatrix34_t matEyeRight = m_pHMD->GetEyeToHeadTransform(nEye);
    Matrix4 matrixObj(
        matEyeRight.m[0][0], matEyeRight.m[1][0], matEyeRight.m[2][0], 0.0,
        matEyeRight.m[0][1], matEyeRight.m[1][1], matEyeRight.m[2][1], 0.0,
        matEyeRight.m[0][2], matEyeRight.m[1][2], matEyeRight.m[2][2], 0.0,
        matEyeRight.m[0][3], matEyeRight.m[1][3], matEyeRight.m[2][3], 1.0f);

    return matrixObj.invert();
}

//-----------------------------------------------------------------------------
// Purpose: Gets a Current View Projection Matrix with respect to nEye,
//          which may be an Eye_Left or an Eye_Right.
//-----------------------------------------------------------------------------
Matrix4 HMD::GetCurrentViewProjectionMatrix(vr::Hmd_Eye nEye)
{
    Matrix4 matMVP;
    if (nEye == vr::Eye_Left)
    {
        matMVP = m_mat4ProjectionLeft * m_mat4eyePosLeft * m_mat4HMDPose;
    }
    else if (nEye == vr::Eye_Right)
    {
        matMVP = m_mat4ProjectionRight * m_mat4eyePosRight * m_mat4HMDPose;
    }

    return matMVP;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int HMD::UpdateHMDMatrixPose()
{
    if (!m_pHMD)
        return 0;

    vr::VRCompositor()->WaitGetPoses(m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);

    auto m_iValidPoseCount = 0;
    m_strPoseClasses = "";
    for (int nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; ++nDevice)
    {
        if (m_rTrackedDevicePose[nDevice].bPoseIsValid)
        {
            m_iValidPoseCount++;
            m_rmat4DevicePose[nDevice] = ConvertSteamVRMatrixToMatrix4(m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking);
            if (m_rDevClassChar[nDevice] == 0)
            {
                switch (m_pHMD->GetTrackedDeviceClass(nDevice))
                {
                case vr::TrackedDeviceClass_Controller:
                    m_rDevClassChar[nDevice] = 'C';
                    break;
                case vr::TrackedDeviceClass_HMD:
                    m_rDevClassChar[nDevice] = 'H';
                    break;
                case vr::TrackedDeviceClass_Invalid:
                    m_rDevClassChar[nDevice] = 'I';
                    break;
                case vr::TrackedDeviceClass_GenericTracker:
                    m_rDevClassChar[nDevice] = 'G';
                    break;
                case vr::TrackedDeviceClass_TrackingReference:
                    m_rDevClassChar[nDevice] = 'T';
                    break;
                default:
                    m_rDevClassChar[nDevice] = '?';
                    break;
                }
            }
            m_strPoseClasses += m_rDevClassChar[nDevice];
        }
    }

    if (m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
    {
        m_mat4HMDPose = m_rmat4DevicePose[vr::k_unTrackedDeviceIndex_Hmd];
        m_mat4HMDPose.invert();
    }

    return m_iValidPoseCount;
}
void HMD::SetupCameras()
{
    m_mat4ProjectionLeft = GetHMDMatrixProjectionEye(vr::Eye_Left);
    m_mat4ProjectionRight = GetHMDMatrixProjectionEye(vr::Eye_Right);
    m_mat4eyePosLeft = GetHMDMatrixPoseEye(vr::Eye_Left);
    m_mat4eyePosRight = GetHMDMatrixPoseEye(vr::Eye_Right);
}

bool HMD::PoseIsValid(uint32_t unTrackedDevice)
{
    return m_rTrackedDevicePose[unTrackedDevice].bPoseIsValid;
}

void HMD::Update()
{
    for (vr::TrackedDeviceIndex_t unDevice = 0; unDevice < vr::k_unMaxTrackedDeviceCount; unDevice++)
    {
        vr::VRControllerState_t state;
        if (m_pHMD->GetControllerState(unDevice, &state, sizeof(state)))
        {
            m_rbShowTrackedDevice[unDevice] = state.ulButtonPressed == 0;
        }
    }
}

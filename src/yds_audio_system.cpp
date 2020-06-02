#include "../include/yds_audio_system.h"

#include "../include/yds_audio_device.h"
#include "../include/yds_ds8_system.h"

ysAudioSystem::ysAudioSystem() : ysAudioSystemObject("AUDIO_SYSTEM", API::Undefined) {
    /* void */
}

ysAudioSystem::ysAudioSystem(API api) : ysAudioSystemObject("AUDIO_SYSTEM", api) {
    /* void */
}

ysAudioSystem::~ysAudioSystem() {
    /* void */
}

ysError ysAudioSystem::CreateAudioSystem(ysAudioSystem **newAudioSystem, API api) {
    YDS_ERROR_DECLARE("CreateAudioSystem");

    if (newAudioSystem == nullptr) return YDS_ERROR_RETURN_STATIC(ysError::YDS_INVALID_PARAMETER);
    *newAudioSystem = nullptr;

    if (api == API::Undefined) return YDS_ERROR_RETURN_STATIC(ysError::YDS_INVALID_PARAMETER);

    switch (api) {
    case API::DirectSound8:
        *newAudioSystem = new ysDS8System;
        break;
    default:
        *newAudioSystem = nullptr;
        break;
    }

    return YDS_ERROR_RETURN_STATIC(ysError::YDS_NO_ERROR);
}

ysError ysAudioSystem::DestroyAudioSystem(ysAudioSystem **audioSystem) {
    YDS_ERROR_DECLARE("DestroyAudioSystem");

    if (audioSystem == nullptr) return YDS_ERROR_RETURN_STATIC(ysError::YDS_INVALID_PARAMETER);

    delete *audioSystem;
    *audioSystem = nullptr;

    return YDS_ERROR_RETURN_STATIC(ysError::YDS_NO_ERROR);
}

void ysAudioSystem::EnumerateDevices() {
    m_devices.Clear();
}

ysAudioDevice *ysAudioSystem::GetPrimaryDevice() {
    if (m_devices.GetNumObjects() == 0) return 0;
    else {
        // Zeroeth device is always the default
        return m_devices.Get(0);
    }
}

ysAudioDevice *ysAudioSystem::GetAuxDevice(int device) {
    if ((device + 1) >= GetDeviceCount()) return 0;
    return m_devices.Get(device + 1);
}

void ysAudioSystem::ConnectDevice(ysAudioDevice *device, ysWindow *windowAssociation) {
    device->m_connected = true;
    device->m_windowAssociation = windowAssociation;
}

void ysAudioSystem::ConnectDeviceConsole(ysAudioDevice *device) {
    device->m_connected = true;
    device->m_windowAssociation = NULL;
}

void ysAudioSystem::DisconnectDevice(ysAudioDevice *device) {
    device->m_connected = false;
    device->m_windowAssociation = 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <setupapi.h>
#include <ks.h>
#include <ksmedia.h>
#include <mmdeviceapi.h>
#include <initguid.h>  // To define the GUIDs

// Define the GUID for the audio device interface
//DEFINE_GUID(GUID_DEVINTERFACE_AUDIO_RENDER, 0xe6327cad, 0xdcec, 0x4949, 0xae, 0x8a, 0x99, 0x1e, 0x97, 0x82, 0x96, 0x46);
DEFINE_GUID(DEVINTERFACE_AUDIO_RENDER, 0xe6327cad,0xdcec,0x4949,0xae,0x8a,0x99,0x1e,0x97,0x6a,0x79,0xd2);

HANDLE OpenAudioDevice() {
    HDEVINFO deviceInfoSet;
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA deviceInterfaceDetailData = NULL;
    DWORD requiredSize = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;
	
    deviceInfoSet = SetupDiGetClassDevsExA(&DEVINTERFACE_AUDIO_RENDER, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE,NULL,NULL,0);
    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "SetupDiGetClassDevs failed\n");
        return INVALID_HANDLE_VALUE;
    }

    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    if (!SetupDiEnumDeviceInterfaces(deviceInfoSet, NULL, &DEVINTERFACE_AUDIO_RENDER, 0, &deviceInterfaceData)) {
        fprintf(stderr, "SetupDiEnumDeviceInterfaces failed\n");
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        return INVALID_HANDLE_VALUE;
    }

    SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, NULL, 0, &requiredSize, NULL);
    deviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(requiredSize);
    if (!deviceInterfaceDetailData) {
        fprintf(stderr, "Memory allocation failed\n");
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        return INVALID_HANDLE_VALUE;
    }

    deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
    if (!SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, deviceInterfaceDetailData, requiredSize, &requiredSize, NULL)) {
        fprintf(stderr, "SetupDiGetDeviceInterfaceDetail failed\n");
        free(deviceInterfaceDetailData);
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        return INVALID_HANDLE_VALUE;
    }

    hDevice = CreateFile(deviceInterfaceDetailData->DevicePath, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "CreateFile failed\n");
    }

    free(deviceInterfaceDetailData);
    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    return hDevice;
}

void PlayAudioStream(HANDLE hDevice) {
    KSPIN_CONNECT pinConnect;
    KSDATAFORMAT_WAVEFORMATEX dataFormat;
    DWORD bytesReturned;
    BYTE buffer[4096];
    size_t bytesRead;

    // Initialize the pin connection and data format
    pinConnect.Interface.Set = KSINTERFACESETID_Standard;
    pinConnect.Interface.Id = KSINTERFACE_STANDARD_STREAMING;
    pinConnect.Interface.Flags = 0;

    pinConnect.Medium.Set = KSMEDIUMSETID_Standard;
    pinConnect.Medium.Id = KSMEDIUM_STANDARD_DEVIO;
    pinConnect.Medium.Flags = 0;

    pinConnect.PinId = 0;  // Adjust according to your audio device
    pinConnect.PinToHandle = NULL;
    pinConnect.Priority.PriorityClass = KSPRIORITY_NORMAL;
    pinConnect.Priority.PrioritySubClass = 0;

    dataFormat.DataFormat.FormatSize = sizeof(KSDATAFORMAT_WAVEFORMATEX);
    dataFormat.DataFormat.Flags = 0;
    dataFormat.DataFormat.SampleSize = 4;  // 2 channels * 2 bytes per sample
    dataFormat.DataFormat.Reserved = 0;
    dataFormat.DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
    dataFormat.DataFormat.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    dataFormat.DataFormat.Specifier = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;

    dataFormat.WaveFormatEx.wFormatTag = WAVE_FORMAT_PCM;
    dataFormat.WaveFormatEx.nChannels = 2;
    dataFormat.WaveFormatEx.nSamplesPerSec = 44100;
    dataFormat.WaveFormatEx.nAvgBytesPerSec = 44100 * 4;
    dataFormat.WaveFormatEx.nBlockAlign = 4;
    dataFormat.WaveFormatEx.wBitsPerSample = 16;
    dataFormat.WaveFormatEx.cbSize = 0;

    if (!DeviceIoControl(hDevice, IOCTL_KS_PROPERTY, &pinConnect, sizeof(pinConnect), &dataFormat, sizeof(dataFormat), &bytesReturned, NULL)) {
        fprintf(stderr, "Failed to connect to pin and set data format\n");
        return;
    }

    while ((bytesRead = fread(buffer, sizeof(BYTE), sizeof(buffer), stdin)) > 0) {
        if (!WriteFile(hDevice, buffer, bytesRead, &bytesReturned, NULL)) {
            fprintf(stderr, "Failed to write audio data\n");
            break;
        }
    }
}

int main() {
    HANDLE hDevice = OpenAudioDevice();
    if (hDevice == INVALID_HANDLE_VALUE) {
        return 1;
    }

    PlayAudioStream(hDevice);

    CloseHandle(hDevice);
    return 0;
}


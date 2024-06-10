#include <windows.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <initguid.h>  // To define the GUIDs
#include <fcntl.h>

// Define CD audio format
#define SAMPLE_RATE 44100
#define CHANNELS 2
#define BITS_PER_SAMPLE 16
#define BUFFER_SIZE 8192

// Error handling macro

#define HRCK(hr) HRCK_(hr,__FILE__,__LINE__)
#define HRCK_(hr,file,line) if FAILED(hr){fprintf(stderr, "Error in %s, line %d: %d",file,line,hr); goto cleanup;}
#define CHECK_HR(hr, msg) if (FAILED(hr)) { printf("%s Error: 0x%lx\n", msg, hr); goto cleanup; }

DEFINE_GUID(IID_IAudioClient, 0x1cb9ad4c, 0xdbfa, 0x4c32, 0xb1,0x78, 0xc2,0xf5,0x68,0xa7,0x03,0xb2);
DEFINE_GUID(IID_IAudioRenderClient, 0xf294acfc, 0x3146, 0x4483, 0xa7,0xbf, 0xad,0xdc,0xa7,0xc2,0x60,0xe2);
DEFINE_GUID(CLSID_MMDeviceEnumerator, 0xbcde0395, 0xe52f, 0x467c, 0x8e,0x3d, 0xc4,0x57,0x92,0x91,0x69,0x2e);
DEFINE_GUID(IID_IMMDeviceEnumerator, 0xa95664d2, 0x9614, 0x4f35, 0xa7,0x46, 0xde,0x8d,0xb6,0x36,0x17,0xe6);

int main() {
	 _setmode(_fileno(stdin), _O_BINARY); //required
    HRESULT hr;
    IMMDeviceEnumerator *pEnumerator = NULL;
    IMMDevice *pDevice = NULL;
    IAudioClient *pAudioClient = NULL;
    IAudioRenderClient *pRenderClient = NULL;
    WAVEFORMATEX waveFormat;
    UINT32 bufferFrameCount;
    BYTE *pData;
    DWORD flags = 0;

    HRCK(CoInitialize(NULL));
    HRCK(CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator, (void**)&pEnumerator));
    HRCK(pEnumerator->lpVtbl->GetDefaultAudioEndpoint(pEnumerator, eRender, eConsole, &pDevice));
    HRCK(pDevice->lpVtbl->Activate(pDevice, &IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient));

    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = CHANNELS;
    waveFormat.nSamplesPerSec = SAMPLE_RATE;
    waveFormat.wBitsPerSample = BITS_PER_SAMPLE;
    waveFormat.nBlockAlign = (CHANNELS * BITS_PER_SAMPLE) / 8;
    waveFormat.nAvgBytesPerSec = SAMPLE_RATE * waveFormat.nBlockAlign;
    waveFormat.cbSize = 0;

    HRCK(pAudioClient->lpVtbl->Initialize(pAudioClient, AUDCLNT_SHAREMODE_SHARED, 0, 10000000, 0, &waveFormat, NULL)); // 1 s
	 HRCK(pAudioClient->lpVtbl->GetBufferSize(pAudioClient, &bufferFrameCount));
    HRCK(pAudioClient->lpVtbl->GetService(pAudioClient, &IID_IAudioRenderClient, (void**)&pRenderClient));
    HRCK(pAudioClient->lpVtbl->Start(pAudioClient));

    // Main loop to read from stdin and write to the audio buffer
    while (1) {
        BYTE buffer[BUFFER_SIZE];
        size_t bytesRead = fread(buffer, 1, BUFFER_SIZE, stdin);
        if (bytesRead == 0) break;  // Exit loop if no more data

        UINT32 numFramesPadding;
        hr = pAudioClient->lpVtbl->GetCurrentPadding(pAudioClient, &numFramesPadding);
        CHECK_HR(hr, "GetCurrentPadding");
		  while(numFramesPadding + (BUFFER_SIZE/2) > bufferFrameCount){
			  Sleep(1000 / 24);
			  hr = pAudioClient->lpVtbl->GetCurrentPadding(pAudioClient, &numFramesPadding);
			  CHECK_HR(hr, "GetCurrentPadding");
		  }

		  printf("%d %d %d\n",bufferFrameCount,bytesRead,numFramesPadding);

        UINT32 numFramesAvailable = bufferFrameCount - numFramesPadding;
        UINT32 numFramesToWrite = bytesRead / waveFormat.nBlockAlign;

        if (numFramesToWrite > numFramesAvailable) {
            numFramesToWrite = numFramesAvailable;
        }

        HRCK(pRenderClient->lpVtbl->GetBuffer(pRenderClient, numFramesToWrite, &pData));
        memcpy(pData, buffer, numFramesToWrite * waveFormat.nBlockAlign);
        HRCK(pRenderClient->lpVtbl->ReleaseBuffer(pRenderClient, numFramesToWrite, flags));
    }
    HRCK(pAudioClient->lpVtbl->Stop(pAudioClient));

cleanup:
    // Clean up
    if (pRenderClient) pRenderClient->lpVtbl->Release(pRenderClient);
    if (pAudioClient) pAudioClient->lpVtbl->Release(pAudioClient);
    if (pDevice) pDevice->lpVtbl->Release(pDevice);
    if (pEnumerator) pEnumerator->lpVtbl->Release(pEnumerator);
    CoUninitialize();
    return hr == S_OK ? 0 : 1;
}


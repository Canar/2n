#include <windows.h>
#include <xaudio2.h>
#include <audiosessiontypes.h>
#include <stdio.h>
#include <fcntl.h>

#define BUFFER_SIZE 4096

// Error handling macro
#define CHECK_HR(hr) if (FAILED(hr)) { fprintf(stderr, "Error: 0x%08X\n", hr); return hr; }

// Function pointers for XAudio2
typedef HRESULT (WINAPI *XAudio2CreateProc)(IXAudio2**, UINT32, XAUDIO2_PROCESSOR);

int main() {
	 _setmode(_fileno(stdin), _O_BINARY); //required
    HRESULT hr;
    IXAudio2 *pXAudio2 = NULL;
    IXAudio2MasteringVoice *pMasteringVoice = NULL;
    IXAudio2SourceVoice *pSourceVoice = NULL;
    BYTE buffer[BUFFER_SIZE];
    XAUDIO2_BUFFER audioBuffer = { 0 };
    WAVEFORMATEX waveFormat = { 0 };

    // Load XAudio2 library
 /*   HMODULE hXAudio2 = LoadLibraryA("XAudio2_9.dll");
    if (!hXAudio2) {
        fprintf(stderr, "Failed to load XAudio2_9.dll\n");
        return -1;
    }
*/
    // Get XAudio2Create function
/*    XAudio2CreateProc XAudio2Create = (XAudio2CreateProc)GetProcAddress(hXAudio2, "XAudio2Create");
    if (!XAudio2Create) {
        fprintf(stderr, "Failed to get XAudio2Create function\n");
        FreeLibrary(hXAudio2);
        return -1;
    }
*/	 //
    // Initialize XAudio2
    hr = XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
    CHECK_HR(hr);

    // Initialize COM
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    // Initialize XAudio2
    hr = XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
    CHECK_HR(hr);

    // Create a mastering voice
//    hr = IXAudio2_CreateMasteringVoice(pXAudio2, &pMasteringVoice, XAUDIO2_DEFAULT_CHANNELS, 44100, 0, 0, NULL);
hr = pXAudio2->lpVtbl->CreateMasteringVoice(pXAudio2, &pMasteringVoice, XAUDIO2_DEFAULT_CHANNELS, 44100, 0, 0, NULL, 0);
    CHECK_HR(hr);

    // Set up the wave format for CD Audio
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = 2;
    waveFormat.nSamplesPerSec = 44100;
    waveFormat.nAvgBytesPerSec = 44100 * 2 * 2;
    waveFormat.nBlockAlign = 4;
    waveFormat.wBitsPerSample = 16;
    waveFormat.cbSize = 0;

    // Create a source voice
    hr = pXAudio2->lpVtbl->CreateSourceVoice(pXAudio2, &pSourceVoice, &waveFormat, 0, XAUDIO2_DEFAULT_FREQ_RATIO, NULL, NULL, NULL);
    CHECK_HR(hr);

    // Start the source voice
    hr = pSourceVoice->lpVtbl->Start(pSourceVoice, 0, 0);
    CHECK_HR(hr);

    // Main loop to read PCM data from stdin and play it
    while (1) {
        size_t bytesRead = fread(buffer, 1, BUFFER_SIZE, stdin);
        if (bytesRead == 0) {
            break;  // EOF or error
        }

        audioBuffer.AudioBytes = (UINT32)bytesRead;
        audioBuffer.pAudioData = buffer;
        audioBuffer.Flags = 0;  // Clear the flag (not end of stream)
        
        hr = pSourceVoice->lpVtbl->SubmitSourceBuffer(pSourceVoice, &audioBuffer, NULL);
        CHECK_HR(hr);

        // Wait until the buffer is processed
        XAUDIO2_VOICE_STATE state;
        do {
            pSourceVoice->lpVtbl->GetState(pSourceVoice, &state, 0);
            Sleep(10);
        } while (state.BuffersQueued > 0);
    }

    // Clean up
    pSourceVoice->lpVtbl->DestroyVoice(pSourceVoice);
    pMasteringVoice->lpVtbl->DestroyVoice(pMasteringVoice);
    pXAudio2->lpVtbl->Release(pXAudio2);
    CoUninitialize();

    return 0;
}


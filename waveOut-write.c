//to test: ffmpeg -i audio.file -f s16le - | waveOut-write.exe

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>

#define BLK_N 8
#define BLK_LEN 16384

int main(){
	char *buffer;
	long fileSize;

	fseek(stdin, 0, SEEK_END);
	fileSize = ftell(stdin);
	fseek(stdin, 0, SEEK_SET);
	buffer = (char*) calloc(1, fileSize+1);
	fread(buffer, 1, fileSize, stdin);

	WAVEFORMATEX wf;
	WAVEHDR wh;
	HWAVEOUT hWaveOut;

	wf.wFormatTag = WAVE_FORMAT_PCM;
	wf.nChannels = 2;
	wf.nSamplesPerSec = 44100;
	wf.nAvgBytesPerSec = 44100 * 2 * 2;
	wf.nBlockAlign = 4;
	wf.wBitsPerSample = 16;
	wf.cbSize = 0;

	wh.lpData = buffer;
	wh.dwBufferLength = fileSize;
	wh.dwFlags = 0;
	wh.dwLoops = 0;

	waveOutOpen(&hWaveOut,WAVE_MAPPER,&wf,0,0,CALLBACK_NULL);
	waveOutPrepareHeader(hWaveOut,&wh,sizeof(wh));
	waveOutWrite(hWaveOut,&wh,sizeof(wh));

	do {} while (!(wh.dwFlags & WHDR_DONE));

	waveOutUnprepareHeader(hWaveOut,&wh,sizeof(wh));
	waveOutClose(hWaveOut);

	free(buffer);

	return 0;
}

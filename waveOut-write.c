//to test: ffmpeg -i audio.file -f s16le test.raw ; waveOut-write.exe test.raw
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>

int main(int argc,char** argv){
	char *buffer;
	long fileSize;

	FILE *fp = fopen(argv[1], "rb");
	fseek(fp, 0, SEEK_END);
	fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	buffer = (char*) calloc(1, fileSize+1);
	fread(buffer, 1, fileSize, fp);
	fclose(fp);

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

	do {}
	while (!(wh.dwFlags & WHDR_DONE));

	waveOutUnprepareHeader(hWaveOut,&wh,sizeof(wh));
	waveOutClose(hWaveOut);

	free(buffer);

	return 0;
}

//to test: ffmpeg -i audio.file -f s16le - | waveOut-write.exe

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <fcntl.h>

#define BLK_N 8
#define BLK_LEN 16384

int main(){
	char *buffer=calloc(BLK_N,BLK_LEN);
	char *buf[BLK_N];
	for (int i=0;i<BLK_N;i++)
		buf[i]=buffer+(i*BLK_LEN);

	WAVEFORMATEX wf;
	wf.wFormatTag = WAVE_FORMAT_PCM;
	wf.nBlockAlign = ( wf.wBitsPerSample = 16 ) >> 3 * ( wf.nChannels = 2);
	wf.nAvgBytesPerSec = ( wf.nSamplesPerSec = 44100 ) * wf.nBlockAlign;
	wf.cbSize = 0;

	WAVEHDR wh[BLK_N];
	for (int i=0;i<BLK_N;i++){
		wh[i].lpData = buf[i];
		wh[i].dwBufferLength = BLK_LEN;
		wh[i].dwFlags = (wh[i].dwLoops = 0);
	}


	HWAVEOUT hWaveOut;
	waveOutOpen(&hWaveOut,WAVE_MAPPER,&wf,0,0,CALLBACK_NULL);

	int l;
	_setmode(_fileno(stdin), _O_BINARY);

	for (int i=0;i<BLK_N;i++){
		l=fread(buf[i],1,BLK_LEN,stdin);
	}

	for (int i=0;i<BLK_N;i++){
		waveOutPrepareHeader(hWaveOut,&wh[i],sizeof(wh));
	}

	for (int i=0;i<BLK_N;i++){
		waveOutWrite(hWaveOut,&wh[i],sizeof(wh));
	}

	int ef=0;
	do {
		ef=0;
		for (int i=0;i<BLK_N;i++){
			if(wh[i].dwFlags & WHDR_DONE){
				waveOutUnprepareHeader(hWaveOut,&wh[i],sizeof(wh));
				l=fread(buf[i],1,BLK_LEN,stdin);
				wh[i].dwBufferLength = l;
				ef=l<1;
				waveOutPrepareHeader(hWaveOut,&wh[i],sizeof(wh));
				waveOutWrite(hWaveOut,&wh[i],sizeof(wh));
				break;
			};
		}
	} while (!ef);

	for (int i=0;i<BLK_N;i++)
		waveOutUnprepareHeader(hWaveOut,&wh[i],sizeof(wh));

	waveOutClose(hWaveOut);

	free(buffer);

	return 0;
}

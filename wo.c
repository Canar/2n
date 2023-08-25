//to test: ffmpeg -i audio.file -f s16le - | waveOut-write.exe

#define WIN32_LEAN_AND_MEAN

#define BPS        16
#define RATE_      44100
#define CHAN_      2

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <fcntl.h>

#define BLK_N 8
#define BLK_LEN 16384

int main(){
	char buffer[BLK_N*BLK_LEN];
	char *buf[BLK_N];
	for (int i=0;i<BLK_N;i++)
		buf[i]=buffer+(i*BLK_LEN);

	WAVEFORMATEX wf;
	wf.wFormatTag = WAVE_FORMAT_PCM;
	wf.nBlockAlign = ( wf.wBitsPerSample = BPS ) >> 3 * ( wf.nChannels = CHAN_ );
	wf.nAvgBytesPerSec = ( wf.nSamplesPerSec = RATE_ ) * wf.nBlockAlign;
	wf.cbSize = 0;

	WAVEHDR wh[BLK_N];
	for (int i=0;i<BLK_N;i++){
		wh[i].lpData = buf[i];
		wh[i].dwBufferLength = BLK_LEN;
		wh[i].dwFlags = WHDR_DONE;
		wh[i].dwLoops = 0;
	}

	_setmode(_fileno(STDIN), _O_BINARY); // required
	HWAVEOUT hWaveOut;
	waveOutOpen(&hWaveOut,WAVE_MAPPER,&wf,0,0,CALLBACK_NULL);
	int ef;
	do {
		ef=0;
		for (int i=0;i<BLK_N;i++)
			if(wh[i].dwFlags & WHDR_DONE){
				waveOutUnprepareHeader(hWaveOut,&wh[i],sizeof(wh));
				wh[i].dwBufferLength = fread(buf[i],1,BLK_LEN,stdin);
				ef=wh[i].dwBufferLength<1;
				waveOutPrepareHeader(hWaveOut,&wh[i],sizeof(wh));
				waveOutWrite(hWaveOut,&wh[i],sizeof(wh));
				break;
			}
	} while (!ef);

	for (int i=0;i<BLK_N;i++) waveOutUnprepareHeader(hWaveOut,&wh[i],sizeof(wh));
	waveOutClose(hWaveOut);
	return 0;
}

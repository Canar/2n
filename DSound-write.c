
#define WIN32_LEAN_AND_MEAN

#include "config.linux.h"
#include "config.mingw.h"

#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024*256
#define BUFFER_SIZE_BYTES BUFFER_SIZE * CHAN_ * BPS / 8 

int trunc_align(int val,int mult){ return ( val / mult ) * mult ; }
void check_result(HRESULT result,char* file,int line) {
    if (result != DS_OK) 
		 fprintf(stderr, "Error in %s, line %d: %d",file,line,result);

}
#define DSCK(x) check_result(result = (x),__FILE__,__LINE__)
void wf_init(WAVEFORMATEX* wf){
	wf->wFormatTag = WAVE_FORMAT_PCM;
	wf->nChannels = CHAN_;
	wf->nSamplesPerSec = RATE_;
	wf->wBitsPerSample = BPS;
	wf->nBlockAlign = wf->nChannels * (wf->wBitsPerSample / 8);
	wf->nAvgBytesPerSec = wf->nSamplesPerSec * wf->nBlockAlign;
}
void dsbd_init(DSBUFFERDESC* dsbd,WAVEFORMATEX* wf){
	dsbd->dwSize = sizeof(DSBUFFERDESC);
	dsbd->dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLVOLUME | DSBCAPS_GETCURRENTPOSITION2;
	dsbd->dwBufferBytes = BUFFER_SIZE_BYTES; // Buffer size in bytes
	dsbd->lpwfxFormat = wf;
}


int main(int argc, char** argv) {
	LPDIRECTSOUND lpds;
	HRESULT result;
	DSCK(DirectSoundCreate(0,&lpds,0));	
	struct IDirectSoundVtbl* dst = lpds->lpVtbl;
	
	HWND hwnd = GetConsoleWindow();
	DSCK(dst->SetCooperativeLevel(lpds, hwnd, DSSCL_PRIORITY));

	WAVEFORMATEX wf = {0}; wf_init(&wf);
	DSBUFFERDESC dsbdesc = {0};dsbd_init(&dsbdesc,&wf);

	LPDIRECTSOUNDBUFFER lpdsb;
	DSCK(lpds->lpVtbl->CreateSoundBuffer(lpds, &dsbdesc, &lpdsb, NULL));
	 
	DWORD playCursor, writeCursor, bufferSize, lockSize, wanted, written_to, written, bytes_read;
	BYTE buffer[BUFFER_SIZE_BYTES];
	_setmode(_fileno(stdin), _O_BINARY); //required

	LPVOID lpb;
	DWORD szb;
	DSCK(lpdsb->lpVtbl->Lock(lpdsb, 0, BUFFER_SIZE_BYTES, &lpb, &szb, 0, 0, DSBLOCK_ENTIREBUFFER));
	DSCK(lpdsb->lpVtbl->Unlock(lpdsb, lpb, bytes_read=fread(lpb, 1, szb, stdin), 0, 0));
	DSCK(lpdsb->lpVtbl->Play(lpdsb, 0, 0, DSBPLAY_LOOPING));

	written_to=0;

#define ACTFRAC BUFFER_SIZE_BYTES / 5

	while (bytes_read>0) {
		DSCK(lpdsb->lpVtbl->GetCurrentPosition(lpdsb, &playCursor, &writeCursor));
		
		if(written_to==0){ // if you've written to the end
			if(playCursor < ACTFRAC){ // and it isn't part played yet, sleep
				wanted=0;
			}else{ // otherwise, catch up
				wanted=trunc_align(playCursor - 1,4096);
			}
		}else{
			if(playCursor < written_to){ // if the cursor is behind where is written to, try remedy that
				wanted=BUFFER_SIZE_BYTES - written_to;
			}else{
				wanted=trunc_align(playCursor - written_to - 1,1024); // otherwise, just try keep up
			}
		}

		if(( wanted < ACTFRAC) && (!(wanted + written_to == BUFFER_SIZE_BYTES))){ Sleep(100); }else{
			DSCK(lpdsb->lpVtbl->Lock(lpdsb, written_to, wanted, &lpb, &szb, 0, 0, 0 ));
			DSCK(lpdsb->lpVtbl->Unlock(lpdsb, lpb, bytes_read = fread(lpb, 1, szb, stdin), 0, 0));
			written_to = (written_to+bytes_read)==BUFFER_SIZE_BYTES ? 0 : written_to+bytes_read;
			if(bytes_read==0) memset(lpb,1024,0);
		}
	}
	Sleep(((BUFFER_SIZE_BYTES-playCursor)+written_to) * 10 / 441 / 4);
	DSCK(lpdsb->lpVtbl->Stop(lpdsb));
	return 0;
}


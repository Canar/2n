
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

#define BUFF 1024
#define BUFFER_SIZE BUFF*256
#define BUFFER_SIZE_BYTES BUFFER_SIZE * CHAN_ * BPS / 8 
#define ACTFRAC BUFFER_SIZE_BYTES / 5 //what proportion of the buffer is worth writing to?

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
	dsbd->dwBufferBytes = BUFFER_SIZE_BYTES;
	dsbd->lpwfxFormat = wf;
}

int main(int argc, char** argv) {
	_setmode(_fileno(stdin), _O_BINARY); //required

	LPDIRECTSOUND lpds;
	HRESULT result;
	DSCK(DirectSoundCreate(0,&lpds,0));	
	
	HWND hwnd = GetConsoleWindow();
	struct IDirectSoundVtbl* dst = lpds->lpVtbl;
	DSCK(dst->SetCooperativeLevel(lpds, hwnd, DSSCL_PRIORITY));

	WAVEFORMATEX wf={0}; wf_init(&wf);
	DSBUFFERDESC dsbdesc={0}; dsbd_init(&dsbdesc,&wf);

	LPDIRECTSOUNDBUFFER lpdsb;
	DSCK(dst->CreateSoundBuffer(lpds, &dsbdesc, &lpdsb, NULL));
	 
	void* data;
	DWORD size,bytes_read,written_to=0,play_cursor,new_written_to,wanted;
	struct IDirectSoundBufferVtbl* dsbt=lpdsb->lpVtbl;

	DSCK(dsbt->Lock(lpdsb, 0, BUFFER_SIZE_BYTES, &data, &size, 0, 0, DSBLOCK_ENTIREBUFFER));
	DSCK(dsbt->Unlock(lpdsb, data, bytes_read=fread(data, 1, (size_t)size, stdin),0,0));
	DSCK(dsbt->Play(lpdsb, 0, 0, DSBPLAY_LOOPING));

	while (bytes_read>0) {
		DSCK(dsbt->GetCurrentPosition(lpdsb, &play_cursor, 0));
		
		wanted= written_to==0 ? ( (                      //if you've written to the end
			play_cursor < ACTFRAC ) ? (                   //and it isn't part played yet
				0 ) : (                                    //signal sleep,
				trunc_align(play_cursor-1,BUFF) )          //otherwise, catch up,
			) : ( play_cursor < written_to ? (            //but if you can write to the end,
				BUFFER_SIZE_BYTES - written_to ) : (       //do so,
				trunc_align(play_cursor-written_to-1,BUFF) //otherwise, write to the cursor
		) ) ;

		if(( wanted < ACTFRAC )&&(!( wanted+written_to == BUFFER_SIZE_BYTES ))){ //ignore small write unless it fills buffer
			Sleep(1000 / 24); //cinematic!
		}else{
			DSCK(dsbt->Lock(lpdsb,written_to,wanted,&data,&size,0,0,0));
			DSCK(dsbt->Unlock(lpdsb,data,bytes_read = fread(data,1,size,stdin),0,0));
			new_written_to=written_to+bytes_read;
			written_to = new_written_to==BUFFER_SIZE_BYTES ? 0 : new_written_to;
			if(bytes_read==0){ //EOF -> write a little silence at the end so imprecisely timed stop doesn't matter
				DSCK(dsbt->Lock(lpdsb,written_to,wanted,&data,&size,0,0,0));
				memset(data,0,size);
				DSCK(dsbt->Unlock(lpdsb,data,size,0,0));
			}
		}
	}
	Sleep(((BUFFER_SIZE_BYTES-play_cursor)+new_written_to) * 10 / 441 / 4);
	DSCK(dsbt->Stop(lpdsb));
	return 0;
}


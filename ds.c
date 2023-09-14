#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>

#define BPS	16
#define RATE	44100
#define CHAN	2
#define BLK_N	8
#define BLK_LEN	16384

#define iDirectSoundCreate(a,b,c)	pDirectSoundCreate(a,b,c)

HRESULT (WINAPI *pDirectSoundCreate)(GUID FAR *lpGUID, LPDIRECTSOUND FAR *lplpDS, IUnknown FAR *pUnkOuter);
void check(int check,char* msg){
	if(check){
		printf("%s\n",msg);
		exit(1);
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
/*int main(){*/
	WAVEFORMATEX wf;
	memset(&wf, 0, sizeof(wf));
	wf.wFormatTag = WAVE_FORMAT_PCM;
	wf.nBlockAlign = ( wf.wBitsPerSample = BPS ) >> 3 * ( wf.nChannels = CHAN );
	wf.nAvgBytesPerSec = ( wf.nSamplesPerSec = RATE ) * wf.nBlockAlign;

	DWORD			dwSize, dwWrite;
	DSCAPS			dscaps;
	WAVEFORMATEX	format, pformat; 
	HRESULT			hresult;

	HINSTANCE hInstDS;
	check(!(hInstDS = LoadLibrary("dsound.dll")),"Couldn't load dsound.dll");
	check(!(pDirectSoundCreate = (void *)GetProcAddress(hInstDS,"DirectSoundCreate")),"Couldn't get DS proc addr");
	
	LPDIRECTSOUND pDS;
	check((hresult = iDirectSoundCreate(NULL, &pDS, NULL) != DS_OK),"DirectSoundCreate failed");
	IDirectSoundVtbl* dsvt=pDS->lpVtbl;

	dscaps.dwSize = sizeof(dscaps);
	check(DS_OK != dsvt->GetCaps(pDS, &dscaps), "Couldn't get DS caps");
	check(dscaps.dwFlags & DSCAPS_EMULDRIVER,"No DS driver installed");

	check(DS_OK != dsvt->SetCooperativeLevel(pDS, GetConsoleWindow(), DSSCL_EXCLUSIVE),"Set coop level failed");

	DSBUFFERDESC	dsbuf;
	memset (&dsbuf, 0, sizeof(dsbuf));
	dsbuf.dwSize = sizeof(DSBUFFERDESC);
	dsbuf.dwFlags = DSBCAPS_PRIMARYBUFFER;
	dsbuf.dwBufferBytes = 0;
	dsbuf.lpwfxFormat = NULL;

	DSBCAPS dsbcaps;
	memset(&dsbcaps, 0, sizeof(dsbcaps));
	dsbcaps.dwSize = sizeof(dsbcaps);

	check(DS_OK != dsvt->CreateSoundBuffer(pDS, &dsbuf, &pDSPBuf, NULL),"CreateSoundBuffer failed");
	check(DS_OK != dsvt->SetFormat(pDSPBuf, &wf),"SetFormat failed");

	check(DS_OK != dsvt->SetCooperativeLevel (pDS, GetConsoleWindow(),DSSCL_WRITEPRIMARY),"SetCoop failed");
	dsvt->Play(pDSBuf, 0, 0, DSBPLAY_LOOPING);

// initialize the buffer
	reps = 0;

	while ((hresult = dsvt->Lock(pDSBuf, 0, dsbcaps.dwBufferBytes, &lpData, &dwSize, NULL, NULL, 0)) != DS_OK)
	{
		if (hresult != DSERR_BUFFERLOST)
		{
			Con_SafePrintf ("SNDDMA_InitDirect: DS::Lock Sound Buffer Failed\n");
			FreeSound ();
			return SIS_FAILURE;
		}

		if (++reps > 10000)
		{
			Con_SafePrintf ("SNDDMA_InitDirect: DS: couldn't restore buffer\n");
			FreeSound ();
			return SIS_FAILURE;
		}

	}

	memset(lpData, 0, dwSize);
	lpData[4] = lpData[5] = 0x7f;	// force a pop for debugging

	dsvt->Unlock(pDSBuf, lpData, dwSize, NULL, 0);
	dsvt->Stop(pDSBuf);
	dsvt->GetCurrentPosition(pDSBuf, &mmstarttime.u.sample, &dwWrite);
	dsvt->Play(pDSBuf, 0, 0, DSBPLAY_LOOPING);
	printf("hello world\n");
	return(0);
}


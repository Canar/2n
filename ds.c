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

	check(DS_OK != dsvt->SetCooperativeLevel(pDS, GetConsoleWindow(), DSSCL_BACKGROUND | DSSCL_EXCLUSIVE),"Set coop level failed");

	DSBUFFERDESC	dsbuf;
	memset (&dsbuf, 0, sizeof(dsbuf));
	dsbuf.dwSize = sizeof(DSBUFFERDESC);
	dsbuf.dwFlags = DSBCAPS_PRIMARYBUFFER;
	dsbuf.dwBufferBytes = 0;
	dsbuf.lpwfxFormat = NULL;

	DSBCAPS dsbcaps;
	memset(&dsbcaps, 0, sizeof(dsbcaps));
	dsbcaps.dwSize = sizeof(dsbcaps);
	primary_format_set = false;

	if (DS_OK == pDS->lpVtbl->CreateSoundBuffer(pDS, &dsbuf, &pDSPBuf, NULL)){
		pformat = format;

		if (DS_OK != pDSPBuf->lpVtbl->SetFormat (pDSPBuf, &pformat))
		{
			if (snd_firsttime)
				Con_SafePrintf ("Set primary sound buffer format: no\n");
		}
		else
		{
			if (snd_firsttime)
				Con_SafePrintf ("Set primary sound buffer format: yes\n");

			primary_format_set = true;
		}
	}

	if (!primary_format_set || !COM_CheckParm ("-primarysound"))
	{
	// create the secondary buffer we'll actually work with
		memset (&dsbuf, 0, sizeof(dsbuf));
		dsbuf.dwSize = sizeof(DSBUFFERDESC);
		dsbuf.dwFlags = DSBCAPS_CTRLFREQUENCY | DSBCAPS_LOCSOFTWARE;
		dsbuf.dwBufferBytes = SECONDARY_BUFFER_SIZE;
		dsbuf.lpwfxFormat = &format;

		memset(&dsbcaps, 0, sizeof(dsbcaps));
		dsbcaps.dwSize = sizeof(dsbcaps);

		if (DS_OK != pDS->lpVtbl->CreateSoundBuffer(pDS, &dsbuf, &pDSBuf, NULL))
		{
			Con_SafePrintf ("DS:CreateSoundBuffer Failed");
			FreeSound ();
			return SIS_FAILURE;
		}

		shm->channels = format.nChannels;
		shm->samplebits = format.wBitsPerSample;
		shm->speed = format.nSamplesPerSec;

		if (DS_OK != pDSBuf->lpVtbl->GetCaps (pDSBuf, &dsbcaps))
		{
			Con_SafePrintf ("DS:GetCaps failed\n");
			FreeSound ();
			return SIS_FAILURE;
		}

		if (snd_firsttime)
			Con_SafePrintf ("Using secondary sound buffer\n");
	}
	else
	{
		if (DS_OK != pDS->lpVtbl->SetCooperativeLevel (pDS, mainwindow, DSSCL_WRITEPRIMARY))
		{
			Con_SafePrintf ("Set coop level failed\n");
			FreeSound ();
			return SIS_FAILURE;
		}

		if (DS_OK != pDSPBuf->lpVtbl->GetCaps (pDSPBuf, &dsbcaps))
		{
			Con_Printf ("DS:GetCaps failed\n");
			return SIS_FAILURE;
		}

		pDSBuf = pDSPBuf;
		Con_SafePrintf ("Using primary sound buffer\n");
	}

	// Make sure mixer is active
	pDSBuf->lpVtbl->Play(pDSBuf, 0, 0, DSBPLAY_LOOPING);

	if (snd_firsttime)
		Con_SafePrintf("   %d channel(s)\n"
		               "   %d bits/sample\n"
					   "   %d bytes/sec\n",
					   shm->channels, shm->samplebits, shm->speed);
	
	gSndBufSize = dsbcaps.dwBufferBytes;

// initialize the buffer
	reps = 0;

	while ((hresult = pDSBuf->lpVtbl->Lock(pDSBuf, 0, gSndBufSize, &lpData, &dwSize, NULL, NULL, 0)) != DS_OK)
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
//		lpData[4] = lpData[5] = 0x7f;	// force a pop for debugging

	pDSBuf->lpVtbl->Unlock(pDSBuf, lpData, dwSize, NULL, 0);

	we don't want anyone to access the buffer directly w/o locking it first. 
	lpData = NULL; 

	pDSBuf->lpVtbl->Stop(pDSBuf);
	pDSBuf->lpVtbl->GetCurrentPosition(pDSBuf, &mmstarttime.u.sample, &dwWrite);
	pDSBuf->lpVtbl->Play(pDSBuf, 0, 0, DSBPLAY_LOOPING);

	shm->soundalive = true;
	shm->splitbuffer = false;
	shm->samples = gSndBufSize/(shm->samplebits/8);
	shm->samplepos = 0;
	shm->submission_chunk = 1;
	shm->buffer = (unsigned char *) lpData;
	sample16 = (shm->samplebits/8) - 1;

	dsound_init = true;
*/
	printf("hello world\n");
	return(0);
}


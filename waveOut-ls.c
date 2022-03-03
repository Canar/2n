#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>

void printwf(int fmt,DWORD flag,char* desc){
	if(fmt||flag)
		printf("\t\t%s\n",desc);
}

int main(int argc,char** argv){
	int devs=waveOutGetNumDevs();
	printf("%d devices detected.\n",devs);
	WAVEOUTCAPS pwoc;
	for(int i=0;i<devs;i++){
		waveOutGetDevCaps(i,&pwoc,sizeof(WAVEOUTCAPS));
		printf("ID: %d  wMid: %d  wPid: %d  ver: %d  Name: '%s'  fmt: %d  ch: %d  res: %d  supp: %d\n",i,pwoc.wMid,pwoc.wPid,pwoc.vDriverVersion,pwoc.szPname,pwoc.dwFormats,pwoc.wChannels,pwoc.wReserved1,pwoc.dwSupport);
		printf("\tFormats:\n");
		printwf(pwoc.dwFormats,WAVE_FORMAT_1M08,"11.025 kHz, mono, 8-bit");
		printwf(pwoc.dwFormats,WAVE_FORMAT_1M16,"11.025 kHz, mono, 16-bit");
		printwf(pwoc.dwFormats,WAVE_FORMAT_1S08,"11.025 kHz, stereo, 8-bit");
		printwf(pwoc.dwFormats,WAVE_FORMAT_1S16,"11.025 kHz, stereo, 16-bit");
		printwf(pwoc.dwFormats,WAVE_FORMAT_2M08,"22.05 kHz, mono, 8-bit");
		printwf(pwoc.dwFormats,WAVE_FORMAT_2M16,"22.05 kHz, mono, 16-bit");
		printwf(pwoc.dwFormats,WAVE_FORMAT_2S08,"22.05 kHz, stereo, 8-bit");
		printwf(pwoc.dwFormats,WAVE_FORMAT_2S16,"22.05 kHz, stereo, 16-bit");
		printwf(pwoc.dwFormats,WAVE_FORMAT_4M08,"44.1 kHz, mono, 8-bit");
		printwf(pwoc.dwFormats,WAVE_FORMAT_4M16,"44.1 kHz, mono, 16-bit");
		printwf(pwoc.dwFormats,WAVE_FORMAT_4S08,"44.1 kHz, stereo, 8-bit");
		printwf(pwoc.dwFormats,WAVE_FORMAT_4S16,"44.1 kHz, stereo, 16-bit");
		printwf(pwoc.dwFormats,WAVE_FORMAT_96M08,"96 kHz, mono, 8-bit");
		printwf(pwoc.dwFormats,WAVE_FORMAT_96M16,"96 kHz, mono, 16-bit");
		printwf(pwoc.dwFormats,WAVE_FORMAT_96S08,"96 kHz, stereo, 8-bit");
		printwf(pwoc.dwFormats,WAVE_FORMAT_96S16,"96 kHz, stereo, 16-bit");
		printf("\tCapabilities:\n");
		printwf(pwoc.dwSupport,WAVECAPS_LRVOLUME,"Supports separate left and right volume control.");
		printwf(pwoc.dwSupport,WAVECAPS_PITCH,"Supports pitch control.");
		printwf(pwoc.dwSupport,WAVECAPS_PLAYBACKRATE,"Supports playback rate control.");
		printwf(pwoc.dwSupport,WAVECAPS_SYNC,"The driver is synchronous and will block while playing a buffer.");
		printwf(pwoc.dwSupport,WAVECAPS_VOLUME,"Supports volume control.");
		printwf(pwoc.dwSupport,WAVECAPS_SAMPLEACCURATE,"Returns sample-accurate position information.");
	}
	return 0;
}

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>

int main(int argc,char** argv){
	int devs=waveOutGetNumDevs();
	WAVEOUTCAPS pwoc;

	printf("%d devices detected.\n",devs);
	for(int i=0;i<devs;i++){
		waveOutGetDevCaps(i,&pwoc,sizeof(WAVEOUTCAPS));
		printf("ID: %d  wMid: %d  wPid: %d  ver: %d  Name: '%s'  fmt: %d  ch: %d  res: %d  supp: %d\n",i,pwoc.wMid,pwoc.wPid,pwoc.vDriverVersion,pwoc.szPname,pwoc.dwFormats,pwoc.wChannels,pwoc.wReserved1,pwoc.dwSupport);
	}
	return 0;
}

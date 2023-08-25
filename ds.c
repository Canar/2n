#include <stdio.h>
#include <windows.h>

#define iDirectSoundCreate(a,b,c)	pDirectSoundCreate(a,b,c)

HRESULT (WINAPI *pDirectSoundCreate)(GUID FAR *lpGUID, LPDIRECTSOUND FAR *lplpDS, IUnknown FAR *pUnkOuter);

int main(){
	printf("hello world\n");
	return(0);
}

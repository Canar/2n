//Code below doesn't work, links might
//
// https://hero.handmade.network/forums/code-discussion/t/920-directsound_and_c
// https://github.com/id-Software/Quake/blob/master/WinQuake/snd_win.c#L340
// DirectSound sucks, requires COM/OOP interfacing
// https://www.codeproject.com/Articles/13601/COM-in-plain-C


#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <windows.h>
#include <dsound.h>

static const char* getSndErr(HRESULT hr){
	char *p;
		switch(hr){
		case DSERR_ALLOCATED :
		p=" DSERR_ALLOCATED";
		break;
		case DSERR_CONTROLUNAVAIL :
		p=" DSERR_CONTROLUNAVAIL";
		break;
		case DSERR_BADFORMAT :
		p=" DSERR_BADFORMAT";
		break;
		case DSERR_INVALIDPARAM:
		p=" DSERR_INVALIDPARAM";
		break;
		case DSERR_NOAGGREGATION :
		p=" DSERR_NOAGGREGATION";
		break;
		case DSERR_OUTOFMEMORY :
		p=" DSERR_OUTOFMEMORY";
		break;
		case DSERR_UNINITIALIZED :
		p=" DSERR_UNINITIALIZED";
		break;
		case DSERR_UNSUPPORTED :
		p=" DSERR_UNSUPPORTED";
		break;
		default :
		p="Unknown";
		break;
		}
	return p;
}

static LPGUID snd_guid=NULL;
static BOOL OnDSoundDev( LPGUID lpGuid, LPCSTR lpDesp, LPCSTR lpModule, LPVOID lpContext ){
static int i;
//fprintf(stderr, "Get DSound: %s\t%s\n", lpDesp, lpModule);
fprintf(stderr, "Get DSound device\n");
if( i ) snd_guid= lpGuid;
return i++ == 0;
}

int main(int argc ,char** argv){
if( argc!= 2) return 0;
LPVOID lpPtr1=NULL;//指针1;
LPVOID lpPtr2=NULL;//指针2;
HRESULT hResult;
DWORD dwLen1,dwLen2;
char* m_pMemory=NULL;//内存指针;
LPWAVEFORMATEX m_pFormat=NULL;//LPWAVEFORMATEX变量;
LPVOID m_pData;//指向语音数据块的指针;
DWORD m_dwSize;//WAVE文件中语音数据块的长度;
int file;//Cfile对象;
DWORD dwSize;//存放WAV文件长度;

//~ if(FAILED( CoInitialize(NULL) )) return 0;

//打开sound.wav文件;
file= _open (argv[1],_O_RDONLY | _O_BINARY);
if(file<0) return 0 ;

//get size
dwSize= _lseek (file ,0,SEEK_END);
//为m_pMemory分配内存,类型为LPVOID,用来存放WAVE文件中的数据;
m_pMemory =(char*) GlobalAlloc (GMEM_FIXED, dwSize);

dwLen1=0;
_lseek (file, 0,SEEK_SET);//定位到打开的WAVE文件头;
while( !_eof(file)){
dwLen2=_read(file, &m_pMemory[dwLen1] , dwSize- dwLen1) ;
if( dwLen2<0) return 0;
dwLen1+= dwLen2;
} //while
fprintf(stderr, "Read %d bytes, acturally has %d bytes\n", dwLen1, dwSize);
if( dwLen1!=dwSize) perror("");
_close(file);
LPDWORD pdw=NULL,pdwEnd=NULL;
DWORD dwRiff,dwType, dwLength;
//if (m_pFormat) //格式块指针
m_pFormat = NULL;
//if (m_pData) //数据块指针,类型:LPBYTE
m_pData = NULL;
//if (m_dwSize) //数据长度,类型:DWORD
m_dwSize = 0;
pdw = (DWORD *) m_pMemory;
dwRiff = *pdw++;
dwLength = *pdw++;
dwType = *pdw++;
if (dwRiff != mmioFOURCC ('R', 'I', 'F', 'F'))
return 0 ;//判断文件头是否为"RIFF"字符;
if (dwType != mmioFOURCC ('W', 'A', 'V', 'E'))
return 0 ;//判断文件格式是否为"WAVE";
//寻找格式块,数据块位置及数据长度
pdwEnd = (DWORD *)((BYTE *) m_pMemory+dwLength -4);
bool m_bend=false;

//~ DirectSoundEnumerate( (LPDSENUMCALLBACK )OnDSoundDev, NULL);
//~ do Sleep(100); while( snd_guid ==NULL);
while ((pdw < pdwEnd)&&(!m_bend))
//pdw文件没有指到文件末尾并且没有获取到声音数据时继续;
{
dwType = *pdw++;
dwLength = *pdw++;
switch (dwType)
{
case mmioFOURCC('f', 'm', 't', ' ')://如果为"fmt"标志;
if (!m_pFormat)//获取LPWAVEFORMATEX结构数据;
{
if (dwLength < sizeof (WAVEFORMAT)) return 0 ;
m_pFormat = (LPWAVEFORMATEX) pdw;
}
break;
case mmioFOURCC('d', 'a', 't', 'a')://如果为"data"标志;
if (!m_pData || !m_dwSize)
{
m_pData = (LPBYTE) pdw;//得到指向声音数据块的指针;
m_dwSize = dwLength;//获取声音数据块的长度;
if (m_pFormat)m_bend=TRUE;
}
break;
}
pdw = (DWORD *)((BYTE *) pdw + ((dwLength + 1)&~1));//修改pdw指针,继续循环;
}
DSBUFFERDESC BufferDesc;//定义DSUBUFFERDESC结构对象;
memset (&BufferDesc, 0, sizeof (BufferDesc));
BufferDesc.lpwfxFormat = (LPWAVEFORMATEX)m_pFormat;
BufferDesc.dwSize = sizeof (DSBUFFERDESC);
BufferDesc.dwBufferBytes = m_dwSize;
BufferDesc.dwFlags = 0;
HRESULT hRes;
LPDIRECTSOUND m_lpDirectSound;

hRes = ::DirectSoundCreate(0, &m_lpDirectSound, 0);//创建DirectSound对象;
if( hRes != DS_OK ) return 0;

//~ DirectSoundEnumerate((LPDSENUMCALLBACK)OnDSoundDev, 0);

if( DS_OK !=m_lpDirectSound->SetCooperativeLevel(
GetDesktopWindow(),
DSSCL_EXCLUSIVE
//DSSCL_NORMAL
)){
fprintf(stderr," SetCooperativeLevel err: %s\n", getSndErr(hRes));
return 0;
}
//设置声音设备优先级别为"NORMAL";
//创建声音数据缓冲;
LPDIRECTSOUNDBUFFER m_pDSoundBuffer;
if ( (hRes=m_lpDirectSound->CreateSoundBuffer (&BufferDesc, &m_pDSoundBuffer, 0)) != DS_OK) {
fprintf(stderr," CreateSoundBuffer err: %s\n", getSndErr(hRes));
return 0;
}
//载入声音数据,这里使用两个指针lpPtr1,lpPtr2来指向DirectSoundBuffer缓冲区的数据,这是为了处理大型WAVE文件而设计的。dwLen1,dwLen2分别对应这两个指针所指向的缓冲区的长度。
hResult=m_pDSoundBuffer->Lock(0,m_dwSize,&lpPtr1,&dwLen1,&lpPtr2,&dwLen2,0);
if (hResult == DS_OK){
memcpy (lpPtr1, m_pData, dwLen1);
if(dwLen2>0){
BYTE *m_pData1=(BYTE*)m_pData+dwLen1;
m_pData=(void *)m_pData1;
memcpy(lpPtr2,m_pData, dwLen2);
}
if( DS_OK!=m_pDSoundBuffer->Unlock (lpPtr1, dwLen1, lpPtr2, dwLen2))
fprintf(stderr," unlock err: %s\n", getSndErr(hRes));
else if( DS_OK!= m_pDSoundBuffer->Play (0, 0, 0)) //播放WAVE声音数据;
fprintf(stderr," play err: %s\n", getSndErr(hRes));
} else fprintf(stderr," Lock err: %s\n", getSndErr(hRes));

out:
if(m_pMemory) GlobalFree(m_pMemory);
//~ CoUninitialize();
}

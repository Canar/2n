#include "config.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

enum ERROR {
	NONE,
	PIPE,
	DECODE_FORK,
	OUTPUT_FORK,
	REFRESH_FORK,
	INPUT_FORK,
	GETTIME,
	PLAYLIST_STAT,
	PLAYLIST_WRITE_OPEN,
	PLAYLIST_WRITE,
	PLAYLIST_WRITE_CLOSE,
	STATE_WRITE_OPEN,
	STATE_WRITE,
	STATE_WRITE_CLOSE,
	PLAYLIST_READ_STAT,
	PLAYLIST_READ_OPEN,
	PLAYLIST_READ,
	PLAYLIST_READ_CLOSE,
	STATE_READ_OPEN,
	STATE_READ,
	STATE_READ_CLOSE,
	STATE_READ_SPRINTF,
	TERMIOS_READ,
	TERMIOS_WRITE
};

const char* errmsg[]={
	"not an error",
	"pipe()",
	"fork(decode)",
	"fork(output)",
	"fork(input)",
	"fork(refresh)",
	"clock_gettime()",
	"stat(playlist)",
	"open(playlist,write)",
	"write(playlist)",
	"close(playlist,write)",
	"open(state,write)",
	"write(state)",
	"close(state,write)",
	"open(playlist,read)",
	"read(playlist)",
	"close(playlist_read)"
	"open(state,read)",
	"read(state)",
	"close(state_read)",
	"sprintf(state_read)",
	"tcgetattr(STDIN,termios_state)"
	"tcsetattr(STDIN,termios_state)"
};

typedef struct state_struct {
	int pos;
	time_t off;
} statest;

/*
dr_flac - small flac decoder - https://github.com/mackron/dr_libs
pipe / exec dox - https://www.cs.uleth.ca/~holzmann/C/system/pipeforkexec.html

Windows file change tracking
	https://qualapps.blogspot.com/2010/05/understanding-readdirectorychangesw.html
	https://qualapps.blogspot.com/2010/05/understanding-readdirectorychangesw_19.html
	https://learn.microsoft.com/en-us/windows/win32/fileio/obtaining-directory-change-notifications?redirectedfrom=MSDN
 */

static struct termios *termios_state=NULL; 

#define CK(x) vck(x,__FILE__,__LINE__,NULL)
void vck(int,char*,int,char*);

void halt(int x){
	if(termios_state!=NULL)
		CK(tcsetattr(STDIN,TCSANOW,termios_state));
	exit(x);
}

void vck(int retval,char* file,int line,char* msg){
	if(retval>=0)
		return;
	fprintf(stderr, "Error in %s, line %d. Value: %d. %s\n",file,line,retval,msg);
	halt(1);
}

void validate(int retval,enum ERROR kind){
	if(retval>=0)
		return;
	printf("ERROR %d: %s failed.\n",kind+1,errmsg[kind]);
	exit((int)kind+1);
}

int prefix_len(int plen,int count,char* ary[]){
	if(count<=1)
		return(plen);
	for(int j=0;j<plen;j++)
		for(int i=1;i<count;i++)
			if(!(ary[0][j]==ary[i][j]))
				return(j);
	return(count);
}

//evaluates full path of files in inv array, allocates to outv
void validate_playlist(int count,char** inv,char** outv,char*** plv){
//void validate_playlist(int count,char* inv[],char** outv[]){
	char* rp[count];
	int rpc[count];//chars in realpath path of index
	size_t pc=0;

	//get realpath output
	for(int i=0;i<count;i++){
		rp[i]=realpath(inv[i],NULL);
		rpc[i]=strlen(rp[i]);
		pc+=rpc[i]+1;
	}

	//alloc a contiguous block
	(*outv)=malloc(pc);
	(*plv)=malloc(count*sizeof(char*));

	//init loop
	for(int i=0,pc=0;i<count;i++){
		memcpy(&(*outv)[pc],rp[i],rpc[i]);
		(*outv)[pc+rpc[i]]=0;
		(*plv)[i]=&(*outv)[pc];
		free(rp[i]);
		pc+=rpc[i]+1;
	}
}

//evaluates config dirs, ensures they exist
void dir_eval(int plc, char** pl, char** playlist_fn, char** state_fn, char** prefix, int* plen){
	//check HOME dir
	char* home_dir=getenv("HOME");
	if(NULL==home_dir){
		printf("ERROR: $HOME unset. Environment appears faulty.\n");
		exit(1);
	}

	//make config dir CFGDIR
	int cfg_dir_len=strlen(home_dir)+strlen(CFGDIR)+1;
	char* cfg_dir=malloc(cfg_dir_len);
	strcpy(cfg_dir,home_dir);
	strcat(cfg_dir,CFGDIR);
	int ret=mkdir(cfg_dir,0775);
	if(-1==ret){
		if(EEXIST!=errno){
			printf("ERROR: %d in mkdir()\n",errno);
			exit(1);
		}
	}

	//define playlist and state filenames
	*playlist_fn = malloc(cfg_dir_len+strlen(PLAYLISTFN));
	strcpy(*playlist_fn,cfg_dir);
	strcat(*playlist_fn,PLAYLISTFN);

	*state_fn = malloc(cfg_dir_len+strlen(STATEFN));
	strcpy(*state_fn,cfg_dir);
	strcat(*state_fn,STATEFN);

	//calc prefix
	*plen=strlen(pl[0]);
	*plen=prefix_len(*plen,plc,pl);
	*prefix=malloc(*plen+1);
	memcpy(*prefix,pl[0],*plen);
	prefix[*plen]=0;
}

void print_usage(){
	/*
	 * --loop -l - loop final track in playlist
	 * --playlist -p [playlist_name] -  use non-default playlist
	 * --relative -r - don't use realpath() to ensure absolute path [useless?]
	 * --quiet -q - don't perform playback
	 * --service -s - service mode
	 * 	changes some semantics, stops giving every-second updates
	 * 	autodetected if stdin is closed?
	 *
	 * --loglevel -v [value] - set loglevel
	 *  	56 - trace
	 *  	48 - debug
	 *  	40 - verbose - metadata?
	 *		32 - info - one line per track
	 *		24 - warning - one line per invocation
	 *		16 - error
	 *		8 - fatal
	 *		0 - panic
	 *		-8 - silent
	 */ 
}

enum PIDS {P_RET,P_REFRESH,P_INPUT,P_DECODE,P_OUTPUT,P_ENUM_COUNT};
int pids[(int)P_ENUM_COUNT]={0};
int pipefd[2];

statest state={.off=0,.pos=-1};

int playback(char** pl,int plc,int plen){
	struct timespec start,now;

	#ifndef DEBUG
	close(2); //hide stderr
	#endif

	//setup pids

	CK( pipe(pipefd) );
	CK( pids[P_OUTPUT]=vfork() );
	if(0==pids[P_OUTPUT]){
		close(pipefd[1]);
		dup2(pipefd[0],0);
		close(pipefd[0]);
		execlp("pacat","-p","-v","--channels="CHAN,"--format="FRMT,"--rate="RATE,"--raw","--client-name="PKGVER,"--stream-name="STRM,(char*)0);
		//execlp("pw-cat","-v","-p","--channels="CHAN,"--format=f32","--rate="RATE,"-",(char*)0);
		//execlp("ffmpeg","-hide_banner","-ac","2","-ar","44100","-f","f32le","-i","-","-f","pulse","default",(char*)0);
	}

	char ss_buf[22]={'0',0,'0',0}; // contains start location
	int ret;
	char key;

	// caching is a hack to tell ffmpeg to cache more of the input
	char cache_fn[4096+1+6]="cache:"; /* max_path + null + "cache:" */

	//TODO: refactor using poll()
	while(1){
		if(pids[P_RET]==pids[P_OUTPUT]){
			printf("ERROR: Output process died unexpectedly. Terminating.\n");
			halt(2);

		}else if(pids[P_RET]==pids[P_DECODE]){
			if(plc==++state.pos)
				return(0);
			if(state.pos<0)
				state.pos=0;
			if(plc>1)
				printf("\e[2K\r%s\n",&pl[state.pos][plen]);
			clock_gettime(CLOCK_MONOTONIC,&start);
			char* ss=ss_buf;
			if(state.off>0){
				start.tv_sec-=state.off;
				ss=&ss_buf[2];
				CK( snprintf(ss,20,"%d",state.off) );
				state.off=0;
			}
			if(pids[P_REFRESH]>0)
				kill(pids[P_REFRESH],SIGINT);
			CK( pids[P_DECODE]=vfork() );
			if(0==pids[P_DECODE]){
				close(pipefd[0]);
				dup2(pipefd[1],1);
				close(pipefd[1]);
				strcpy(&cache_fn[6],pl[state.pos]);
				execlp("ffmpeg","-hide_banner","-ss",ss,"-i",cache_fn,"-loglevel","-8","-af","volume=0.75","-ac","2","-ar","44100","-f","f32le","-",
					#ifdef DEBUG
					"-report",
					#endif
					(char*)0
				);
				cache_fn[6]='\0';
			}
		}else if(pids[P_RET]==pids[P_INPUT]){
				switch(key=WEXITSTATUS(ret)){
				case 'Q':
					return(0);
				case 'P':
					state.pos-=2; //fallthru
				case 'N':
					kill(pids[P_DECODE],SIGINT);
				}
				CK( pids[P_INPUT]=fork() );
				if(0==pids[P_INPUT]){
					read(0,&key,1);
					exit((int)toupper(key));
				}
		}else if(pids[P_RET]==pids[P_REFRESH]){
				clock_gettime(CLOCK_MONOTONIC,&now);
				sprintf(&cache_fn[6],
					"\e[2K\r%d/%d %d:%02d > ",state.pos+1,plc,
					(now.tv_sec-start.tv_sec)/60,
					(now.tv_sec-start.tv_sec)%60
				);
				printf("%s",&cache_fn[6]);
				cache_fn[6]='\0';
				CK( pids[P_REFRESH]=fork() );
				if(0==pids[P_REFRESH]){
					usleep((start.tv_nsec+1E9-now.tv_nsec)/1E3);
					return(0);
				}
		}
		pids[P_RET] = pids[P_RET]<0 ? pids[P_RET]+1 : wait(&ret);
	}
}


int main(int argc, char *argv[]){

	int output_pid;
	int decode_pid=-3; 
	int input_pid=-2;
	int refresh_pid=-1;
	int ret_pid=decode_pid;

	int pipefd[2];
	char key;

	int pos=-1; /* position in playlist */
	char** pl;
	char* plmap;
	int plc=argc-1;


	//use realpath to get canonical path names of playlist
	int fd;
	int pllen=0;
	char*playlist_fn,*state_fn,*prefix;

	//TODO: reimplement playlist load
	validate_playlist(argc-1,&argv[1],&plmap,&pl); //only valid when executed with args!

	int plen;
	dir_eval(plc,pl,&playlist_fn,&state_fn,&prefix,&plen);
	printf("%s\n",prefix);

	/*
-       struct stat st;
-       if( ((0>stat(playlist_fn,&st)) && (2>argc)) || (1>st.st_size)){
-               printf("ERROR: Playlist is empty. Run " PKG " with a list of files as argument to generate it.\n");
-               goto quit;
-       }
*/

	// read state
	struct stat st;
	if(!(0>stat(state_fn,&st) || 1<argc)){
		validate(fd=open(state_fn,O_RDONLY),STATE_READ_OPEN);
		validate(read(fd,&state,sizeof(statest))-sizeof(statest),STATE_READ);
		validate(close(fd),STATE_READ_CLOSE);
		pos=state.pos-1;
	}

	termios_state=malloc(sizeof(struct termios));
	CK( tcgetattr(STDIN,termios_state) ); //save terminal state
	playback(pl,plc,plen);

	//playback begins
	//open output process
	//pipefd, pids, plc, state

	/*
	validate(pipe(pipefd),PIPE);
	validate(output_pid=vfork(),OUTPUT_FORK);
	if(0==output_pid){
		close(pipefd[1]);
		dup2(pipefd[0],0);
		close(pipefd[0]);
		//execlp("pw-cat","-v","-p","--channels="CHAN,"--format=f32","--rate="RATE,"-",(char*)0);
		execlp("pacat","-p","-v","--channels="CHAN,"--format="FRMT,"--rate="RATE,"--raw","--client-name="PKGVER,"--stream-name="STRM,(char*)0);
		//execlp("ffmpeg","-hide_banner","-ac","2","-ar","44100","-f","f32le","-i","-","-f","pulse","default",(char*)0);
	}

	char ss_buf[22]={'0',0,'0',0}; // contains start location
	int ret;
	while(1){
		
		if(ret_pid==output_pid){
			printf("ERROR: Output process died unexpectedly. Terminating.\n");
			return(1);
		}

		else if(ret_pid==decode_pid){
			if(plc==++pos)
				goto quit;
			if(pos<0)
				pos=0;
			if(plc>1)
				printf("\e[2K\r%s\n",&pl[pos][plen]);
			clock_gettime(CLOCK_MONOTONIC,&start);
			char* ss=ss_buf;
			if(state.off>0){
				start.tv_sec-=state.off;
				ss=&ss_buf[2];
				validate(snprintf(ss,20,"%d",state.off),STATE_READ_SPRINTF);
				state.off=0;
			}
			if(refresh_pid>0)
				kill(refresh_pid,SIGINT);
			validate(decode_pid=vfork(),DECODE_FORK);
			if(0==decode_pid){
				close(pipefd[0]);
				dup2(pipefd[1],1);
				close(pipefd[1]);
				strcpy(&cache_fn[6],pl[pos]);
				execlp("ffmpeg","-hide_banner","-ss",ss,"-i",cache_fn,"-loglevel","-8","-af","volume=0.75","-ac","2","-ar","44100","-f","f32le","-",
					#ifdef DEBUG
					"-report",
					#endif
					(char*)0
				);
				cache_fn[6]='\0';
			}
		}

		else if(ret_pid==input_pid){
			switch(key=WEXITSTATUS(ret)){
			case 'Q':
				goto quit;
			case 'P':
				pos-=2; //fallthru
			case 'N':
				kill(decode_pid,SIGINT);
			}
			validate(input_pid=fork(),INPUT_FORK);
			if(0==input_pid){
				read(0,&key,1);
				return((int)toupper(key));
			}
		}

		else if(ret_pid==refresh_pid){
			clock_gettime(CLOCK_MONOTONIC,&now);
			sprintf(&cache_fn[6],
				"\e[2K\r%d/%d %d:%02d > ",pos+1,plc,
				(now.tv_sec-start.tv_sec)/60,
				(now.tv_sec-start.tv_sec)%60
			);
			printf("%s",&cache_fn[6]);
			cache_fn[6]='\0';
			validate(refresh_pid=fork(),REFRESH_FORK);
			if(0==refresh_pid){
				usleep((start.tv_nsec+1E9-now.tv_nsec)/1E3);
				return(0);
			}
		}

		ret_pid= ret_pid<0 ? ret_pid+1 : wait(&ret);
	}

	quit:
	state.off=now.tv_sec-start.tv_sec;
	state.pos=pos;
	validate(fd=open(state_fn,O_WRONLY|O_CREAT|O_TRUNC,0775),STATE_WRITE_OPEN);
	validate(write(fd,&state,sizeof(statest)),STATE_WRITE);
	validate(close(fd),STATE_WRITE_CLOSE);

	if(refresh_pid>0) 
		kill(refresh_pid,SIGINT);
	if(decode_pid>0) 
		kill(decode_pid,SIGINT);
	if(output_pid>0) 
		kill(output_pid,SIGINT);
	if(input_pid>0) 
		kill(input_pid,SIGINT);
	*/
	for(int i=1;i<P_ENUM_COUNT;i++)
		kill(pids[i],SIGINT);
	printf("\e[2K\r    ]  " PKGVER "  [    \n");
	return(0);
}

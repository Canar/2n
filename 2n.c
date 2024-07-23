#include "config.h"

//#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
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

#define KBUSAGE "Keyboard Controls\n\
  h  print help\n\
  n  next track\n\
  p  previous track\n\
  r  restart current track\n\
  q  quit\n\
  Q  quit after current track (TODO)\n"

#define USAGE PKG": A minimal gapless music player.\n\
\n\
Usage: "PKG" [OPTION...] [FILE...]\n\
\n\
Options\n\
  -s  --shuffle  randomizes playback order of files\n\
  -h  --help     displays this text\n\
\n\
"PKG" takes a playlist of files as parameters.\n\
Playlist is written to \"$HOME"CFGDIR"\".\n\
Playlist session is saved on quit.\n\
Resume session by running "PKG" without file arguments.\n\
\n\
"KBUSAGE" \
\n\
PCM transport format: "STRM"\n\
\n"

#define OUTEXEC "pacat","-p","-v","--channels="CHAN,"--format="FRMT,"--rate="RATE,"--raw","--client-name="PKGVER,"--stream-name="STRM
//#define OUTEXEC "pw-cat","-v","-p","--channels="CHAN,"--format=f32","--rate="RATE,"-"
//#define OUTEXEC "ffmpeg","-hide_banner","-ac","2","-ar","44100","-f","f32le","-i","-","-f","pulse","default"

typedef struct state_struct {
	int pos;
	time_t off;
} statest;

enum PIDS {P_RET,P_REFRESH,P_INPUT,P_DECODE,P_OUTPUT,P_ENUM_COUNT};
int pids[(int)P_ENUM_COUNT]={-3,-1,-2,-3,-4};

statest state={.off=0,.pos=-1};
struct termios *termios_state=NULL; 
struct timespec start;
int pipefd[2];
static int rseed=0;

#define CK(x) vck(x,__FILE__,__LINE__,NULL)
void vck(int,char*,int,char*);

void halt(int x){
	for(int i=1;i<P_ENUM_COUNT;i++)
		if(pids[i]>0)
			kill(pids[i],SIGINT);

	if(termios_state!=NULL)
		CK( tcsetattr(STDIN,TCSANOW,termios_state) );

	if(0==x){
		printf("\e[2K\r    ]  " PKGVER "  [    \n");
	}
	exit(x);
}
void vck(int retval,char* file,int line,char* msg){
	if(retval>=0)
		return;
	fprintf(stderr, "Error in %s, line %d. Value: %d. %s\n",file,line,retval,msg);
	halt(1);
}
int prefix_len(int prefixc,int count,char* ary[]){
	if(count<=1)
		return(0);
	for(int j=0;j<prefixc;j++)
		for(int i=1;i<count;i++)
			if(!(ary[0][j]==ary[i][j]))
				return(j);
	return(count);
}
size_t validate_playlist(int count,char** inv,char** outv,char*** plv){ //evaluates full path of files in inv array, allocates to outv
	char* rp[count];
	int rpc[count];//chars in realpath path of index
	size_t pc=0;

	//get realpath output
	for(int i=0;i<count;i++){
		rp[i]=realpath(inv[i],NULL);
		if(rp[i]){
			rpc[i]=strlen(rp[i]);
			pc+=rpc[i]+1;
		}else{
			return(0);
		}
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
	return(pc);
}
void dir_eval(char** playlist_fn, char** state_fn){ //evaluates config dirs, ensures they exist
	//check HOME dir
	char* home_dir=getenv("HOME");
	if(NULL==home_dir){
		printf("ERROR: $HOME unset. Environment appears faulty.\n");
		halt(1);
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
			halt(1);
		}
	}

	//define playlist and state filenames
	*playlist_fn = malloc(cfg_dir_len+strlen(PLAYLISTFN));
	strcpy(*playlist_fn,cfg_dir);
	strcat(*playlist_fn,PLAYLISTFN);

	*state_fn = malloc(cfg_dir_len+strlen(STATEFN));
	strcpy(*state_fn,cfg_dir);
	strcat(*state_fn,STATEFN);

	free(cfg_dir);
}
void prefix_eval(int plc, char** pl, char** prefix, int* prefixc){ //evaluates prefix
	int pc=strlen(pl[0]);
	//*prefixc=strlen(pl[0]);
	pc=prefix_len(pc,plc,pl);
	//*prefixc=prefix_len(*prefixc,plc,pl);
	*prefix=malloc(pc+1);
	memcpy(*prefix,pl[0],pc+1);
	(*prefix)[pc]=0;
	(*prefixc)=pc;
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
	printf( USAGE );
	halt(0);
}
void p_output_init(){
	CK( pipe(pipefd) );
	CK( pids[P_OUTPUT]=vfork() );
	if(0==pids[P_OUTPUT]){
		close(pipefd[1]);
		dup2(pipefd[0],0);
		close(pipefd[0]);
		execlp( OUTEXEC ,(char*)0);
	}
}
void p_output(){
	printf("ERROR: Output process died unexpectedly. Terminating.\n");
	halt(2);
}
void p_decode(char*** pl,int plc, int prefixc){
	// caching is a hack to tell ffmpeg to cache more of the input
	char cache_fn[4096+1+6]="cache:"; /* max_path + null + "cache:" */
	char ss_buf[22]={'0',0,'0',0}; // contains start location

	if(plc==++state.pos) halt(0);
	if(state.pos<0) state.pos=0;
	state.off=0;

	printf("\e[2K\r%s\n",&((*pl)[state.pos][prefixc]));
	clock_gettime(CLOCK_MONOTONIC,&start);
	char* ss=ss_buf;
	if(state.off>0){
		start.tv_sec-=state.off;
		ss=&ss_buf[2];
		CK( snprintf(ss,20,"%ld",state.off) );
		state.off=0;
	}
	if(pids[P_REFRESH]>0)
		kill(pids[P_REFRESH],SIGINT);
	CK( pids[P_DECODE]=vfork() );
	if(0==pids[P_DECODE]){
		close(pipefd[0]);
		dup2(pipefd[1],1);
		close(pipefd[1]);
		strcpy(&cache_fn[6],(*pl)[state.pos]);
		//execlp("ffmpeg","-hide_banner","-ss",ss,"-i",cache_fn,"-loglevel","-8","-af","volume=0.75","-ac","2","-ar","44100","-f","f32le","-",
		execlp("ffmpeg","-hide_banner","-ss",ss,"-i",cache_fn,"-loglevel","-8","-ac","2","-ar","44100","-f","f32le","-",
			#ifdef DEBUG
	//		"-report",
			#endif
			(char*)0
		);
		cache_fn[6]='\0';
	}
}
void p_input(int ret){
	char key;
	switch(key=WEXITSTATUS(ret)){
	case 'h': case 'H': printf( "\n" KBUSAGE ); break;
	case 'q': case 'Q': halt(0);
	case 'p': case 'P': state.pos--; //fallthru
	case 'r': case 'R': state.pos--; //fallthru
	case 'n': case 'N': kill(pids[P_DECODE],SIGINT);
	}
	CK( pids[P_INPUT]=fork() );
	if(0==pids[P_INPUT]){
		read(0,&key,1);
		exit(key);
	}
}
void p_refresh(int plc){
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC,&now);
	printf("\e[2K\r%d/%d %ld:%02ld > ",state.pos+1,plc,
		(now.tv_sec-start.tv_sec)/60,
		(now.tv_sec-start.tv_sec)%60
	);
	state.off=now.tv_sec-start.tv_sec;
	CK( pids[P_REFRESH]=fork() );
	if(0==pids[P_REFRESH]){
		usleep((start.tv_nsec+1E9-now.tv_nsec)/1E3);
		exit(0);
	}
}
int playback(char** pl,int plc,int prefixc){
	//#ifndef DEBUG
	close(2); //hide stderr
	//#endif

	p_output_init();

	//TODO: refactor using poll()
	int ret=0;
	while(1){
		//printf("%d\n",pids[P_RET]);
		if( pids[P_RET]==pids[P_OUTPUT] )
			{ p_output(); }
		else if(pids[P_RET]==pids[P_DECODE])
			{ p_decode(&pl,plc,prefixc); }
		else if(pids[P_RET]==pids[P_INPUT])
			{ p_input(ret); } 
		else if(pids[P_RET]==pids[P_REFRESH]){
			p_refresh(plc);
		}
		pids[P_RET] = pids[P_RET]<0 ? pids[P_RET]+1 : wait(&ret);
	}
}
void load_state(char* state_fn){ //loads state from state_fn or initializes
	struct stat st;
	if(0>stat(state_fn,&st)){
		int fd;
		CK( fd=open(state_fn,O_RDONLY) );
		CK( read(fd,&state,sizeof(statest))-sizeof(statest) );
		CK( close(fd) );
	}else{
		state.off=0;
		state.pos=-1;
	}
}
void save_state(char* state_fn){
	int fd;
	CK( fd=open(state_fn,O_WRONLY|O_CREAT|O_TRUNC,0775) );
	CK( write(fd,&state,sizeof(statest)) );
	CK( close(fd) );
}
void shuffle(int c,char** v){
	if(!rseed){
		rseed=time(NULL);
		srandom(rseed);
	}
	for(int i=0;i<(c-1);i++){ //shuffle
		int j=(random()%(c-i))+i;
		char* tmp=v[i];
		v[i]=v[j];
		v[j]=tmp;
	}
}
void parse_options(int argc,char**argv,int*shuffle_flag){
	static struct option long_options[] = {
		{"help",    no_argument, 0, 'h' },
		{"shuffle", no_argument, 0, 's' },
		{0,         0,           0, 0   }
	};

	//when implementing optional args, see
	//https://cfengine.com/blog/2021/optional-arguments-with-getopt-long/
	int opt;
	while ((opt = getopt_long (argc, argv, "ahs",long_options,NULL)) != -1){
		switch(opt){
		case 's':
			(*shuffle_flag)=!0;
			break;
      case '?': printf("\n"); //fallthru
		case 'h': print_usage(); //terminates
		case '\0': break;
		default:
			printf("Unexpected option %c %d.\n\n",opt,opt);
			print_usage();
		}
	}
}
int main(int argc, char **argv){
	termios_state=malloc(sizeof(struct termios));
	CK( tcgetattr(STDIN,termios_state) );

	int shuffle_flag=0;
	parse_options(argc,argv,&shuffle_flag);

	char*playlist_fn,*state_fn,*prefix;
	dir_eval(&playlist_fn,&state_fn);
	
	char** pl;
	char* plmap;
	size_t plmapc;

	if(optind<argc){ //file args present
		if(shuffle_flag) shuffle(argc-optind,&argv[optind]);
		plmapc=validate_playlist(argc-optind,&argv[optind],&plmap,&pl); //only valid when executed with args!
		if(plmapc<1){
			printf("ERROR: Invalid file specified.\n");
			print_usage();
		}
	}else{ //no file args
		struct stat st;
		if((0>stat(playlist_fn,&st)) || (1>st.st_size)){
			printf("ERROR: Playlist is empty. Run " PKG " with a list of files as argument to generate one.\n\n");
			print_usage();
		}
		//if(argc<2) load_state(state_fn);
		halt(0);
		//TODO: reimplement playlist load
	}

	int prefixc;
	prefix_eval(argc-optind,pl,&prefix,&prefixc);
	printf("%s\n",prefix);

	playback(pl,argc-optind,prefixc);
	save_state(state_fn);

	halt(0);
}

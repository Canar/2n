
#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "core.h"
#include "fork.h"

int rseed=0;

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
		}else{ //realpath fails
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
	if(home_dir==NULL) prerror(-1,"ERROR: $HOME unset. Environment appears faulty.\n");

	//make config dir CFGDIR
	int cfg_dir_len=strlen(home_dir)+strlen(CFGDIR)+1;
	char* cfg_dir=malloc(cfg_dir_len);
	strcpy(cfg_dir,home_dir);
	strcat(cfg_dir,CFGDIR);
	int ret=mkdir(cfg_dir,0775);
	if((-1==ret)&&(EEXIST != errno)) prerror(-100,"ERROR: %d in mkdir()\n",errno);

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
void load_state(){ //loads state from state_fn or initializes
	struct stat st;
	int ret=stat(state_fn,&st);
	if(0>=ret){
		int fd;
		CK( fd=open(state_fn,O_RDONLY) );
		CK( read(fd,&state,sizeof(statest))-sizeof(statest) );
		CK( close(fd) );
		state.pos--;
	}else{
		state.off=0;
		state.pos=-1;
	}
}
void read_buffer(char*fn,size_t*c,char**v){
	struct stat st;
	int fd;
	CK( stat(fn,&st) );
	(*c)=st.st_size;
	(*v)=malloc(*c);
	CK( fd=open(fn,O_RDONLY) );
	CK( read(fd,(*v),(*c)) );
	CK( close(fd) );
}
void shuffle(int c,char** v){
	if(!rseed)srandom(rseed=time(NULL));
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
void partition_buffer(size_t bc,char*bv,int*sc,char***sv){ //deserializes saved playlist
	(*sc)=0;
	for(int i=1;i<bc;i++)//count nulls
		if(0==bv[i])
			(*sc)++;

	(*sv)=malloc((*sc)*sizeof(char*));
	int j=1;
	(*sv)[0]=&bv[0];
	for(int i=1; (i<bc) && (j<(*sc)) ;i++)
		if(0==bv[i])
			(*sv)[j++]=&bv[i+1];
}
int main(int argc, char **argv){
	CK( tcgetattr(STDIN,&termios_state) );

	int shuffle_flag=0;
	parse_options(argc,argv,&shuffle_flag);
	dir_eval(&playlist_fn,&state_fn);
	
	char** pl;
	int plc=argc-optind,prefixc;
	char*plmap,*prefix;
	size_t plmapc;

	if(optind<argc){ //file args present
		if(shuffle_flag) shuffle(argc-optind,&argv[optind]);
		plmapc=validate_playlist(argc-optind,&argv[optind],&plmap,&pl);
		prerror(plmapc-1,"ERROR: Invalid file specified.\n");
		write_buffer(playlist_fn,plmapc,plmap);
	}else{ //no file args
		struct stat st;
		if((0>stat(playlist_fn,&st)) || (1>st.st_size))
			prerror(-1,"ERROR: Playlist is empty. Run " PKG " with a list of files as argument to generate one.\n");
		read_buffer(playlist_fn,&plmapc,&plmap);
		partition_buffer(plmapc,plmap,&plc,&pl);
		load_state();
	}
	prefix_eval(plc,pl,&prefix,&prefixc);
	if(prefixc>0)printf("%s\n",prefix);

	//#ifndef DEBUG
	close(2); //hide stderr
	//#endif

	playback(pl,plc,prefixc); //terminates
}

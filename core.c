#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "core.h"

int pids[(int)P_ENUM_COUNT]={-3,-1,-2,-3,-4};
statest state={.off=0,.pos=-1};
struct timespec start;
char*playlist_fn,*state_fn;
struct termios termios_state; 

void halt(int x){
	platform_halt();
	CK( tcsetattr(STDIN,TCSANOW,&termios_state) );
	if(0==x){
		printf("\e[2K\r    ]  " PKGVER "  [    \n");
	}
	exit(x);
}
void vck(int retval,char* file,int line,char* msg){ //unhandled
	if(retval>=0)
		return;
	fprintf(stderr, "Error in %s, line %d. Value: %d. %s\n",file,line,retval,msg);
	halt(1);
}
void prerror(int test,char* format,...){ //terminal errors
	if(test>=0)return;
	va_list args;
	va_start(args, format);
	vfprintf(stdout,format,args);
	va_end(args);
	halt(abs(test));
}
void write_buffer(char*fn,int c,char* v){
	int fd;
	CK( fd=open(fn,O_WRONLY|O_CREAT|O_TRUNC,PERM) );
	CK( write(fd,v,c) );
	CK( close(fd) );
}
void save_state(){
	write_buffer(state_fn,sizeof(statest),(char*)&state);
	/*
	int fd;
	CK( fd=open(state_fn,O_WRONLY|O_CREAT|O_TRUNC,PERM) );
	CK( write(fd,&state,sizeof(statest)) );
	CK( close(fd) );
	*/
}
int offsec(){
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC,&now);
	return(now.tv_sec-start.tv_sec);
}

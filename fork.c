#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "core.h"
#include "fork.h"

int pipefd[2];
int dpipe[2],ppipe[2];//decode,playback


void platform_halt(){
	for(int i=1;i<P_ENUM_COUNT;i++)
		if(pids[i]>0)
			kill(pids[i],SIGINT);
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
void pp_output_init(){
	CK( pipe(ppipe) );
	CK( pids[P_OUTPUT]=vfork() );
	if(0==pids[P_OUTPUT]){
		close(ppipe[1]);
		dup2(ppipe[0],0);
		close(ppipe[0]);
		execlp( OUTEXEC ,(char*)0);
	}
}
void p_output(){
	prerror(-2,"ERROR: Output process died unexpectedly. Terminating.\n");
}
void p_decode(char*** pl,int plc, int prefixc){
	// caching is a hack to tell ffmpeg to cache more of the input
	char cache_fn[4096+1+6]="cache:"; /* max_path + null + "cache:" */
	char ss_buf[22]={'0',0,'0',0}; // contains start location

	if(plc==++state.pos) halt(0);
	if(state.pos<0) state.pos=0;
	//state.off=0;

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
	case 'q': case 'Q': 
		state.off=offsec();
		save_state();
		halt(0);
	case 'h': case 'H': printf( "\n" KBUSAGE ); break;
	case 'p': case 'P': state.pos--; //fallthru
	case 'r': case 'R': state.pos--; //fallthru
	case 'n': case 'N': 
		state.off=0;
		kill(pids[P_DECODE],SIGINT);
	}
	CK( pids[P_INPUT]=fork() );
	if(0==pids[P_INPUT]){
		read(0,&key,1);
		exit(key);
	}
}
void p_refresh(int plc){
	int t=offsec();
	printf("\e[2K\r%lu/%d %d:%02d > ",state.pos+1,plc,t/60,t%60);
	CK( pids[P_REFRESH]=fork() );
	if(0==pids[P_REFRESH]){
		usleep(1000000);
		exit(0);
	}
}
int playback(char** pl,int plc,int prefixc){
	p_output_init();

	int ret=0;
	while(1){
		if( pids[P_RET]==pids[P_OUTPUT] )
			{ p_output(); }
		else if(pids[P_RET]==pids[P_DECODE])
			{ p_decode(&pl,plc,prefixc); }
		else if(pids[P_RET]==pids[P_INPUT])
			{ p_input(ret); } 
		else if(pids[P_RET]==pids[P_REFRESH])
			{ p_refresh(plc); }
		pids[P_RET] = pids[P_RET]<0 ? pids[P_RET]+1 : wait(&ret);
	}
}
int pplayback(char**pl,int plc,int prefixc){
	struct pollfd fds[3];
	fds[0].fd=STDIN;
	fds[0].events=POLLIN;

	pp_output_init();
	close(ppipe[0]);
	fds[1].fd=ppipe[1];
	fds[1].events=POLLOUT;

	int ret=0;
	while(1){
		if( pids[P_RET]==pids[P_OUTPUT] )
			{ p_output(); }
		else if(pids[P_RET]==pids[P_DECODE])
			{ p_decode(&pl,plc,prefixc); }
		else if(pids[P_RET]==pids[P_INPUT])
			{ p_input(ret); } 
		else if(pids[P_RET]==pids[P_REFRESH])
			{ p_refresh(plc); }
		pids[P_RET] = pids[P_RET]<0 ? pids[P_RET]+1 : wait(&ret);
	}
}

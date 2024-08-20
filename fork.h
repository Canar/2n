#ifndef FORK_H
#define FORK_H

/* fork() job implementation */
void p_output_init();
void pp_output_init();
void p_output();
void p_decode(char*** pl,int plc, int prefixc);
void p_input(int ret);
void p_refresh(int plc);
int playback(char** pl,int plc,int prefixc);
int pplayback(char**pl,int plc,int prefixc);

extern int pipefd[2];
extern int dpipe[2],ppipe[2];//decode,playback

#endif

#include "alias.h"
#include "get_path.h"
#include "users.h"

int pid;
int sh( int argc, char **argv, char **envp);
char *which(char *command, struct pathelement *pathlist);
void where(char *command, struct pathelement *pathlist);
void list ( char *dir );
void printenv(char **envp);
char *cd( char *dir, char *homedir, char *prevdir);
void kill_process_signal(char *process, char *signal);
void kill_process(char *process);
void *warnload(void *args);
void sig_handle(int sig);

#define PROMPTMAX 32
#define MAXARGS 10

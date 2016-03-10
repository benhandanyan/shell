#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <glob.h>
#include "sh.h"

int sh( int argc, char **argv, char **envp ) {
	//signal(SIGINT, sig_handle);
	signal(SIGINT, sig_handle);
	signal(SIGTERM, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	char *prompt = calloc(PROMPTMAX, sizeof(char));
	char *commandline = calloc(MAX_CANON, sizeof(char));
	char *command, *arg, *commandpath, *p, *pwd, *owd;
	char **args = calloc(MAXARGS, sizeof(char*));
	int uid, i, status, argsct, a, go = 1;
	struct passwd *password_entry;
	char *homedir;
	struct pathelement *pathlist, *history;
	struct aliaselement *aliases;
	const char sp[2] = " ";
	extern char **environ;
	glob_t globbuf;
	char **gl;

	uid = getuid();
	password_entry = getpwuid(uid);               /* get passwd info */
	homedir = getenv("HOME");						/* get homedir */
     
	if ( (pwd = getcwd(NULL, PATH_MAX+1)) == NULL ) {
    	perror("getcwd");
    	exit(2);
	}
	
	owd = calloc(strlen(pwd) + 1, sizeof(char));
	memcpy(owd, pwd, strlen(pwd));
	prompt[0] = ' '; prompt[1] = '\0';

	/* Put PATH into a linked list */
	pathlist = get_path();
	history = NULL;
	aliases = NULL;

	while ( go ) {
    	printf(prompt); /* print your prompt */
    	printf(" [");
    	printf(pwd);
    	printf("]> ");

    	i = 0;

    	while (fgets(commandline, MAX_CANON, stdin) != NULL) { /* get command line and process */
			if (commandline[strlen(commandline) - 1] == '\n') {
				commandline[strlen(commandline) - 1] = 0; /* replace newline with null */
			}

			/* Add the command to history */
			history = add_last(history, commandline);

			/* Get the command */	
			command = strtok(commandline, sp);
			if(command == NULL) break;

			//Search for aliases
			a = 0;	
			struct aliaselement *alias = aliases;
			while(alias != NULL) {
				if(strcmp(command, alias->name) == 0) {
					a = 1;
					break;
				}
				alias = alias->next;
			}

			/* If we have an alias */
			if(a) {
				strcpy(commandline, alias->command);
				command = strtok(commandline, sp);
			} 

			i = 0;
			while (command != NULL) {
	    		if(i == MAXARGS) {
					strcpy(args[0], "maxargs");
					break;
				}
	    		args[i] =  calloc(strlen(command) + 1, sizeof(char));
	   			strcpy(args[i], command);
	    		command = strtok(NULL, sp);
	    		i++;
			}
	
			/* make sure the user passed in something */
			if(args[0] == NULL) {
				break;
			}

			/* Expand wildcard characters */
			glob(args[0], GLOB_NOCHECK, NULL, &globbuf);
			i = 1;	
			while(args[i] != NULL) {
 				glob(args[i], GLOB_APPEND | GLOB_NOCHECK, NULL, &globbuf);
				i++;
			}
		
			gl = globbuf.gl_pathv;
	
    		/* check for each built in command and implement */
			
			/* Built in list */
			if(strcmp(gl[0], "list") == 0) {
	    		printf("Executing built-in [%s]\n", gl[0]);
	    		i = 1;
	    		if(gl[i] == NULL) {
	    			list(pwd);
	    		} else {
	        		while(gl[i] != NULL) {
	            		list(gl[i]);
		    			printf("\n");
		    			i++;
	        		}
	    		}
    		} 

			/* Built in exit */
			else if (strcmp(gl[0], "exit") == 0) {
 	    		printf("Executing built-in [%s]\n", gl[0]);
	    		go = 0;
	    		break;
			} 

			/* Built in prompt */
			else if (strcmp(gl[0], "prompt") == 0) {
 	    		printf("Executing built-in [%s]\n", gl[0]);
	    		if(gl[1] == NULL) {
					printf("input prompt prefix:");
					fgets(prompt, PROMPTMAX, stdin);
					if(prompt[strlen(prompt) - 1] == '\n') {
		    			prompt[strlen(prompt) - 1] = 0;
					}
	    		} else {
					strncpy(prompt, gl[1], PROMPTMAX);
	    		}
			} 

			/* Built in pwd */
			else if (strcmp(gl[0], "pwd") == 0) {
 	    		printf("Executing built-in [%s]\n", gl[0]);
	    		printf(pwd);
	    		printf("\n");
			} 

			/* Built in pid */
			else if (strcmp(gl[0], "pid") == 0) {
 	    		printf("Executing built-in [%s]\n", gl[0]);
	    		printf("%d\n", getpid());
			} 

			/* Built in which */
			else if (strcmp(gl[0], "which") == 0) {
 	    		printf("Executing built-in [%s]\n", gl[0]);
				if(gl[1] == NULL) {
					fprintf(stderr, "which: Too few arguments.\n");
				} else {
					i = 1;
					while(gl[i] != NULL) {
						char *wh;
	    				wh = which(gl[i], pathlist);
						if(wh == NULL) { 
							fprintf(stderr, "%s: Command not found.\n", gl[i]);
						} else {
							printf("%s\n", wh);
						}
						free(wh);
						i++;
					}
				}	   
			} 

			/* Built in where */
			else if (strcmp(gl[0], "where") == 0) {
 	    		printf("Executing built-in [%s]\n", gl[0]);
				if(gl[1] == NULL) {
					fprintf(stderr, "where: Too few arguments.\n");
				} else {
					i = 1;
					while(gl[i] != NULL) {
						where(gl[i], pathlist);
						i++;
					}	
				} 
			}

			/* Built in cd */
			else if (strcmp(gl[0], "cd") == 0) {
 	    		printf("Executing built-in [%s]\n", gl[0]);
				if(gl[2] != NULL) {
					printf("cd: Too many arguments.\n");
				} else {
	    			char *tmp;	    
	    			tmp = cd(gl[1], homedir, owd);
	    			if(tmp == NULL) {
						break;
	    			} else { 
						free(owd);
						owd = pwd;
						pwd = tmp;
	    			}    
	    			tmp = NULL;
				}
			} 

			/* Built in printenv */
			else if (strcmp(gl[0], "printenv") == 0) {
 	    		printf("Executing built-in [%s]\n", gl[0]);
	    		if(gl[2] != NULL) {
					fprintf(stderr, "%s: Too many arguments.\n", gl[0]);
	    		} else if (gl[1] != NULL) { 
					char *tmp;
					tmp = getenv(gl[1]);
					if(tmp == NULL) {
						fprintf(stderr, "%s: Environmental variable not found.\n", gl[1]);
					} else {
						printf("%s\n", tmp);
					}
					tmp = NULL;
	    		} else {
	        		i = 0;
	        		while(environ[i] != NULL) {
		    			printf("%s\n", environ[i]);
		    			i++;
					}
	    		}
			} 

			/* Built in setenv */
			else if (strcmp(gl[0], "setenv") == 0) {
 	    		printf("Executing built-in [%s]\n", gl[0]);
				if(gl[3] != NULL) {
					fprintf(stderr, "%s: Too many arguments.\n", gl[0]);
				} else if (gl[2] != NULL) {
					setenv(gl[1], gl[2], 1);
				} else if (gl[1] != NULL) {
					setenv(gl[1], "", 1);
				} else {
					i = 0;
					while(environ[i] != NULL) {
						printf("%s\n", environ[i]);
						i++;
					}
				}
				homedir = getenv("HOME");
			} 

			/* Built in history */
			else if (strcmp(gl[0], "history") == 0) {
 	    		printf("Executing built-in [%s]\n", gl[0]);
				int n = 10;
				if(gl[1] != NULL && atoi(gl[1]) != 0) {
					n = atoi(gl[1]);
				}
				struct pathelement *curr = history;
				while(curr != NULL && n > 0) {
					printf("%s\n", curr->element);
					curr = curr->next;
					n--;
				}
			} 

			/* Built in alias */
			else if (strcmp(gl[0], "alias") == 0) {
 	    		printf("Executing built-in [%s]\n", gl[0]);
				if(gl[1] == NULL) {
					struct aliaselement *curr = aliases;
					while(curr != NULL) {
						printf("%s     %s\n", curr->name, curr->command);
						curr = curr->next;
					}
				} else if(gl[2] == NULL) {
					struct aliaselement *curr = aliases;
					while(curr != NULL) {
						if(strcmp(gl[1], curr->name) == 0) {
							printf("%s\n", curr->command);
						}
						curr = curr->next;
					}
				} else {
					char buf[MAX_CANON];
					snprintf(buf, MAX_CANON, "%s", gl[2]);
					i = 3;
					while(gl[i] != NULL && i < MAXARGS) {
						strcat(buf, " ");
						strcat(buf, gl[i]);
						i++;
					}
					aliases = add_alias(aliases, gl[1], buf);
				}
			} 

			/* Built in kill */
			else if (strcmp(gl[0], "kill") == 0) {
 	    		printf("Executing built-in [%s]\n", gl[0]);
				if(gl[1] == NULL) {
					fprintf(stderr, "kill: Too few arguments.\n");
				} else if(gl[2] == NULL || strchr(gl[1], '-') == NULL) {
					i = 1;
					while(gl[i] != NULL) {
						kill_process(gl[i]);
						i++;
					}
				} else {
					char *signal;
					signal = strtok(gl[1], "-");
					i = 2;
					while(gl[i] != NULL) {
						kill_process_signal(gl[i], signal);	
						i++;
					}
				}
			} 

			/* Absolute/Relative paths */
			else if (strncmp(gl[0], "/", 1) == 0 || strncmp(gl[0], "./", 2) == 0 || strncmp(gl[0], "../", 3) == 0) {
				if(access(gl[0], X_OK) == 0) {
					pid_t pid = fork();
					if(pid == -1) {
						perror("fork");
					} else if(pid == 0) {              /* Child */
 	    				printf("Executing [%s]\n", gl[0]);
						execve(gl[0], gl, environ);
						perror(gl[0]);
						exit(127);
					} else {                           /* Parent */
						waitpid(pid, &status, 0);
						if(WEXITSTATUS(status) != 0) {
							printf("Exit: %d\n", WEXITSTATUS(status));
						}
					}
				} else {
					fprintf(stderr,"%s: Command not found.\n", gl[0]);
				}
			} 

			/* MAXARGS handler */
			else if (strcmp(gl[0], "maxargs") == 0) {
	    		fprintf(stderr, "Error: Too many arguments.\n");
			} 

			/* Executable */
			else {
				char *wh;
				wh = which(gl[0], pathlist);
				if(wh == NULL) {
					fprintf(stderr, "%s: Command not found.\n", gl[0]);   
				} else {
					pid_t pid = fork();
					if(pid == -1) {
						perror("fork");
					} else if (pid == 0) {
 	    				printf("Executing [%s]\n", wh);
						execve(wh, gl, environ);
						perror(wh);
						exit(127);
					} else {
						waitpid(pid, &status, 0);
						if(WEXITSTATUS(status) != 0){
							printf("Exit: %d\n", WEXITSTATUS(status));
						}
					}
					free(wh);
				} 
    		}
		
			/* reset glob */
			globfree(&globbuf);
			gl = NULL;

			i = 0;
			while(args[i] != NULL) {
	    		free(args[i]);
	    		args[i] = NULL;
	    		i++;
			}

			/* Print prompt again */
			printf(prompt);
			printf(" [");
			printf(pwd);
			printf("]> ");
    	}
	}

	/* free allocated memory */
	free(prompt);
	free(commandline);
	i = 0;
	while(args[i] != NULL) {
    	free(args[i]);
    	args[i] = NULL;
    	i++;
	}
	free(args);
	free(owd);
	free(pwd);
	struct pathelement *tmp;
	while(pathlist != NULL) {
    	tmp = pathlist->next;
    	free(pathlist);
    	pathlist = tmp;
	}
	while(history != NULL) {
		tmp = history->next;
		free(history->element);
		free(history);
		history = tmp;
	}
	struct aliaselement *temp;
	while(aliases != NULL) {
		temp = aliases->next;
		free(aliases->name);
		free(aliases->command);
		free(aliases);
		aliases = temp;
	}
	globfree(&globbuf);

	return 0;
} /* sh() */

char *which(char *command, struct pathelement *pathlist ) {
	/* loop through pathlist until finding command and return it.  Return
	NULL when not found. */
	struct pathelement *current = pathlist;
	char *buf;
	buf = malloc(MAX_CANON * sizeof(char));
	while(current != NULL) {
		snprintf(buf, MAX_CANON, "%s/%s", current->element, command);
		if(access(buf, F_OK) == 0) {
    		return buf;
			break;
    	}
   	 	current = current->next;
	}
	free(buf);
	buf = NULL;
	return buf;
} /* which() */

void where(char *command, struct pathelement *pathlist ) {
	/* similarly loop through finding all locations of command */
	struct pathelement *current = pathlist;
	char buf[MAX_CANON];
	while(current != NULL) {
		snprintf(buf, MAX_CANON, "%s/%s", current->element, command);
		if(access(buf, F_OK) == 0) 
			printf("%s\n", buf);
		current = current->next;
	}
} /* where() */

void list ( char *dir ) {
	/* see man page for opendir() and readdir() and print out filenames for
	the directory passed */
  
	DIR* dirstream;
	struct dirent* files;
	dirstream = opendir(dir);

	if(dirstream == NULL) {
		perror(dir);
	} else {
    	printf("%s:\n", dir);
		while((files = readdir(dirstream)) != NULL){
        	printf("%s\n", files->d_name);
    	}
	}
	free(dirstream);
} /* list() */

char *cd( char *dir, char *homedir, char *prevdir ) {
    if(dir == NULL) dir = homedir;
    if(strcmp(dir, "-") == 0) dir = prevdir;
    if(chdir(dir) == -1) {
		perror(dir);
		return NULL;
    } else {
		return getcwd(NULL, PATH_MAX+1);
    }
}

/* try to kill the given process with the given signal */
void kill_process_signal(char* process, char *signal) {
	int i = 0;
	if(atoi(process) && atoi(signal))
    	i = kill(atoi(process), atoi(signal));
    else
    	fprintf(stderr, "kill: Arguments should be jobs or process id's.\n");
    if(i == -1)
    	perror(process);
}

/* try to terminate the given process */
void kill_process(char *process) {
	int i = 0;
	if(atoi(process))
		i = kill(atoi(process),SIGTERM);
	else
		fprintf(stderr, "kill: Arguments should be jobs or process id's.\n");
	if(i==-1)
		perror(process);
}

void sig_handle(int sig) {
	signal(sig, sig_handle);
}

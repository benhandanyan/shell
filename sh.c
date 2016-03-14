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
	signal(SIGINT, sig_handle);
	signal(SIGTERM, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	char *prompt = calloc(PROMPTMAX, sizeof(char));
	char *commandline = calloc(MAX_CANON, sizeof(char));
	char *command, *arg, *aliashelp, *commandpath, *p, *pwd, *owd;
	/* Use these two save chars for strtok_r on command and alias commands*/
	char *command_save, *alias_save;
	char **args = calloc(MAXARGS, sizeof(char*));
	int uid, i, status, argsct, a, go = 1;
	struct passwd *password_entry;
	char *homedir;
	struct pathelement *pathlist, *history;
	struct aliaselement *aliases;
	const char sp[2] = " ";
	extern char **environ;
	glob_t globbuf;
	size_t glc;
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

	/* By default, we have no history or aliases */
	history = NULL;
	aliases = NULL;

	while ( go ) {
		/* print prompt */
		printf("\n");
    	printf(prompt);
    	printf(" [");
    	printf(pwd);
    	printf("]> ");

    	i = 0;

		/* get command line and process */
    	while (fgets(commandline, MAX_CANON, stdin) != NULL) {
			if (commandline[strlen(commandline) - 1] == '\n') {
				commandline[strlen(commandline) - 1] = 0;
			}

			/* Add the command to history */
			history = add_last(history, commandline);

			/* Get the command */	
			command = strtok_r(commandline, sp, &command_save);
			if(command == NULL) {
				break;
			}

			/* Search for aliases */
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
			i = 0;
			if(a) {
				/* parse alias command */
				arg = calloc(strlen(alias->command) + 1, sizeof(char));
				strcpy(arg, alias->command);
				aliashelp = strtok_r(arg, sp, &alias_save);
				while(aliashelp != NULL) {
					if(i == MAXARGS) {
						strcpy(args[0], "maxargs");
						break;
					}
					args[i] = calloc(strlen(aliashelp) + 1, sizeof(char));
					strcpy(args[i], aliashelp);
					aliashelp = strtok_r(NULL, sp, &alias_save);
					i++;
				}
				command = strtok_r(NULL, sp, &command_save);
				free(arg);
			} 

			/* parse command line or remainder of command line if we have an alias */
			while (command != NULL) {
	    		if(i == MAXARGS) {
					strcpy(args[0], "maxargs");
					break;
				}
	    		args[i] =  calloc(strlen(command) + 1, sizeof(char));
	   			strcpy(args[i], command);
	    		command = strtok_r(NULL, sp, &command_save);
	    		i++;
			}
			
			/* SANITY CHECK: make sure the user passed in something */
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
		
			/* gl becomes our arguments, it is the expanded version of args */
			gl = globbuf.gl_pathv;
			/* glc is the number of arguments. Use it for checking built in commands */
			glc = globbuf.gl_pathc;
	
    		/* check for each built in command and implement */
			/* Built in list */
			if(strcmp(gl[0], "list") == 0) {
	    		printf("Executing built-in [%s]\n", gl[0]);
	    		i = 1;
				/* No arguments, print the current working directory */
	    		if(glc < 2) {
	    			list(pwd);
	    		} else {
					/* list each of the arguments passed in */
	        		while(i < glc) {
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
	    		if(glc < 2) {
					/* user didn't enter a prompt so request one */
					printf("input prompt prefix:");
					fgets(prompt, PROMPTMAX, stdin);
					if(prompt[strlen(prompt) - 1] == '\n') {
		    			prompt[strlen(prompt) - 1] = 0;
					}
	    		} else {
					/* set the prompt to be the first argument */
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
				if(glc < 2) {
					fprintf(stderr, "which: Too few arguments.\n");
				} else {
					i = 1;
					while(i < glc) {
						char *wh;
						/* call the which function which will check for the command in the path */
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
				if(glc < 2) {
					fprintf(stderr, "where: Too few arguments.\n");
				} else {
					i = 1;
					while(i < glc) {
						where(gl[i], pathlist);
						i++;
					}	
				} 
			}

			/* Built in cd */
			else if (strcmp(gl[0], "cd") == 0) {
 	    		printf("Executing built-in [%s]\n", gl[0]);
				if(glc > 2) {
					printf("cd: Too many arguments.\n");
				} else {
	    			char *tmp;	    
					/* tmp will be the new directory or NULL on failure */
	    			tmp = cd(gl[1], homedir, owd);
	    			if(tmp == NULL) {
						break;
	    			} else { 
						free(owd);
						/* set owd to be the old (previous) working directory */
						owd = pwd;
						/* set the new working directory */
						pwd = tmp;
	    			}    
	    			tmp = NULL;
				}
			} 

			/* Built in printenv */
			else if (strcmp(gl[0], "printenv") == 0) {
 	    		printf("Executing built-in [%s]\n", gl[0]);
	    		if(glc > 2) {
					fprintf(stderr, "%s: Too many arguments.\n", gl[0]);
	    		} else if (glc > 1) { 
					/* print a particular environmental variable */
					char *tmp;
					tmp = getenv(gl[1]);
					if(tmp == NULL) {
						fprintf(stderr, "%s: Environmental variable not found.\n", gl[1]);
					} else {
						printf("%s\n", tmp);
					}
					tmp = NULL;
	    		} else {
					/* print all environmental variables */
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
				if(glc > 3) {
					fprintf(stderr, "%s: Too many arguments.\n", gl[0]);
				} else if (glc > 2) {
					/* set an environmental variable to the given value */
					setenv(gl[1], gl[2], 1);
				} else if (glc > 1) {
					/* set an environmental variable to an empty value */
					setenv(gl[1], "", 1);
				} else {
					/* print all environmental variables */
					i = 0;
					while(environ[i] != NULL) {
						printf("%s\n", environ[i]);
						i++;
					}
				}
				/* in case we update the home directory */
				homedir = getenv("HOME");
			} 

			/* Built in history */
			else if (strcmp(gl[0], "history") == 0) {
 	    		printf("Executing built-in [%s]\n", gl[0]);
				int n = 10;
				/* set how many history records to print */
				if(glc > 1 && atoi(gl[1]) != 0) {
					n = atoi(gl[1]);
				}
				struct pathelement *curr = history;
				/* loop and print past commands */
				while(curr != NULL && n > 0) {
					printf("%s\n", curr->element);
					curr = curr->next;
					n--;
				}
			} 

			/* Built in alias */
			else if (strcmp(gl[0], "alias") == 0) {
 	    		printf("Executing built-in [%s]\n", gl[0]);
				if(glc < 2) {
					/* list all aliases */
					struct aliaselement *curr = aliases;
					while(curr != NULL) {
						printf("%s     %s\n", curr->name, curr->command);
						curr = curr->next;
					}
				} else if(glc < 3) {
					/* list a specific alias */
					struct aliaselement *curr = aliases;
					while(curr != NULL) {
						if(strcmp(gl[1], curr->name) == 0) {
							printf("%s\n", curr->command);
						}
						curr = curr->next;
					}
				} else {
					/* add an alias */
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
				if(glc < 2) {
					fprintf(stderr, "kill: Too few arguments.\n");
				} else if(glc < 3 || strchr(gl[1], '-') == NULL) {
					/* default kill with SIGINT */
					i = 1;
					while(gl[i] != NULL) {
						kill_process(gl[i]);
						i++;
					}
				} else {
					/* kill with the given signal number */
					char *signal;
					signal = strtok(gl[1], "-");
					i = 2;
					while(gl[i] != NULL) {
						kill_process_signal(gl[i], signal);	
						i++;
					}
				}
			} 

			/* MAXARGS handler */
			else if (strcmp(gl[0], "maxargs") == 0) {
	    		fprintf(stderr, "Error: Too many arguments.\n");
			} 

			/* Absolute/Relative paths */
			else if (strncmp(gl[0], "/", 1) == 0 || strncmp(gl[0], "./", 2) == 0 || strncmp(gl[0], "../", 3) == 0) {
				if(access(gl[0], X_OK) == 0) {
					pid_t pid = fork();
					if(pid == -1) {
						perror("fork");
					} else if(pid == 0) {
						/* print what child is executing and execve it */
 	    				printf("Executing [%s]\n", gl[0]);
						execve(gl[0], gl, environ);
						/* on exec error */
						perror(gl[0]);
						exit(127);
					} else {
						/* wait for chil process */
						waitpid(pid, &status, 0);
						/* if child exits with non-zero status print it */
						if(WEXITSTATUS(status) != 0) {
							printf("Exit: %d\n", WEXITSTATUS(status));
						}
					}
				} else {
					/* path doens't exist */
					fprintf(stderr,"%s: Command not found.\n", gl[0]);
				}
			} 

			/* Executable */
			else {
				char *wh;
				/* figure out which executable to execute */
				wh = which(gl[0], pathlist);
				if(wh == NULL) {
					fprintf(stderr, "%s: Command not found.\n", gl[0]);   
				} else {
					pid_t pid = fork();
					if(pid == -1) {
						perror("fork");
					} else if (pid == 0) {
						/* what we are executing */
 	    				printf("Executing [%s]\n", wh);
						execve(wh, gl, environ);
						/* on execve error */
						perror(wh);
						exit(127);
					} else {
						/* wait for child */
						waitpid(pid, &status, 0);
						/* if child exits with nonzero value print it */
						if(WEXITSTATUS(status) != 0){
							printf("Exit: %d\n", WEXITSTATUS(status));
						}
					}
					free(wh);
				} 
    		}
		
			/* reset glob */
			globfree(&globbuf);

			/* reset args */
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
		/* we've found the command, so return it and exit */
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
		/* we've found the command, so print it and continue */
		if(access(buf, F_OK) == 0) { 
			printf("%s\n", buf);
		}
		current = current->next;
	}
} /* where() */

void list ( char *dir ) {
	/* see man page for opendir() and readdir() and print out filenames for
	the directory passed */
  
	DIR* dirstream;
	struct dirent* files;
	/*try to open the given direcotry */
	dirstream = opendir(dir);

	/* failed to open */
	if(dirstream == NULL) {
		perror(dir);
	} else {
		/* print what directory we're listing */
    	printf("%s:\n", dir);
		/* print the files in the directory */
		while((files = readdir(dirstream)) != NULL){
        	printf("%s\n", files->d_name);
    	}
	}
	free(dirstream);
} /* list() */

char *cd( char *dir, char *homedir, char *prevdir ) {
    if(dir == NULL) {
		/* with no dir, we want to change to the homedir */
		dir = homedir;
	} else if(strcmp(dir, "-") == 0) {
		/* change to the previous directory capability */
		dir = prevdir;
	}
    if(chdir(dir) == -1) {
		/* on chdir error */
		perror(dir);
		return NULL;
    } else {
		/* return the new directory after we change directories */
		return getcwd(NULL, PATH_MAX+1);
    }
}

/* try to kill the given process with the given signal */
void kill_process_signal(char* process, char *signal) {
	int i = 0;
	if(atoi(process) && atoi(signal)) {
		/* if we can convert process and signal to numbers, try to kill the process with the signal */
    	i = kill(atoi(process), atoi(signal));
	} else {
		/* something was wrong with process or signal */
    	fprintf(stderr, "kill: Arguments should be jobs or process id's.\n");
	}
	/* kill didn't work */
    if(i == -1)
    	perror(process);
}

/* try to terminate the given process */
void kill_process(char *process) {
	int i = 0;
	if(atoi(process)) {
		/* if we can convert process to number, try to kill the process with SIGTERM */ 
		i = kill(atoi(process),SIGTERM);
	} else {
		/* something was wrong with process */
		fprintf(stderr, "kill: Arguments should be jobs or process id's.\n");
	}
	/* kill didn't work */
	if(i==-1)
		perror(process);
}

/* handle CTRL-C */
void sig_handle(int sig) {
	signal(sig, sig_handle);
}

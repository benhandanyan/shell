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
#include "sh.h"

int sh( int argc, char **argv, char **envp )
{
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

  uid = getuid();
  password_entry = getpwuid(uid);               /* get passwd info */
  homedir = password_entry->pw_dir;		/* Home directory to start
						  out with*/
     
  if ( (pwd = getcwd(NULL, PATH_MAX+1)) == NULL )
  {
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

  while ( go )
  {
    printf(prompt); /* print your prompt */
    printf(" [");
    printf(pwd);
    printf("]> ");

    i = 0;
    while (fgets(commandline, MAX_CANON, stdin) != NULL) { /* get command line and process */
	if (commandline[strlen(commandline) - 1] == '\n')
		commandline[strlen(commandline) - 1] = 0; /* replace newline with null */
	
	history = add_last(history, commandline);

	i = 0;	
	command = strtok(commandline, sp);

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
	if(a) {
		strcpy(commandline, alias->command);
		command = strtok(commandline, sp);
	} 
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

    /* check for each built in command and implement */
    if(strcmp(args[0], "list") == 0) {
	    printf("Executing built-in [%s]\n", args[0]);
	    i = 1;
	    if(args[i] == NULL) {
	    	list(pwd);
	    } else {
	        while(args[i] != NULL) {
	            list(args[i]);
		    	printf("\n");
		    	i++;
	        	}
	    	}
    } else if (strcmp(args[0], "exit") == 0) {
		printf("exit\n");
	    go = 0;
	    break;
	} else if (strcmp(args[0], "prompt") == 0) {
 	    printf("Executing built-in [%s]\n", args[0]);
	    if(args[1] == NULL) {
			printf("input prompt prefix:");
			fgets(prompt, PROMPTMAX, stdin);
			if(prompt[strlen(prompt) - 1] == '\n')
		    	prompt[strlen(prompt) - 1] = 0;
	    } else {
			strncpy(prompt, args[1], PROMPTMAX);
	    }
	} else if (strcmp(args[0], "pwd") == 0) {
 	    printf("Executing built-in [%s]\n", args[0]);
	    printf(pwd);
	    printf("\n");
	} else if (strcmp(args[0], "pid") == 0) {
 	    printf("Executing built-in [%s]\n", args[0]);
	    printf("%d\n", getpid());
	} else if (strcmp(args[0], "which") == 0) {
 	    printf("Executing built-in [%s]\n", args[0]);
		if(args[1] == NULL) printf("which: Too few arguments.\n");
		i = 1;
		while(args[i] != NULL) {
	    	which(args[i], pathlist);
			i++;
		}	    
	} else if (strcmp(args[0], "where") == 0) {
 	    printf("Executing built-in [%s]\n", args[0]);
		if(args[1] == NULL) printf("where: Too few arguments.\n");
		i = 1;
		while(args[i] != NULL) {
			where(args[i], pathlist);
			i++;
		}	
	} else if (strcmp(args[0], "cd") == 0) {
 	    printf("Executing built-in [%s]\n", args[0]);
	    char *tmp;	    
	    tmp = cd(args[1], homedir, owd);
	    if(tmp == NULL) {
			break;
	    } else {
			free(owd);
			owd = pwd;
			pwd = tmp;
	    }    
	    tmp = NULL;
	} else if (strcmp(args[0], "printenv") == 0) {
 	    printf("Executing built-in [%s]\n", args[0]);
	    if(args[2] != NULL) {
			printf("%s: Too many arguments.\n", args[0]);
	    } else if (args[1] != NULL) { 
			char *tmp;
			tmp = getenv(args[1]);
			if(tmp == NULL)
				printf("%s: Environmental variable not found.\n", args[1]);
			else
				printf("%s\n", tmp);
			tmp = NULL;
	    }
	    else {
	        i = 0;
	        while(environ[i] != NULL) {
		    	printf("%s\n", environ[i]);
		    	i++;
			}
	    }
	} else if (strcmp(args[0], "setenv") == 0) {
 	    printf("Executing built-in [%s]\n", args[0]);
		if(args[3] != NULL) printf("%s: Too many arguments.\n", args[0]);
		else if (args[2] != NULL) {
			setenv(args[1], args[2], 1);
		} else if (args[1] != NULL) {
			setenv(args[1], "", 1);
		} else {
			i = 0;
			while(environ[i] != NULL) {
				printf("%s\n", environ[i]);
				i++;
			}
		}
	} else if (strcmp(args[0], "history") == 0) {
 	    printf("Executing built-in [%s]\n", args[0]);
		int n = 10;
		if(args[1] != NULL && atoi(args[1]) != 0) n = atoi(args[1]);
		struct pathelement *curr = history;
		while(curr != NULL && n > 0) {
			printf("%s\n", curr->element);
			curr = curr->next;
			n--;
		}
	} else if (strcmp(args[0], "alias") == 0) {
 	    printf("Executing built-in [%s]\n", args[0]);
		if(args[1] == NULL) {
			struct aliaselement *curr = aliases;
			while(curr != NULL) {
				printf("%s     %s\n", curr->name, curr->command);
				curr = curr->next;
			}
		} else if(args[2] == NULL) {
			struct aliaselement *curr = aliases;
			while(curr != NULL) {
				if(strcmp(args[1], curr->name) == 0) {
					printf("%s\n", curr->command);
				}
				curr = curr->next;
			}
		} else {
			char buf[MAX_CANON];
			snprintf(buf, MAX_CANON, "%s", args[2]);
			i = 3;
			while(args[i] != NULL && i < MAXARGS) {
				strcat(buf, " ");
				strcat(buf, args[i]);
				i++;
			}
			aliases = add_alias(aliases, args[1], buf);
		}
	} else if (strcmp(args[0], "kill") == 0) {
		if(args[1] == NULL) {
			printf("kill: Too few arguments.\n");
		} else if(args[2] == NULL || strchr(args[1], '-') == NULL) {
			i = 1;
			while(args[i] != NULL) {
				kill_process(args[i]);
				i++;
			}
		} else {
			char *signal;
			signal = strtok(args[1], "-");
			i = 2;
			while(args[i] != NULL) {
				kill_process_signal(args[i], signal);	
				i++;
			}
		}
	} else if (strcmp(args[0], "maxargs") == 0) {
	    printf("Error: Too many arguments.\n");
	} 

	else {
   	    fprintf(stderr, "%s: Command not found.\n", args[0]);   
    }
     /*  else  program to exec */
       /* find it */
      /* do fork(), execve() and waitpid() */

	i = 0;
	while(args[i] != NULL) {
	    free(args[i]);
	    args[i] = NULL;
	    i++;
	}

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

  return 0;
} /* sh() */

char *which(char *command, struct pathelement *pathlist )
{
   /* loop through pathlist until finding command and return it.  Return
   NULL when not found. */
  struct pathelement *current = pathlist;
  char buf[MAX_CANON];
  while(current != NULL) {
    snprintf(buf, MAX_CANON, "%s/%s", current->element, command);
	if(access(buf, F_OK) == 0) {
    	printf("%s\n", buf);
		break;
    }
    current = current->next;
  }
} /* which() */

char *where(char *command, struct pathelement *pathlist )
{
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

void list ( char *dir )
{
  /* see man page for opendir() and readdir() and print out filenames for
  the directory passed */
  
  DIR* dirstream;
  struct dirent* files;
  dirstream = opendir(dir);

  if(dirstream == NULL) {
    printf("Error opening: %s", dir);
  } else {
    printf("%s:\n", dir);
    while((files = readdir(dirstream)) != NULL){
        printf("%s\n", files->d_name);
    }
  }
  free(dirstream);
} /* list() */

char *cd( char *dir, char *homedir, char *prevdir )
{
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
void kill_process_signal(char* process, char *signal)
{
	int i = 0;
	if(atoi(process) && atoi(signal))
    	i = kill(atoi(process), atoi(signal));
    else
    	printf("kill: Arguments should be jobs or process id's.\n");
    if(i == -1)
    	perror(process);
}

/* try to terminate the given process */
void kill_process(char *process)
{
	int i = 0;
	if(atoi(process))
		i = kill(atoi(process),SIGTERM);
	else
		printf("kill: Arguments should be jobs or process id's.\n");
	if(i==-1)
		perror(process);
}
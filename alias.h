#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/* Add an alias to the list of aliases. If one with the same name exists, replace the command with the new command */
struct aliaselement *add_alias(struct aliaselement *head, char *name, char *command);

/* get the alias command with the given name, if it exists */
char *get_alias(struct aliaselement *head, char name);

struct aliaselement
{
  char *name;                       /* the name of the alias */
  char *command;			        /* command of the alias  */
  struct aliaselement *next;		/* pointer to next node */
};


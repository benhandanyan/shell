/*
  get_path.c
  Ben Miller

  Just a little sample function that gets the PATH env var, parses it and
  puts it into a linked list, which is returned.
*/
#include "get_path.h"

struct pathelement *get_path()
{
  /* path is a copy of the PATH and p is a temp pointer */
  char *path, *p;

  /* tmp is a temp point used to create a linked list and pathlist is a
     pointer to the head of the list */
  struct pathelement *tmp, *pathlist = NULL;

  p = getenv("PATH");	/* get a pointer to the PATH env var.
			   make a copy of it, since strtok modifies the
			   string that it is working with... */
  path = malloc((strlen(p)+1)*sizeof(char));	/* use malloc(3C) this time */
  strncpy(path, p, strlen(p));
  path[strlen(p)] = '\0';

  p = strtok(path, ":"); 	/* PATH is : delimited */
  do				/* loop through the PATH */
  {				/* to build a linked list of dirs */
    if ( !pathlist )		/* create head of list */
    {
      tmp = calloc(1, sizeof(struct pathelement));
      pathlist = tmp;
    }
    else			/* add on next element */
    {
      tmp->next = calloc(1, sizeof(struct pathelement));
      tmp = tmp->next;
    }
    tmp->element = p;	
    tmp->next = NULL;
  } while ( p = strtok(NULL, ":") );

  return pathlist;
} /* end get_path() */

struct pathelement *add_last(struct pathelement *head, char* command)
{
	if(head == NULL) {
		head = calloc(1, sizeof(struct pathelement));
		head->element = malloc((strlen(command) + 1) * sizeof(char));
		strcpy(head->element, command);
		head->next = NULL;
	} else {
		struct pathelement *tmp;
		tmp = calloc(1, sizeof(struct pathelement));
		tmp->element = malloc((strlen(command) + 1) * sizeof(char));
		strcpy(tmp->element, command);
		tmp->next = head;
		head = tmp;
	}
	return head;
}

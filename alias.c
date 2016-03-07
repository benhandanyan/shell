#include "alias.h"

struct aliaselement *add_alias(struct aliaselement *head, char* name, char* command)
{
	/* create the head if its null */
	if(head == NULL) {
		head = calloc(1, sizeof(struct aliaselement));
		head->name = malloc((strlen(name) + 1) * sizeof(char));
		strcpy(head->name, name);
		head->command = malloc((strlen(command) + 1) * sizeof(char));
		strcpy(head->command, command);
		head->next = NULL;
	} else {
		struct aliaselement *tmp = head;
		int exists = 0;
		/* Check for an existing alias */
		while(tmp != NULL) {
			if(strcmp(name, tmp->name) == 0) {
				free(tmp->command);
				tmp->command = malloc((strlen(command) + 1) * sizeof(char));
				strcpy(tmp->command, command);
				exists = 1;
				break;
			}
			tmp = tmp->next;
		}
		/* add a new alias if one doesn't already exist with this name */
		if(!exists) {
			tmp = calloc(1, sizeof(struct aliaselement));
			tmp->name = malloc((strlen(name) + 1) * sizeof(char));
			strcpy(tmp->name, name);
			tmp->command = malloc((strlen(command) + 1) * sizeof(char));
			strcpy(tmp->command, command);
			tmp->next = head;
			head = tmp;
		}
	}
	return head;
}



char *get_alias(struct aliaselement *head, char name)
{
	struct aliaselement *tmp = head;
	while(tmp != NULL) {
		if(strcmp(tmp->name, &name) == 0) {
			return tmp->command;
		}
		tmp = tmp->next;
	}
}

#include "users.h"

struct userelement *add_user(struct userelement *head, char *username)
{
	if(contains_user(head, username)) {
		return head;
	} else if(head == NULL) {
		head = calloc(1, sizeof(struct userelement));
		head->username = malloc((strlen(username) + 1) * sizeof(char));
		strcpy(head->username, username);
		head->next = NULL;
	} else {
		struct userelement *tmp;
		tmp = calloc(1, sizeof(struct userelement));
		tmp->username = malloc((strlen(username) + 1) * sizeof(char));
		strcpy(tmp->username, username);
		tmp->next = head;
		head = tmp;
	}
	return head;
}

struct userelement *remove_user(struct userelement *head, char *username) {
	struct userelement *tmp;
	if(!contains_user(head, username)) {
		return 0;
	} else if (strcmp(head->username, username) == 0) {
		tmp = head;
		head = head->next;
		free(tmp->username);
		free(tmp);
		tmp = NULL;
	} else {
		struct userelement *del;
		tmp = head;
		while(tmp->next != NULL) {
			if(strcmp(tmp->next->username, username) == 0) {
				del = tmp->next;
				tmp->next = tmp->next->next;
				free(del->username);
				free(del);
				del = NULL;
				break;
			}
			tmp = tmp->next;
		}
	}
	return head;
}

int contains_user(struct userelement *head, char *username) {
	if(head == NULL) {
		return 0;
	} else {
		struct userelement *tmp;
		tmp = head;
		while(tmp != NULL) {
			if(strcmp(tmp->username, username) == 0) {
				return 1;
			}
			tmp = tmp->next;
		}
	}
	return 0;
}

void print_users(struct userelement *head) {
	struct userelement *curr;
	curr = head;
	while(curr != NULL) {
		printf("%s -> ", curr->username);
		curr = curr->next;
	}
	printf("\n");
}

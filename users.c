#include "users.h"

/* Add a user to the linked list of watched users */
struct userelement *add_user(struct userelement *head, char *username) {
	/* if we already have the user, don't add a duplicate */
	if(contains_user(head, username)) {
		return head;
	} 
	/* if this is the first user */
	else if(head == NULL) {
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

/* Remove a user from the linked list of watched users */
struct userelement *remove_user(struct userelement *head, char *username) {
	struct userelement *tmp;
	/* if we don't have that user in the list, return */
	if(!contains_user(head, username)) {
		return head;
	} 
	/* if we are removing the head */
	else if (strcmp(head->username, username) == 0) {
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

/* determine if the username is in the list of watched users */
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

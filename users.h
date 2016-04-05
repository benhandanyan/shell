#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

struct userelement *add_user(struct userelement *head, char *username);
struct userelement *remove_user(struct userelement *head, char *username);
int contains_user(struct userelement *head, char *username);
struct userelement* login(struct userelement *head, char *username);
struct userelement* logoff(struct userelement *head, char* username);
struct userelement* check(struct userelement *head, char* username);
struct userelement* uncheckAll(struct userelement *head);

struct userelement {
	char *username;
	struct userelement *next;
	int logged_on;
	int checked;
};

struct userarg {
	struct userelement *users;
	pthread_mutex_t lock;
};

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

struct userelement *add_user(struct userelement *head, char *username);
struct userelement *remove_user(struct userelement *head, char *username);
int contains_user(struct userelement *head, char *username);
void print_users(struct userelement *head);

struct userelement
{
  char *username;
  struct userelement *next;
};


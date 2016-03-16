# choose your compiler
CC=gcc
#CC=gcc -Wall

mysh: sh.o get_path.o alias.o users.o main.c 
	$(CC) -g main.c sh.o get_path.o alias.o users.o -o mysh -lpthread
#	$(CC) -g main.c sh.o get_path.o bash_getcwd.o -o mysh

sh.o: sh.c sh.h
	$(CC) -g -c sh.c

get_path.o: get_path.c get_path.h
	$(CC) -g -c get_path.c

alias.o: alias.c alias.h
	$(CC) -g -c alias.c

users.o: users.c users.h
	$(CC) -g -c users.c

clean:
	rm -rf sh.o get_path.o mysh

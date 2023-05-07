# Simple C Shell Makefile
CC = gcc
CFLAGS  = -Wall -g
OBJ = mysh.o

all: mysh

mysh: $(OBJ)
	$(CC) $(CFLAGS) -o mysh $(OBJ) 

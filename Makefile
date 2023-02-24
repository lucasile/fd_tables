
CC=gcc
TARGET=fd_tables
RM=rm

default: all

all: fd_tables

fd_tables: fd_tables.c
	$(CC) fd_tables.c -Wall -o $(TARGET) -lm

clean:
	$(RM) $(TARGET)


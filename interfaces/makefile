CC = gcc
CFLAGS = -Wall -Werror
LIB = -pthread
OUTPUT = s.out
DEBUG = -g
SOURCES = server.c

all:
	$(CC) $(SOURCES) $(CFLAGS) $(LIB) -o $(OUTPUT)
debug:
	$(CC) $(SOURCES) $(CFLAGS) $(DEGUB) -o $(OUTPUT)
clean:
	rm -f $(OUTPUT)
fresh:
	make clean
	make all

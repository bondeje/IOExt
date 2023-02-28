UNAME := $(shell uname)
CC = gcc

ifeq ($(UNAME), Linux)
EXT = 
LFLAGS = -lm
else
EXT = .exe
LFLAGS =
endif

CFLAGS = -std=c99 -O2 -o test_IOExt$(EXT)

all: build

build:
	$(CC) $(CFLAGS) test_IOExt.c ../src/IOExt.c $(LFLAGS)

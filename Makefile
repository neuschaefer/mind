all: mind

CC=gcc
LD=gcc

CFLAGS+=-Wall
LDFLAGS+=

mind: mind.o
	$(LD) $(LDFLAGS) -o $@ $<

mind.o: mind.c

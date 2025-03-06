
CC=gcc
CFLAGS=-Wall -pthread -g
LIBS=
OBJS=main.o options.o op_count.o rw_mutex.o

PROGS= main

all: $(PROGS)

%.o : %.c
	$(CC) $(CFLAGS) -c $<

main: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

clean:
	rm -f $(PROGS) *.o *~


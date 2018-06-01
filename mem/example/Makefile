CC=gcc

FTLIB_ROOT=../../


CFLAGS += -I$(FTLIB_ROOT)/include
CFLAGS += -g -rdynamic -Wall -fPIC

LDFLAGS += -L$(FTLIB_ROOT)
LDFLAGS += -lft


all : example

example : example.c
	@echo "  CC $^"
	@$(CC) $(LDFLAGS) $(CFLAGS)  $^ -o $@


clean:
	rm -f *.o
	rm -f example

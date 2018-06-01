
FT_ROOT=$(PWD)


CC := gcc
LD := ld
STRIP := strip

CFLAGS += -I$(FT_ROOT)/include
CFLAGS += -g -rdynamic -Wall
CFLAGS += -fPIC

LDFLAGS +=

FT_LIB := $(FT_ROOT)/libft.so
FT_SRC := $(wildcard */*.c)
FT_OBJ := $(FT_SRC:%.c=%.o)

all: prep $(FT_LIB)

%.o : %.c
	@echo "  CC $^"
	@$(CC) $(CFLAGS) -c $^ -o $@

$(FT_LIB) : $(FT_OBJ)
	@echo "  LD $@"
	@$(LD) $(LDFLAGS) -shared -o $@ $^

prep:

install:

clean:
	rm -f $(FT_OBJ)
	rm -f $(FT_LIB)

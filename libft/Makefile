
STATIC_LIB	= libft.a
DYNAMIC_LIB	= libft.so

SRC := $(wildcard *.c)
SRC += $(wildcard */*.c)
OBJ := $(patsubst %.c,%.o,$(SRC))

CFLAGS  += -fPIC -DMEMWATCH
CFLAGS  += -I./include
LDFLAGS	+= -fPIC


.PHONY : all install clean


all: $(DYNAMIC_LIB) $(STATIC_LIB)


$(STATIC_LIB) : $(OBJ)
	@echo AR $@
	@${AR} -cvq $@ ${OBJ} > /dev/null

$(DYNAMIC_LIB) : $(OBJ)
	@echo LD $@
	@$(CC) -shared  $(LDFLAGS) -o  $(DYNAMIC_LIB) $(OBJ)

%.o : %.c
	@echo CC $@
	@$(CC) $(CFLAGS) -c $< -o $@

install : $(DYNAMIC_LIB)
	$(INSTALL) -d -m 755 $(INSTALL_LIB_DIR)
	$(INSTALL) -c -m 644 $(DYNAMIC_LIB) $(INSTALL_LIB_DIR)

clean :
	rm -f $(OBJ) $(DYNAMIC_LIB) $(STATIC_LIB)

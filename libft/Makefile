
STATIC_LIB	= libft.a
DYNAMIC_LIB	= libft.so

SRC := $(wildcard *.c)
OBJ := $(patsubst %.c,%.o,$(SRC))

CFLAGS  += -fPIC -DMEMWATCH
LDFLAGS	+= -fPIC


.PHONY : all install clean


all: $(DYNAMIC_LIB) $(STATIC_LIB)


$(STATIC_LIB) : $(OBJ)
	${AR} -cvq $@ ${OBJ}

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
	rm -f *.o $(DYNAMIC_LIB) $(STATIC_LIB)

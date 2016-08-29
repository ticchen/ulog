APP = ulog
APP2 = ulogread
SOURCE = $(wildcard *.c)
OBJS = $(SOURCE:.c=.o)

CFLAGS = -ffunction-sections -fdata-sections -MMD -MP
LDFLAGS = -Wl,--gc-sections
LDLIBS =


all: astyle $(APP) $(APP2)

$(APP): $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@
	
$(APP2):
	ln -sf $(APP) $(APP2)

clean:
	rm -rf $(APP) $(APP2)
	rm -rf *.o *.orig *.d

astyle:
	find ./ -type f -name '*.[ch]' | xargs -n 1 astyle --style=linux --indent=tab --pad-oper --unpad-paren --align-pointer=name --add-brackets

-include $(SOURCE:.c=.d)

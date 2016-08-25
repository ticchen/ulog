CC = gcc

all: astyle clean ulog ulogread
	
ulogread:
	ln -sf ulog ulogread

clean:
	rm -rf ulog ulogread
	rm -rf *.o *.orig

astyle:
	find ./ -type f -name '*.[ch]' | xargs -n 1 astyle --style=linux --indent=tab --pad-oper --unpad-paren --align-pointer=name --add-brackets 
